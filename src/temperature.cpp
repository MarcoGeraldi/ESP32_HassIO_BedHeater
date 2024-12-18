#include "temperature.h"

Adafruit_MAX31865 thermo  = Adafruit_MAX31865(27, 14, 12, 13);

void temp_init(){
    thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
}