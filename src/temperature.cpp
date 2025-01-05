#include "temperature.h"

Adafruit_MAX31865 thermo = Adafruit_MAX31865(27, 14, 12, 13);

error_t temp_init()
{
    if (!thermo.begin(MAX31865_3WIRE))
        return false; // set to 2WIRE or 4WIRE as necessary

    return true;
}

error_t verifySensor()
{
    float ratio = thermo.readRTD();
    ratio /= 32768;

    // Check and print any faults
    uint8_t fault = thermo.readFault();
    if (fault)
    {
        Serial.print("Fault 0x");
        Serial.println(fault, HEX);
        if (fault & MAX31865_FAULT_HIGHTHRESH)
        {
            Serial.println("RTD High Threshold");
            return false;
        }
        if (fault & MAX31865_FAULT_LOWTHRESH)
        {
            Serial.println("RTD Low Threshold");
            return false;
        }
        if (fault & MAX31865_FAULT_REFINLOW)
        {
            Serial.println("REFIN- > 0.85 x Bias");
            return false;
        }
        if (fault & MAX31865_FAULT_REFINHIGH)
        {
            Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
            return false;
        }
        if (fault & MAX31865_FAULT_RTDINLOW)
        {
            Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
            return false;
        }
        if (fault & MAX31865_FAULT_OVUV)
        {
            Serial.println("Under/Over voltage");
            return false;
        }
        thermo.clearFault();
    }

    return true;
}