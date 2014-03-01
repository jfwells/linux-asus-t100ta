#!/bin/bash

TOUCHSCREEN=`xinput | grep ATML1000 | cut  -f2 | cut -d= -f2`
CURR_ROT=`xrandr -q --verbose | grep VGA | cut -d" " -f 6`

if [ "$CURR_ROT" == "normal" ]; then
	ROT_TO="right"
	SWAP_TO="1"
	INV_TO="0, 1"
elif [ "$CURR_ROT" == "right" ]; then
	ROT_TO="inverted"
	SWAP_TO="0"
	INV_TO="1, 1"
elif [ "$CURR_ROT" == "inverted" ]; then
	ROT_TO="left"
	SWAP_TO="1"
	INV_TO="1, 0"
else 
	ROT_TO="normal"
	SWAP_TO="0"
	INV_TO="0, 0"
fi

xrandr --screen 0 -o $ROT_TO
xinput set-int-prop $TOUCHSCREEN "Evdev Axis Inversion" 8 $INV_TO
xinput set-int-prop $TOUCHSCREEN "Evdev Axes Swap" 8 $SWAP_TO 
