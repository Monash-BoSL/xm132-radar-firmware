# radar-firmware
Here we dedicate this repository to the development firmware for the Acconeer XM132 in the BoSL radar sensor. 
The firmware uses a similar interface to the acconeer module software for the XM132.
Currently only envelope and sparse services are implemented.
The project is focused on low cost and low power applications. We use a Acconneer XM132 for the radar funcionality and an Arduino ATmega328PB for interfaceing the sensor.
The cost of the sensor is about 50AUD, an order of magnitude improvement on existing commertial solutions. 
Combining the velocity and height measurements allows a estimation of flow velocity in a single device!

Please use these files with an installation of the acconeer sdk. 

For more information please check out our wiki page: http://www.bosl.com.au/wiki/Radar_Velocity
For the main repository on the BoSL radar sensor check out: https://github.com/Monash-BoSL/radar-velocity
