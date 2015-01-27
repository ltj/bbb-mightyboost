#!/usr/bin/bash

# Use overlays
echo BB-UART4 > /sys/devices/bone_capemgr.*/slots
echo MightyBoost-GPIO > /sys/devices/bone_capemgr.*/slots

# Export GPIO pins
echo 60 > /sys/class/gpio/export
echo 48 > /sys/class/gpio/export

# Toggle the BOOTOK pin to signal
echo "out" > /sys/class/gpio/gpio60/direction
echo 1 > /sys/class/gpio/gpio60/value

