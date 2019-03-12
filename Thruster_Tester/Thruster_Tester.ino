// Ryan Baszkowski, Brennan Lambert, Nathan Loughner
// ECE 486, ECE 487, MAE 487, w/ Dr. Jenkins, Dr. Shultz, Dr. Shaw, and Dr. Wright
// Submarine Remote Operated Vehicle Project
// For client Dr. Anthony Choi
// With Dr. Choi, Dr. Fu, and Dr. Sumner as advisiors
// And with Dr. Wright as project manager
//// ESC Testing and Calibration Software
//// Arduino Sketch
// First build on 30 January 2019

// SET SERIAL MONITOR TO "NO LINE ENDINGS"
// TEST SERVO CONNECTS TO PIN 21 ON ARDUINO MEGA R3

#include <Servo.h>

Servo servo1;
int spd = 1500;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  servo1.attach(21);
  servo1.writeMicroseconds(1500);
  delay(7000);
  Serial.println("It's showtime...");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    spd = Serial.parseInt(); // Doesn't like line ending characters
    Serial.println(spd);
  }
  servo1.writeMicroseconds(spd);
}
