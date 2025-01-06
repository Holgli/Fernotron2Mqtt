# Fernotron 2 MQTT Gateway

Fernotron 2 MQTT Gateway allows you to send MQTT messages from a Fernotron sender to a MQTT broker. So you can connect your Fernotron senders to a home automation software and send open/close/stop commands from your Rademacher 2411 central unit, your Rademacher 2440 sun sensor or Rademacher 2430 o.a. plain sender.

There are several projects that allow you to replace a central untit with a mobile app or to control your Fernotron motors over a home automation server.

The goal of this project is the opposite. Reuse your old Rademacher Fernotron senders and use them to send commands to a MQTT broker to control your shutter motors.

## Fernotorn 2 MQTT hardware

To build the gateway you just need two components: an ESP32 MCU and a 433MHz receiver module. On Amazon or Ebay you find lots of them. Connect the receiver to the ESP32 with some jumper cables, connect the ESP32 to an USB power supply and that it is. I used a ESP32 D1 Mini from AZ-Delivery and a CC1101 Wireless Modul for 433 MHz. Alternatively you can use a MX-05V / RXB8 type receiver module. The wiring is as follows:
<pre> 
  C1101 Module
             C1101     ESP32
      GND      1        GND
      VCC      2        3.3
      GDO0     3         2
      CSN      4         5
      SCK      5        18
      MOSI     6        23
      MISO     7        19
      GDO2     8        22
 
  XY-MK-5V / RXB8 Module
            XY-MK-5V   ESP32
              GND       GND
              VCC       5V
              DATA      22
</pre> 
For both modules the data input pin is IO pin 22. The CC1101 uses a SPI connection, so if you use a MX-05V receiver the serial monitor will show a C1101 connection error and a buildin led (connected to GPIO 2) will blink 5 times. This is normal and you can ignore this message. 

The log should display something like:<pre> 
C1101 Connection OK
Wait for WiFi...
WiFi Connection OK
IP address: x.x.x.x
Attempting MQTT connection...connected
Web-Server started.
</pre> 

Optionally you can put the hardware in a housing, solder the connectors and so on. This will make all look more professional. 


## Fernotron 2 MQTT software

The main goal was to keep things simple. The software is written in C++, but not using any cryptic language constructs or a sophisticated class hierarchie. It is neither space nor time optimized, the code should be easy to understand. Most things are done in strings so that you always can log whats happening. After decoding a Fernotron message the sender id, counter, group id, member id and the command are logged to the serial monitor. Thanks to Bert Winkelmann the protocol is well documented. 

### 1 Install MS Visual Studio Code and the PlatformIO extension

There are lots of tutorials how to install VSCode and the PlatformIO extension.

### 2 Download the the software as zip file or clone the Git repository

Alternatively create a PlatformIO project (project name: Fernotron2MQTT, board: *your board*, framework: Arduino>) and put the five cpp files into the src folder and the corresponding header files into the include folder. Use the library manager to add the ESP Async WebServer, the MTQQ PubSubClient and the SmartRC-CC1101-Driver-Lib to your project.<pre> 
Excerpt platfomio.ini:

ib_deps = 
	lsatan/SmartRC-CC1101-Driver-Lib @ ^2.5.7
	me-no-dev/ESP Async WebServer@^1.2.4
	knolleary/PubSubClient@^2.8
</pre> 

The C1101 library is only necessary if you use a C1101 transceiver module. The libary is used to initialize the module and to set some basic parameters. If you use a MX-05V type receiver module the library can be omitted and you can comment out the C1101 code in main.cpp. 

The ESP Async WebServer library is used to make some gateway informations acessable to your browser. 

### 3 Compile the software 

There are two locations where you have to do modifications to the software. First you have to set your wifi **ssid** and your wifi **password**. This two wifi constants you find in **wificonnection.h**. Next you have to change the MQTT connection settings. Enter your MQTT server address / port and your credentials in **mqttconnection.h**.
Connect the developement board to your computer, compile the software and upload it to your board. In the serial monitor you should see the decoded messages if you press a button on your Fernotron sender. Here you also find the ip address of the gateway.

### 4 Subscribe to gateway topics

The gateway publishes the following topics:
+ Fernotron2MQTT/
  + PlainSender/ (or SunSensor or CentralUntit)
    + ID_**of your sender**/
      + **command** (up / down / stop)

<pre> 
Example: Fernotron2MQTT/PlainSender/ID_106854/stop
</pre> 

In case of a central unit there are additional topics for group and member id.

<pre> 
Example: Fernotron2MQTT/CentralUnit/ID_8020df/Group_1/Member_1/down
</pre> 

Additionally the gateway sends a JSON paylod with each message which contains all informations the Fernotron sender transmitts.

<pre> 
Example: {"Id":"8020df","Group":"1","Member":"1","Action":"5","Counter":"9"}
</pre> 

You can find the id of your sender in the serial montor, in the commad history or by a MQTT explorer software. Then you can subscribe to the topics to create automations for opening / stopping / closing shutters for example.



### 5 Debugging

If not connected to the serieal monitor the gateway gives you some feedback by a led and a web page. 
A ESP32 D1 Mini e.g. has an internal led connected to GPIO 02. If there is a SPI connection error (C1101 module) this led will blink 5 times after a reset. If a Fernotron command is recognized this led will flash shortly. So it will make sense to use an external led if your board is missing an internal one.
The gateway uses a web server to make some further informations available. Point your browser to the ip address of your Fernotron 2 MQTT Gateway (you find the ip in the log after start or reset) or check it out in your router. The gateway responses with a page giving you the list of the last 100 commands. The page will also show the rssi values of the wifi and C1101 connection. 


## Some final words
+ The software currently ignors almost all error detection mechanisms of the protocol (parity bits, control words, retransmissions). Here is room for improvements. 
+ It is necessary to compile the software with your wifi and MQTT credentials.
+ The sun sensors do only have a distance range of 10m. 
+ Tested with AZ-Delivery D1 Mini ESP32, Rademacher 2411 central unit,  2440 sun sensor and 2430 sender

Holger Linning, Dec 2024 

