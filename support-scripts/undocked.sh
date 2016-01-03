#!/bin/sh
notify-send -i notification-device-eject "Tablet undocked"
dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Hide
dconf write /org/onboard/auto-show/enabled true
