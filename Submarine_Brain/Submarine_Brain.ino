// Ryan Baszkowski, Brennan Lambert, Nathan Loughner
// ECE 486, ECE 487, MAE 487, w/ Dr. Jenkins, Dr. Shultz, Dr. Shaw, and Dr. Wright
// Submarine Remote Operated Vehicle Project
// For client Dr. Anthony Choi
// With Dr. Choi, Dr. Fu, and Dr. Sumner as advisiors
// And with Dr. Wright as project manager
//// Submarine ROV Internal Microcontroller
//// Arduino Sketch
// First build on 30 January 2019

// Please read the comments in "MainPage.xaml.cpp in "Submarine" for an introduction
// Almost everything is defined as a global variable so I could keep track of memory usage easily
// Make sure that either an SD card is not inserted in the shield, or pin 4 is held HIGH or this will break

#include <Ethernet.h>
#include <Servo.h>
#include <Wire.h>
#include "MS5837.h"
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>

// Network Globals
byte mac[] = {0x30,0x00,0x02,0x00,0x00,0x00};
IPAddress ip(192, 168, 1, 182);
EthernetServer server(80);
EthernetClient client;

// ESCs and the Light, which are all controlled by servo objects
Servo left, top, front, back, bottom, right, light;

// Servo Speed Values
int nleft = 1500;
int ntop = 1500;
int nfront = 1500;
int nback = 1500;
int nbottom = 1500;
int nright = 1500;
int nlight = 1100;
int arr[7];

// Sensors
MS5837 Bar30;
Adafruit_BNO055 bno = Adafruit_BNO055(55);

// Sensor Values
int thermoread;
float thermovolts;
int internalTemp;
float externalTemp;
float pressure;
float depth;
float alt;
float x_o;
float y_o;
float z_o;
bool water;
int wat1;
int wat2;

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
  bno.begin();
  delay(1000);
  bno.setExtCrystalUse(true);
  Bar30.init();
  Bar30.setFluidDensity(997); // Value for freshwater

  // PinMode Configuration
  pinMode(A15,INPUT);
  pinMode(A8,INPUT);
  pinMode(A9,INPUT);
  
  // Servo Configuration
  light.attach(23); // The Lumen LED light's PWM control is designed to be compatible with the servo library
  left.attach(51);
  top.attach(49);
  front.attach(47);
  back.attach(45);
  bottom.attach(43);
  right.attach(41);
  
  left.writeMicroseconds(1500); // Write neutral values for the ESCs to initialize
  top.writeMicroseconds(1500);
  front.writeMicroseconds(1500);
  back.writeMicroseconds(1500);
  bottom.writeMicroseconds(1500);
  right.writeMicroseconds(1500);
  light.writeMicroseconds(1100); // Write the "off" value for the light
  delay(5000);
  //Serial.println("It's showtime...");
}

// Read the sensors, then do network stuff
void loop(){
  readSensors();
  webServer();
}

// Present the sensor data as a web page, look for requests, control the servos to match those requests
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
          client.print(externalTemp,4);
          client.print('#');
          client.print(pressure,4);
          client.print('#');
          client.print(depth,4);
          client.print('#');
          client.print(alt,4);
          client.print('#');
          client.print(x_o,4);
          client.print('#');
          client.print(y_o,4);
          client.print('#');
          client.print(z_o,4);
          client.print('#');
          client.print(wat1);
          client.print('#');
          client.print(wat2);
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

// Read the sensors
void readSensors(){
// Problems with AD8495, temperature from BNO055 used instead
//  thermoread = analogRead(A15);
//  thermovolts = thermoread * (5.0 / 1023.0);
//  internalTemp = (thermovolts) / 0.005;
  internalTemp = bno.getTemp(); // Here
  
  externalTemp = Bar30.temperature();
  pressure = Bar30.pressure();
  depth = Bar30.depth();
  alt = Bar30.altitude();

  sensors_event_t event;
  bno.getEvent(&event);
  x_o = event.orientation.x;
  y_o = event.orientation.y;
  z_o = event.orientation.z;


  wat1 = analogRead(A8);
  wat2 = analogRead(A9);
}

// The computer sends these as a string, gotta parse to get the values to the ESCs
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
    arr[count] = temp.toInt();
    temp = "";
    count++;
    i++;
  }
//  Serial.println("Left: ");
//  Serial.println(arr[0]);
//  Serial.println("Top: ");
//  Serial.println(arr[1]);
//  Serial.println("Front: ");
//  Serial.println(arr[2]);
//  Serial.println("Back: ");
//  Serial.println(arr[3]);
//  Serial.println("Bottom: ");
//  Serial.println(arr[4]);
//  Serial.println("Right: ");
//  Serial.println(arr[5]);
//  Serial.println("Light: ");
//  Serial.println(arr[6]);
  left.writeMicroseconds(arr[0]);
  top.writeMicroseconds(arr[1]);
  front.writeMicroseconds(arr[2]);
  back.writeMicroseconds(arr[3]);
  bottom.writeMicroseconds(arr[4]);
  right.writeMicroseconds(arr[5]);
  light.writeMicroseconds(arr[6]);
  i = 0;
  count = 0;
}
