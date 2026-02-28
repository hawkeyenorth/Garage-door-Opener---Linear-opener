This is the sketch to upload to a Raspberry Pi Pico W.  This will not work on an Arduino, but most likely can be modified to adapt.  Upload using a C++ compiler, (written for Arduino IDE).  Additional libraries will be required from other sources to make this work,  

This program is written to make use of the internal flash to store the passwords, and any SSID changes.  Also included in this repository is the schematic to create the unit.  A few things to consider:
You will need to define your default password, and the SSID.  Also define the password to access the unit as the ADMIN.  
The reset button is set up to require you to hold it for 5 seconds, and the LED on the pi will flash 10 times to indicate that the unit is completely reset, erasing all data in the Flash Ram
The IP address it uses for the web page is 192.168.42.1.  This is hard coded, but you can change it to suit your needs
The unit is set up as an Access Point, not a station.  This way it can be a stand alone unit.  
The WIFI signal is weak, so it is not recommended to try to set up as a station, unless you have a access point close by.  
