ACPI-enabled driver for CM3281 ambient light sensor
===================================================

Taken from Capella's [original draft submission](https://lwn.net/Articles/557810/)

Largely cleaned up, modified to detect ACPI device.

Module depends on 'industrialio' module ('make install' will handle dependencies for you).


Testing / Installing:
====================

Building the module
1. Download all the files in this folder (or 'git clone' this repository)
2. cd <download_location>/cm3218_ambient_light_sensor_driver
3. make

Installing the module:
4. sudo make install

Loading the module:
5. sudo modprobe cm3218

Testing the module:
6. watch cat /sys/bus/iio/devices/iio\:device0/in_illuminance0_input
7. Move your tablet around under different light sources and watch the numbers change



Todo
==== 
- Enable time integration base modification






Testing
======

make

sudo insmod gpio_button_array.ko
