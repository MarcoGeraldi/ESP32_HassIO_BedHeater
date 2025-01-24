#include "main.h"
#include "../lib/ESP32_HassIO/lib/arduino_tinyCLI/src/tinyCLI.cpp"

// Create the list of commands
tinyCLI::Command commands[] = {
    {CLI_HELP, "Shows this help message", nullptr}, // Placeholder for help function
    {CLI_RESET, "Reset the device", cli_reset},
    {CLI_MQTT_INFO, "Prints stored mqtt settings", cli_mqtt_info},
    {CLI_MQTT_SERVER, "Set MQTT broker IP address", cli_mqtt_server},
    {CLI_MQTT_PORT, "Set MQTT broker port", cli_mqtt_port},
    {CLI_MQTT_USER, "Set MQTT username", cli_mqtt_user},
    {CLI_MQTT_PASSWORD, "Set MQTT password", cli_mqtt_password},
    {CLI_WIFI_CONFIG_PORTAL, "Enable Config Portal", cli_config_portal},
    {CLI_WIFI_SSID, "Set WiFi SSID", nullptr},
    {CLI_WIFI_PSWD, "Set WiFi Password", nullptr},
};

// Initialize the command line interface
tinyCLI commandLine(Serial, commands, sizeof(commands) / sizeof(commands[0]));

void deviceUpdate()
{
  /* -------------------------- update device status -------------------------- */
  if (eth_mqttClient.connected())
  {
    myIoTdevice.update(eth_mqttClient);
  }
  else if (wifi_mqttClient.connected())
  {
    myIoTdevice.update(wifi_mqttClient);
  }
  else
  {
    // Serial.println("Client disconnected");
  }

  // Update the OLED display
  updateDisplay(thermo.temperature(RNOMINAL, RREF), atoi(TemperatureSetpoint->getStatus().c_str()), Heater->getStatusBool(), errorCode);
}

/* ------------------- Increment Timer to schedule events ------------------- */
void IRAM_ATTR onTimer()
{
  timerCounter++;
}

void setup()
{
  /* --------------------- Initialize Serial Communication -------------------- */
  Serial.begin(115200);

  while (!Serial)
    ;

  Serial.print("\nStarting MQTTClient_Basic on " + String(ARDUINO_BOARD));
  Serial.println(" with " + String(SHIELD_TYPE));
  Serial.println(WEBSERVER_WT32_ETH01_VERSION);

#ifdef ETHERNET_ENABLE
  /* --------------------------- initialize Ethernet -------------------------- */
  // To be called before ETH.begin()
  WT32_ETH01_onEvent();

  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

  WT32_ETH01_waitForConnect();
#endif

  /* -------------------------- Initialize Parameters ------------------------- */
  preferences.begin("my-app", false);

  /* ------------------------ Initiliaze WiFi Settings ------------------------ */
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  /* --------------------- Initialize WiFi Manager Object --------------------- */
  wm_init(false);

  /* ----------------------- Initialize MQTT Parameters ----------------------- */
  MQTT_init();

  /* -------------------------- Initialize IoT Device ------------------------- */
  IoT_device_init();

  /* ---------------------- Initialize Temperature Sensor --------------------- */
  if (!temp_init())
    errorCode = E01_SENSOR_ERROR;

  /* -------------------------- initialize gpio pins -------------------------- */
  gpio_init();

  /* --------------------------- Initialize Display --------------------------- */
  if (!displayInit())
    errorCode = E02_DISPLAY_ERROR;

  /* ------------------------------- Start Timer ------------------------------ */
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // 10ms timer
  timerAlarmEnable(timer);

  // Allow the hardware to sort itself out
  delay(1500);
}

void loop()
{

  unsigned long currentMillis = millis();

  /* ----------------------- Process Command line inputs ---------------------- */
  commandLine.processInput();

  /* -------------------------- WiFi manager Process -------------------------- */
  wm.process();

  /* ------------------------- Connect to MQTT Broker ------------------------- */

  if (WT32_ETH01_isConnected())
  {
    if (!eth_mqttClient.connected())
    {
      if (timerCounter % (5000 / 10) == 0)
      {
        Serial.println("Trying to reconnect via Ethernet..");

        if (!MQTT_reconnect(eth_mqttClient))
          errorCode = E04_COMM_ERROR;

        if (eth_mqttClient.connected())
          wifi_mqttClient.disconnect();
      }
    }
    else
    {
      // Nothing to do
    }
  }
  else if (WL_CONNECTED == WiFi.status())
  {
    /* --------------------- Try to reconnect to MQTT Broker -------------------- */
    if (!wifi_mqttClient.connected())
    {
      if (timerCounter % (5000 / 10) == 0)
      {

        Serial.println("Trying to reconnect via WiFi..");

        if (!MQTT_reconnect(wifi_mqttClient))
          errorCode = E04_COMM_ERROR;

        if (wifi_mqttClient.connected())
          eth_mqttClient.disconnect();
      }
    }
    else
    {
      // Nothing to do
    }
  }

  /* -------------------------------------------------------------------------- */
  /*                                 IoT Device                                 */
  /* -------------------------------------------------------------------------- */

  /* ------------------------- check for any button presses event ------------------------ */
  for (int i = 0; i < numButtons; i++)
  {
    handleButtonState(i, millis());

    if (singlePresses[i] || doublePresses[i] || longPresses[i] || longReleases[i])
    {
      int actualSetpoint = atoi(TemperatureSetpoint->getStatus().c_str());
      int actualSetpointModule5 = actualSetpoint % 5;

      /* ------------------ Up button pressed - Increase Setpoint ----------------- */
      if (0 == i && singlePresses[i] || doublePresses[i])
      {
        if (0 == actualSetpointModule5)
          actualSetpoint += 5; // setpoint is already a multiple of 5
        else
          actualSetpoint += (5 - actualSetpointModule5); // set to the next multiple of 5
      }

      /* ----------------- DOWN Button Pressed - Decrease Setpoint ---------------- */
      if (1 == i && singlePresses[i] || doublePresses[i])
      {
        if (0 == actualSetpointModule5)
          actualSetpoint -= 5; // setpoint is already a multiple of 5
        else
          actualSetpoint -= actualSetpointModule5; // set to the next multiple of 5
      }

      /* ------------------------- CNTR Button Long Press ------------------------- */
      if (2 == i && longPresses[i])
      {
        Serial.println(Heater->getStatus() + "Payload On:" + Heater->getPayloadOn() + "Payload Off:" + Heater->getPayloadOff());

        if (Heater->getStatus() == Heater->getPayloadOn())
        {
          Heater->setStatus(false); // disable heater
        }
        else
        {
          Heater->setStatus(true); // enable heater
        }
      }

      /* ----------------------------- Limit Setpoint ----------------------------- */
      actualSetpoint = constrain(actualSetpoint, SETPOINT_MIN, SETPOINT_MAX);

      // Check if setpoint has changed
      if (actualSetpoint != atoi(TemperatureSetpoint->getStatus().c_str()))
      {
        /* ------------------------- Update Setpoint Entity ------------------------- */
        TemperatureSetpoint->setStatus(actualSetpoint); // update setpoint

        /* -------------------------- Send update to HassIO ------------------------- */
        deviceUpdate();
      }
    }

    /* ------------------------------- Reset Flags ------------------------------ */
    singlePresses[i] = false;
    doublePresses[i] = false;
    longPresses[i] = false;
    longReleases[i] = false;
  }

  /* --------------------------------- 5s Task -------------------------------- */
  if (timerCounter % (1000 / 2) == 0)
  {
    /* -------------------------- Send update to HassIO ------------------------- */
    deviceUpdate();
  }

  /* --------------------------------- 1s Task -------------------------------- */
  if (timerCounter % (1000 / 10) == 0)
  {
    /* -------------------- Verify Temperature Sensore Faults ------------------- */
    if (!verifySensor())
      errorCode = E01_SENSOR_ERROR;
    else{
      /* ------------------------ Update Temperature entity ----------------------- */
      TemperatureSensor->setStatus(thermo.temperature(RNOMINAL, RREF));
    }     
   
    /* ----------------------------- Update Display ----------------------------- */
    updateDisplay(thermo.temperature(RNOMINAL, RREF), atoi(TemperatureSetpoint->getStatus().c_str()), Heater->getStatusBool(), errorCode);

  }  

  /* ----------------------------- Control Heater ----------------------------- */
  if ((errorCode == E00_NO_ERROR || errorCode == E04_COMM_ERROR) &&
      Heater->getStatus() == Heater->getPayloadOn() &&
      (float)atof(TemperatureSensor->getStatus().c_str()) < ((float)atof(TemperatureSetpoint->getStatus().c_str()) - HISTERESIS))
  {
    digitalWrite(SSR_PIN, HIGH); // Enable Heater
  } 
  else if (
      (errorCode == E00_NO_ERROR && errorCode == E04_COMM_ERROR) ||
      Heater->getStatus() == Heater->getPayloadOff() ||
      (float)atof(TemperatureSensor->getStatus().c_str()) > ((float)atof(TemperatureSetpoint->getStatus().c_str()) + HISTERESIS))
  {
    digitalWrite(SSR_PIN, LOW); // Disable Heater
  }

  else if (errorCode != E00_NO_ERROR && errorCode != E04_COMM_ERROR){
    digitalWrite(SSR_PIN, LOW); // Disable Heater;
  } 
  
  else {
    //do nothing
  }

  /* -------------------------------------------------------------------------- */
  /* -------------------------------------------------------------------------- */
  /* -------------------------------------------------------------------------- */

  /* -------------------------- Client loop functions ------------------------- */
  eth_mqttClient.loop();
  wifi_mqttClient.loop();




}

void IoT_device_init()
{
  /* --------------------------- Configure entities --------------------------- */
  TemperatureSetpoint->setMin(SETPOINT_MIN);
  TemperatureSetpoint->setMax(SETPOINT_MAX);

  /* ---------------- add all entities to the iot device object --------------- */
  myIoTdevice.addEntity(TemperatureSensor);
  myIoTdevice.addEntity(TemperatureSetpoint);
  myIoTdevice.addEntity(Heater);

  Heater->setStatus(false);

  Serial.println("Device Init");

  /* -------------------------- Configure IoT Device -------------------------- */
  if (eth_mqttClient.connected())
    myIoTdevice.configure(eth_mqttClient); // Update device configuration
  else if (wifi_mqttClient.connected())
    myIoTdevice.configure(wifi_mqttClient); // Update device configuration
  else
    Serial.println("Failed to send Device Configuration via MQTT. Client disconnected.");
}

void MQTT_callback(char *topic, byte *message, unsigned int length)
{

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  Serial.println();

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == TemperatureSetpoint->getCommandTopic())
  {
    TemperatureSetpoint->setStatus(messageTemp);
  }

  if (String(topic) == Heater->getCommandTopic())
  {
    if (messageTemp == Heater->getPayloadOn())
    {
      Heater->setStatus(true);
    }
    else
    {
      Heater->setStatus(false);
    }
  }

  /* -------------------------- update device status -------------------------- */
  deviceUpdate();
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

void saveConfigCallback()
{
  /* --------------------------- Store MQTT settings -------------------------- */
  preferences.putString(PREF_MQTT_SERVER, custom_mqtt_server.getValue());
  preferences.putString(PREF_MQTT_PORT, custom_mqtt_port.getValue());
  preferences.putString(PREF_MQTT_USER, custom_mqtt_user.getValue());
  preferences.putString(PREF_MQTT_PASSWORD, custom_mqtt_password.getValue());
}

void MQTT_init()
{

  /* ----------------------- Initialize MQTT Parameters ----------------------- */
  String _mqtt_port = preferences.getString(PREF_MQTT_PORT, mqtt_port);
  String _mqtt_server = preferences.getString(PREF_MQTT_SERVER, mqtt_server);
  String _mqtt_user = preferences.getString(PREF_MQTT_USER, mqtt_user);
  String _mqtt_password = preferences.getString(PREF_MQTT_PASSWORD, mqtt_password);

  /* ---------------------- Remove whitespaces at the end --------------------- */
  _mqtt_user.trim();
  _mqtt_password.trim();
  _mqtt_port.trim();
  _mqtt_server.trim();

  custom_mqtt_server.setValue(_mqtt_server.c_str(), 128);
  custom_mqtt_port.setValue(_mqtt_port.c_str(), 128);
  custom_mqtt_user.setValue(_mqtt_user.c_str(), 128);
  custom_mqtt_password.setValue(_mqtt_password.c_str(), 128);

  wifi_mqttClient.setServer(custom_mqtt_server.getValue(), 1883);
  wifi_mqttClient.setCallback(MQTT_callback);
  eth_mqttClient.setServer(custom_mqtt_server.getValue(), 1883);
  eth_mqttClient.setCallback(MQTT_callback);
}

void wm_init(bool _reset)
{

  if (_reset)
    wm.resetSettings(); // reset settings - wipe credentials for testing

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_password);

  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);

  // automatically connect using saved credentials if they exist
  // If connection fails it starts an access point with the specified name
  if (wm.autoConnect("AutoConnectAP"))
  {
    Serial.println("connected...yeey :)");
  }
  else
  {
    Serial.println("Failed to connect");
  }

  // set config save notify callback
  wm.setSaveParamsCallback(saveConfigCallback);
}

error_t MQTT_reconnect(PubSubClient &_client)
{

  /* ------------------------ Retrieve data from memory ----------------------- */
  String _mqtt_user = preferences.getString(PREF_MQTT_USER, mqtt_user);
  String _mqtt_password = preferences.getString(PREF_MQTT_PASSWORD, mqtt_password);
  String _mqtt_port = preferences.getString(PREF_MQTT_PORT, mqtt_port);
  String _mqtt_server = preferences.getString(PREF_MQTT_SERVER, mqtt_server);

  /* ---------------------- Remove whitespaces at the end --------------------- */
  _mqtt_user.trim();
  _mqtt_password.trim();
  _mqtt_port.trim();
  _mqtt_server.trim();

  if (!_client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    _client.setServer(_mqtt_server.c_str(), _mqtt_port.toInt());

    if (_client.connect(myIoTdevice.getSerialNumber().c_str(), _mqtt_user.c_str(), _mqtt_password.c_str()))
    {
      Serial.println("connected");

      /* --------------------- Update IoT Device configuration -------------------- */
      myIoTdevice.configure(_client); // Update device configuration
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(_client.state());
      return false;
    }
  }

  return true;
}

int randomInt()
{
  return rand();
}

bool randomBool()
{
  return rand() % 2;
}
