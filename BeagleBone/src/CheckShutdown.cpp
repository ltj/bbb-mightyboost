/*
 * CheckShutdown.cpp
 *
 * by Lars Toft Jacobsen (boxed.dk)
 * Shutdown signal checker for LowPowerLab MightyBoost and
 * BeagleBone Black.
 *
 * Requires appropritate device tree ovelays to be loaded and
 * the pin (HaltGPIO) to be exported.
 */

#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "SimpleGPIO.h"
using namespace std;

unsigned int HaltGPIO = 48;

int main(int argc, char *argv[]){

  cout << "Checking MightyBoost shutdown pin" << endl;
  unsigned int value = LOW;
  
  do {
    gpio_get_value(HaltGPIO, &value); 
    sleep(1);
    //usleep(500000);
  } while (value!=HIGH);
  cout << endl <<  "Shutdown signal received from MightyBoost!" << endl;
  system("/usr/bin/poweroff");
	
  return 0;
}

