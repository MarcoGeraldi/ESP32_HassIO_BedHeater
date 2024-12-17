#ifndef _MAIN_H
#define _MAIN_H

/* -------------------------------------------------------------------------- */
/*                                Include Files                               */
/* -------------------------------------------------------------------------- */
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Preferences.h>
#include <PubSubClient.h>
#include "definitions.h"
#include "iot_cli.h"
#include "IoT_device.h"
#include <WebServer_WT32_ETH01.h>

/* ----------------------- Temperature Sensor Library ----------------------- */
#include <Wire.h>
#include <Adafruit_MAX31865.h>

/* ------------------------ I2C OLED Display Library ------------------------ */
#include <Adafruit_SSD1306.h>

/* -------------------------------------------------------------------------- */
/*                                   Macros                                   */
/* -------------------------------------------------------------------------- */
#define DEBUG_ETHERNET_WEBSERVER_PORT Serial
#define _ETHERNET_WEBSERVER_LOGLEVEL_ 3

/* --------------------------- Temperature Sensor --------------------------- */
#define RREF      430.0 // The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RNOMINAL  100.0 // The 'nominal' 0-degrees-C resistance of the sensor 100.0 for PT100, 1000.0 for PT1000

/* ----------------------------- Display Macros ----------------------------- */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


/* -------------------------------------------------------------------------- */
/*                                Enumerations                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
unsigned long timestamp = 0;

/* ----------------------- MQTT client default values ----------------------- */
char mqtt_server[40];
char mqtt_port[6] = "";
char mqtt_user[34] = "";
char mqtt_password[34] = "";

/* -----------------------  WiFiManager MQTT Definition ---------------------- */
WiFiManager wm;
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT Username", mqtt_user, 32);
WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", mqtt_password, 32);

/* -------------- Preference Object to store data in the memory ------------- */
Preferences preferences;

/* ------------------------------- MQTT Client ------------------------------ */
WiFiClient    espClient;
PubSubClient  wifi_mqttClient(espClient);
WiFiClient    ethClient;
PubSubClient  eth_mqttClient(ethClient);

/* ------------------------------ Timer Handler ----------------------------- */
hw_timer_t *timer = NULL;
unsigned long timerCounter = 0;

/* --------------------------- Temperature Sensor --------------------------- */
Adafruit_MAX31865 thermo  = Adafruit_MAX31865(27, 14, 12, 13);




/* ------------------------------ OLED Display ------------------------------ */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ------------------------------- IoT Device ------------------------------- */
Device myIoTdevice;

auto TemperatureSensor = std::make_shared<Sensor>("Temperature", _HASSIO_DEVICE_CLASS_SENSOR_TEMPERATURE);
 


/* -------------------------------------------------------------------------- */
/*                               Data Structures                              */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Typedefs                                  */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

void wm_init(bool _reset);
void saveConfigCallback();

void MQTT_init();
void MQTT_callback(char *topic, byte *message, unsigned int length);
void MQTT_reconnect(PubSubClient &_client);

void IoT_device_init();
int randomInt();
bool randomBool();


#endif