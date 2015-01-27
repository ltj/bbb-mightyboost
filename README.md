bbb-mightyboost
===============

Stuff for the BeagleBone Black to work with MightyBoost from [LowPowerLab](http://lowpowerlab.com/)

The **BeagleBone** folder contains all things related to the BBB (not tested on original BB):
* DeviceTree: The DT overlay needed to properly set up the GPIO pins. Either move the MightyBoost-GPIO.dts to the BBB and compile it using ```dtc -O dtb -o MightyBost-GPIO-00A0.dtbo -b 0 -@ MightyBost-GPIO.dts``` or copy the already compiled .dtbo to /lib/firmware/
* scripts: The ioenable.sh is a simple bash script that loads the overlay and exports the pins. It also sets the BootOK pin high. This script must be called at boot time.
* src: This folder includes the source to a simple C++ program that checks the shutdown signal pin, and if it goes high, it will power down the BBB. You may need to change the command used for shutdown depending on the distro (Arch Linux here). Compile the program using ```g++ -O2 -Wall CheckShutdown.cpp SimpleGPIO.cpp -o CheckShutDown```. The program should be startet at boot time *after* the bash script has setup gpio.

The **Moteino** folder contains a sketch that sort of combines the Moteino MightyBoost controller and Gateway sketches.
