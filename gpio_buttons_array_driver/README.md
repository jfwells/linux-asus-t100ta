Buttons array driver for PNP0C40 ACPI Device
============================================

Work in progress, not yet ready for sharing/testing.

To get the device recognized and have this loaded, you also need to add the device to the acpi_platform_device_ids list defined in
drivers/acpi/acpi_platform.c

Todo
==== 
- gpiod_get_index() not working -- failing because acpi_get_gpiod() isn't returning a descriptor
-- Then test if gpio-keys is right approach (may not be necessary)
--- Don't use single global to store gpio-keys platform device, allow for multiple
--- Unload gpio-keys platform device on unload

Notes
=====

acpi_get_gpiod() probably broken because not actually tested (!) See http://www.kernelhub.org/?msg=419974&p=2



Testing
======

make
sudo insmod gpio_button_array.ko
