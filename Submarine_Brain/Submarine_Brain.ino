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
#include <Wire.h>
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
int arr[7];

// Sensors
MS5837 Bar30;
Adafruit_BNO055 bno = Adafruit_BNO055(22);

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

  // PinMode Configuration
  pinMode(A15,INPUT);
  pinMode(A8,INPUT_PULLUP);
  pinMode(A9,INPUT_PULLUP);

  // Attaching Interrupts
  attachInterrupt(digitalPinToInterrupt(A8), ohNo, FALLING);
  attachInterrupt(digitalPinToInterrupt(A9), ohNo, FALLING);
  
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
          if(water)
            client.print("WATER");
          else
            client.print("SAFE");
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
  internalTemp = analogRead(A15);
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
