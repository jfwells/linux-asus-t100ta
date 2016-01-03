#!/bin/sh
notify-send -i notification-gpm-keyboard-100 "Tablet docked"
dconf write /org/onboard/auto-show/enabled false
dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Hide
