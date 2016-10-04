
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager


// General brightness of the candles, used to dim or to swicht off
byte brightnes = 0;

// original color to come up
byte base_red = 100;
byte base_green = 55;
byte base_blue = 20;

int target_red = 0;
int target_green = 0;
int target_blue = 0;

int current_red = 0;
int current_green = 0;
int current_blue = 0;

#define CYCLEMOVE  (PWMRANGE/50)

int cyclemover;
int cyclemoveg;
int cyclemoveb;



WiFiUDP listener;
ESP8266WebServer server(80);

// RGB FET
#define redPIN    15 //12
#define greenPIN  13 //15
#define bluePIN   12 //13


// W FET
#define w1PIN     14
#define w2PIN     4

// onbaord green LED D1
#define GREENLEDPIN    5
// onbaord red LED D2
#define REDLEDPIN   1

// note
// TX GPIO2 @Serial1 (Serial ONE)
// RX GPIO3 @Serial


#define GREENLEDoff digitalWrite(GREENLEDPIN,HIGH)
#define GREENLEDon digitalWrite(GREENLEDPIN,LOW)

#define REDLEDoff digitalWrite(REDLEDPIN,HIGH)
#define REDLEDon digitalWrite(REDLEDPIN,LOW)


int gamma_PWM(int c ){
  float percent;
  percent = constrain(c*brightnes/100,0,100); // percentage of 100 to put the color

  // gamma correction X^3
  percent = constrain(percent * percent * percent / 10000, 0, 100);

  return constrain(percent*PWMRANGE/100,0,PWMRANGE);
}
// define target PWM value based on base value and brightnes
void set_rgb() {
  int diffr, diffg, diffb;

  //this constrains show my ditrust in my very own programming skills ;-)
//  target_red =   constrain(base_red  *brightnes*PWMRANGE/10000,0,PWMRANGE);
//  target_green = constrain(base_green*brightnes*PWMRANGE/10000,0,PWMRANGE);
//  target_blue =  constrain(base_blue *brightnes*PWMRANGE/10000,0,PWMRANGE);
  target_red =   gamma_PWM(base_red);
  target_green = gamma_PWM(base_green);
  target_blue =  gamma_PWM(base_blue);

  diffr = abs( current_red - target_red);
  diffg = abs( current_green - target_blue);
  diffb = abs( current_blue - target_green);

  if(diffr>diffg && diffr>diffb){ // red is the biggest
    cyclemover = CYCLEMOVE;
    cyclemoveg = constrain( CYCLEMOVE * diffg / diffr , 1, CYCLEMOVE );
    cyclemoveb = constrain( CYCLEMOVE * diffb / diffr , 1, CYCLEMOVE );
  }else if(diffg>diffb && diffg>diffr){ //green is the biggest
    cyclemoveg = CYCLEMOVE;
    cyclemover = constrain( CYCLEMOVE * diffr / diffg , 1, CYCLEMOVE );
    cyclemoveb = constrain( CYCLEMOVE * diffb / diffg , 1, CYCLEMOVE );
  }else{ //blue is the biggest
    cyclemoveb = CYCLEMOVE;
    cyclemoveg = constrain( CYCLEMOVE * diffg / diffb , 1, CYCLEMOVE );
    cyclemover = constrain( CYCLEMOVE * diffr / diffb , 1, CYCLEMOVE );
  }
}


int move_cycle(int current, int target, int moveval){
  int diff = target - current;

  if(diff >   moveval ) diff = moveval;
  if(diff < (-moveval)) diff = -moveval;

  return constrain(current + diff, 0, PWMRANGE);
}

void move_rgb(){
  current_red =   move_cycle( current_red,   target_red  , cyclemover );
  current_green = move_cycle( current_green, target_green, cyclemoveg );
  current_blue =  move_cycle( current_blue,  target_blue , cyclemoveb );
  analogWrite(redPIN, current_red);
  analogWrite(greenPIN, current_green);
  analogWrite(bluePIN, current_blue);
}

void loop() {

  server.handleClient();


    move_rgb();
    delay(25);
    move_rgb();
    delay(25);
    move_rgb();
    delay(25);
    move_rgb();
    delay(25);
    move_rgb();
    delay(25);


}


void setupWebServer(){
 // listen to brightness changes
 server.on("/", []()
 {
   if (server.arg("b") != "")
   {
     brightnes = (byte)constrain( server.arg("b").toInt(), 0, 100);
   }
   String s = "{\n   \"b\":";
   s += brightnes;
   s += "\n}";
   server.send(200, "text/plain", s);
   set_rgb();
 });
 server.on("/color", []()
 {
   if (server.arg("r") != "")
   {
     base_red = (byte) constrain( server.arg("r").toInt(), 0, 100);
   }
   if (server.arg("g") != "")
   {
     base_green = (byte) constrain( server.arg("g").toInt(), 0, 100);
   }
   if (server.arg("b") != "")
   {
     base_blue = (byte) constrain( server.arg("b").toInt(), 0, 100);
   }
   String s = "{\n   \"r\":";
   s += base_red;
   s += ", \"g\":";
   s += base_green;
   s += ", \"b\":";
   s += base_blue;
   s += "\n}";
   server.send(200, "text/plain", s);
   set_rgb();
 });

 // start the webserver
 server.begin();

}

// the setup function runs once when you press reset or power the board
void setup() {

WiFiManager wifiManager;

// pin setup
pinMode(GREENLEDPIN, OUTPUT);
pinMode(REDLEDPIN, OUTPUT);

pinMode(redPIN, OUTPUT);
pinMode(greenPIN, OUTPUT);
pinMode(bluePIN, OUTPUT);
pinMode(w1PIN, OUTPUT);
pinMode(w2PIN, OUTPUT);

GREENLEDon;


 //fetches ssid and pass from eeprom and tries to connect
 //if it does not connect it starts an access point with the specified name
 //here  "H801"
 //and goes into a blocking loop awaiting configuration
 wifiManager.autoConnect("H801");


 setupWebServer();

 // in do not want to have so much extra light all the time..
 REDLEDoff;
 GREENLEDoff;
}
