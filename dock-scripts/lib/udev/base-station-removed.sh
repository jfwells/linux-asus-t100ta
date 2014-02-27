#!/bin/sh

# Automatically disable the Onboard on-screen keyboard, and hide it, when the base station is plugged in

# This script will run as the current X desktop user

# John Wells, 2014


USER=`whoami`
export XAUTHORITY="/home/$USER/.Xauthority"
export DISPLAY=:0


PID=`pgrep -o -u $USER onboard`

ENVIRON=/proc/$PID/environ
if [ -e $ENVIRON ]
 then
export `grep -z DBUS_SESSION_BUS_ADDRESS $ENVIRON`
 else
echo "Unable to set DBUS_SESSION_BUS_ADDRESS."
exit 1
fi

echo "SENDING CLOSE EVENT"
dbus-send --type=method_call --dest=org.onboard.Onboard /org/onboard/Onboard/Keyboard org.onboard.Onboard.Keyboard.Hide


echo "ENABLING"
dconf write /org/onboard/auto-show/enabled true


