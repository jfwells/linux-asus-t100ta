#!/bin/bash

sensitivity=20
min=50
max=255
sensormax=800
delay=5

cpdevnode=`ls /sys/bus/i2c/devices/i2c-CPLM3218\:00/ | grep iio `
cpdevinput="/sys/bus/i2c/devices/i2c-CPLM3218:00/$cpdevnode/in_illuminance_input"
blnode="/sys/class/backlight/intel_backlight/brightness"

echo $max > "$blnode"


while [ 1 ]             
do                      
        updated=1
        while [ $updated -gt 0 ]
        do      
                updated=0
                backlight=$(cat "$blnode")
                sensor=$(cat "$cpdevinput")

		if [ $sensor -gt $sensormax ]; then
			sensor=$sensormax
		fi
		sens=$((sensor*(sensormax/(max-min))))

                target=$backlight
                        
                if [ $sens -gt $((backlight+sensitivity)) ]
                then    
                        updated=1
                        target=$((target+(sensitivity/2)))
                fi      
                
                if [ $sens -lt $((backlight-sensitivity)) ]
                then    
                        updated=1
                        target=$((target-(sensitivity/2)))
                fi
                
                if [ $target -gt $max ]
                then
                        target=$max
			updated=0                                   
                fi

                if [ $target -lt $min ]
                then
                        target=$min  
			updated=0                                      
                fi
                                                         

                if [ $updated -gt 0 ]
                then                                
			echo "Setting bl to $target"                         
                        echo $target > "$blnode"
                fi
        done
        sleep $delay
done
