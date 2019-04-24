# SubmarineROV_2018-2019
### Mercer University School of Engineering Senior Design 2018-2019
Team: Ryan Baszkowski, Brennan Lambert, Nathan Loughner\
With Dr. Anthony Choi, Dr. Ruiyun Fu, and Dr. Loren Sumner as advisors\
With Dr. James Wright as project manager\
For client Dr. Anthony Choi\
This repository contains all of the software for operation of the Submarine Drone outlined in the following documents:\
https://drive.google.com/open?id=1T8Uh6GV0HjEKu9xRt5F1R8sCDmDm1KGB \\
All 3D models generated throughout the project are available here:\
https://drive.google.com/open?id=1v8MOAiqluuzHnB4pcn_lG2sQ_G6snyrh
## Sub-projects contained:
### Submarine
GUI drone piloting interface for the Submarine ROV\
Universal Windows Program (UWP) x86 and x86_64 (Maybe ARM/ARM64? It should work...)\
Written in C++/CX (Visual C++)\
Requires Windows 10 with Anniversary Update at minimum (Build 14393)\
Building from Source requires Visual Studio 2017 and the C++ Universal Windows Development Package
### Submarine_Brain
Arduino sketch for the Submarine ROV's internal controller\
For Arduino MEGA and compatibles\
Developed targeting an Elegoo MEGA R3
### Thruster_Tester
Arduino sketch for ESC calibration through the serial monitor.\
For Arduino MEGA and compatibles\
Developed targeting an Elegoo MEGA R3
