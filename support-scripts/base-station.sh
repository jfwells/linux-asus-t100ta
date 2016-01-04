#!/bin/sh

if [ -d /dev/input/by-id ]; then
	BSTATE="DOCKED"
else
	BSTATE="UNDOCKED"
fi

while true
do
	if [ -d /dev/input/by-id ]; then
		if [ $BSTATE = "UNDOCKED" ]; then
			BSTATE="DOCKED"
			docked.sh
		fi

	else
		if [ $BSTATE = "DOCKED" ]; then
			BSTATE="UNDOCKED"
			undocked.sh
		fi
	fi

    sleep 3
done
