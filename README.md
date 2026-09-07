# station_selection_controller

This is the 'front panel' to a station automation project to control the operation of the various radios and 
transverters in the W2SZ rover van. We use a repurposed Leitch 16x1SBA button box to control band switch 
configuration, audio routing, PTT routing, and RF routing for a sophisticated 12 band, one to three operator amateur radio station.

The basic operation is the controller scans the buttons, publishes the button status to the local MQTT broker. It 
is also subscribed to the controller  and receives the update from the broker, from which it updates the LEDs to 
visually confirm that the band selection has changed. 

# About the Leitch 16x1SBA

Originally intended for TV/radio station or other performance space applications for audio/video switching,
the Leitch 16x1SBA is a panel with one row of 16 (lit) selection buttons and three other buttons. Each of 
the 16 buttons has a pair of red & green LEDs over it which are used to indicate which source/destination is 
selected. The system can cascade these button panels and associated switch gear to create large and complex systems. 
In the changeover to more digital solutions, many of these panels, and sometimes the associated switch gear,
has become available on the surplus market. I have obtained a few of these panels at flea markets and 
hamfests over the years with an eye to putting them to use within my station control architecture. 

The 16x1SBA has two circuit boards and a 110/220VAC to 5vdc power supply inside. The two boards are a 
front panel with the associated switches/LEDs and a controller that reads the front panel, formats the messages 
and sends the status upstream to the system controller. Since the protocol and code contained within the 68HC11 
controller are proprietary, I have removed the controller board and replace it with a local network 
enabled microprocessor such as an ESP32 or Arduino that uses Ethernet or WiFi.  The initial version 
uses WiFi for testing, the final implementation will be hardwired Ethernet so as not to conflict with 2.4GHz ham radio band. 

The switch/LED board is a relatively simple affair with the 19 switches scanned via a set of shift registers 
and the LEDs are updated similarly. The interface is presented on a 16pin IDC header which we utilize directly. 

Original testing of the front panel was done using the code found elsewhere in the project 'cylon' and it's corresponding 
repository. 
