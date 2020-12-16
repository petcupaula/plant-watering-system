# plant-watering-system

<img src="plant-watering-system_bb.png" width="800">

# Building

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




