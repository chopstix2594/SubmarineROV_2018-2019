// Ryan Baszkowski, Brennan Lambert, Nathan Loughner
// ECE 486, ECE 487, MAE 487, w/ Dr. Jenkins, Dr. Shultz, Dr. Shaw, and Dr. Wright
// Submarine Remote Operated Vehicle Project
// For client Dr. Anthony Choi
// With Dr. Choi, Dr. Fu, and Dr. Sumner as advisiors
// And with Dr. Wright as project manager
//// Submarine ROV Internal Microcontroller
//// Arduino Sketch
// First build on 30 January 2019

#include <Ethernet.h>
#include <Servo.h>
#include "MS5837.h"
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>

// Network Globals
byte mac[] = {0x30,0x00,0x02,0x00,0x00,0x00};
IPAddress ip(192, 168, 1, 182);
EthernetServer server(80);
EthernetClient client;

// Servos
Servo left, top, front, back, bottom, right, light;

// Servo Speed Values
int nleft = 1500;
int ntop = 1500;
int nfront = 1500;
int nback = 1500;
int nbottom = 1500;
int nright = 1500;
int nlight = 1100;

// Sensors
MS5837 Bar30;
const int ad8495 = A15;
Adafruit_BNO055 bno = Adafruit_BNO055(22);
const int waterSens = 23;

// Sensor Values
int internalTemp;
int externalTemp;
int pressure;
int depth;
int alt;
float x_o;
float y_o;
float z_o;
bool water;

// String Parsing
int i = 0;
int count = 0;
char c;
String temp = "";
String currentLine;

void setup() {
  // Ethernet configuration
  Ethernet.init(10);
  Ethernet.begin(mac,ip);
  //Serial.begin(115200);
  //while(!Serial){;}
  if (Ethernet.hardwareStatus() == EthernetNoHardware){
  //Serial.println("No shield");
    while(true)
      delay(1);
  }
  server.begin();
  //Serial.print("Server at ");
  //Serial.println(Ethernet.localIP());

  // Other Steup
  water = false;
  Bar30.init();
  Bar30.setFluidDensity(997); // Value for freshwater

  // Attaching Interrupts
  attachInterrupt(digitalPinToInterrupt(waterSens), ohNo, FALLING);

  // PinMode Configuration
  pinMode(A15,INPUT);
  pinMode(23,INPUT_PULLUP);

  // Servo Configuration
  light.attach(15); // The Lumen LED light's PWM control is designed to be compatible with the servo library
  left.attach(16);
  top.attach(17);
  front.attach(18);
  back.attach(19);
  bottom.attach(20);
  right.attach(21);
  
  left.writeMicroseconds(1500); // Write neutral values for the ESCs to initialize
  top.writeMicroseconds(1500);
  front.writeMicroseconds(1500);
  back.writeMicroseconds(1500);
  bottom.writeMicroseconds(1500);
  right.writeMicroseconds(1500);
  light.writeMicroseconds(1100); // Write the "off" value for the light
  delay(2500);
  //Serial.println("It's showtime...");
}

void loop(){
  readSensors();
  webServer();
}

void webServer(){
  client = server.available();
  if(client){
    //Serial.println("new client");
    currentLine = "";
    while(client.connected()){
      if(client.available()){
        c = client.read();
        //Serial.write(c);
        if(c== '\n'){
          if(currentLine.length() == 0){
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/plain");
          client.println("Connection: close");
          client.println();
          client.print(internalTemp);
          client.print('#');
          client.print(externalTemp);
          client.print('#');
          client.print(pressure);
          client.print('#');
          client.print(depth);
          client.print('#');
          client.print(alt);
          client.print('#');
          client.print(x_o,4);
          client.print('#');
          client.print(y_o,4);
          client.print('#');
          client.print(z_o,4);
          break;
          }
        else
          currentLine = "";
        }
        else if(c != '\r')
         currentLine += c;
        if(currentLine.endsWith("opcode")){ // Client should give command in "$left-top-front-back-bottom-right-light_opcode"
         codeParse(currentLine);
        }
      }
    }
    client.stop();
    //Serial.println("Disconnected");
  }
}

void readSensors(){
  internalTemp = analogRead(ad8495);
  externalTemp = Bar30.temperature();
  pressure = Bar30.pressure();
  depth = Bar30.depth();
  alt = Bar30.altitude();

  sensors_event_t event;
  bno.getEvent(&event);
  x_o = event.orientation.x;
  y_o = event.orientation.y;
  z_o = event.orientation.z;
}

void ohNo(){
  water = true;
}

void codeParse(String item){
  while(item[i] != '$')
    i++;
  i++;
  while(item[i] != '_'){
    while(item[i] != '-' && item[i] != '_'){
      temp += item[i];
      i++;
    }
    //Serial.println(temp);
    if(c = 0)
      left.writeMicroseconds(temp.toInt());
    else if(count = 1)
      top.writeMicroseconds(temp.toInt());
    else if(count = 2)
      front.writeMicroseconds(temp.toInt());
    else if(count = 3)
      back.writeMicroseconds(temp.toInt());
    else if(count = 4)
      bottom.writeMicroseconds(temp.toInt());
    else if(count = 5)
      right.writeMicroseconds(temp.toInt());
    else if(count = 6)
      light.writeMicroseconds(temp.toInt());
    temp = "";
    count++;
    i++;
  }
  i = 0;
  count = 0;
}
