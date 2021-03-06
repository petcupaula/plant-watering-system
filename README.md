# plant-watering-system

## ESP32 WiFi

<img src="esp32-wifi/plant-watering-system_bb.png" width="800">

### Building

To build this project, first install [esp-idf from
github](https://github.com/espressif/esp-idf/) and follow the
instructions [getting started
guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/)
to setup.

When done activate your esp-idf environment and configure the application.

```
$ idf.py menuconfig
````

It's especially important to enter your SSID and WIFI password in the
section: `Plant Watering Wifi Configuration`.

Once that is done you can build the application as usual:
```
$ idf.py build
```

Connect your esp32 and flash the application:
```
$ idf.py flash
```

If you want to troubleshoot you can always monitor the serial port using: 
```
$ idf.py monitor
```

## Using ESP8266

<img src="esp8266/plant-watering-system_bb.png" width="800">

### Building

A very very basic example of a plant watering system. Just need to open the sketch in the Arduino IDE. 

Notes:
* The current example code doesn't actually use the full potential of the ESP8266, i.e. does not use WiFi. Could have used another Arduino compatible board as well, but in this case it was chosen because of its compact size. 
* Do not power the ESP from both the USB and the power source at the same time! Disconnect the wires powering the ESP from the power supply while programming the ESP via USB. (Read more about ESP power supply: https://techexplorations.com/guides/esp32/begin/power/)

## Pictures

The "plant watering backpack":

<img src="esp8266/pics/IMG_0470.png" width="600">

<img src="esp8266/pics/IMG_0473.png" width="600">

See more pics in `esp8266/pics/`

Based on initial experiments, it seems that the soil moisture sensor gets degraded over time, so might need to have some extras at hand, and change them over time.

<img src="esp8266/pics/IMG_0474.png" width="600">
