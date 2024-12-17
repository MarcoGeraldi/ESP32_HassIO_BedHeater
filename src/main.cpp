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
    Serial.println("Client disconnected");
  }
}

/* ------------------- Increment Timer to schedule events ------------------- */
void IRAM_ATTR onTimer() {
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
  thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  
  /* --------------------------- Initialize Display --------------------------- */
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  /* ------------------------------- Start Timer ------------------------------ */
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // 10ms timer
  timerAlarmEnable(timer);

  // Clear the buffer
  display.clearDisplay();

  // Allow the hardware to sort itself out
  delay(1500);
}

// Text size parameters
#define SET_LABEL_TEXT_SIZE 1      // Text size for "SET:"
#define TEMP_TEXT_SIZE 4           // Text size for the temperature reading
#define SETPOINT_TEXT_SIZE 2       // Text size for the setpoint value


// Simulated variables
bool relayOn = true;       // Relay state (true = on, false = off)

void updateDisplay(double _temperature, int _setpoint) {
  // Clear the previous display contents
  display.clearDisplay();

  // Draw a rounded border around the screen
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10, SSD1306_WHITE);

  // Draw a horizontal line to separate temperature and setpoint sections (lowered position)
  display.drawLine(0, SCREEN_HEIGHT / 2 + 10, SCREEN_WIDTH, SCREEN_HEIGHT / 2 + 10, SSD1306_WHITE);

  // Display the temperature centered with padding above it and larger text
  display.setTextSize(TEMP_TEXT_SIZE);  // Larger text size for temperature
  display.setTextColor(SSD1306_WHITE);

  // Calculate the width of the text to properly center it
  String tempString = String(_temperature, 1);  // Convert temperature to string with 1 decimal place
  int textWidth = tempString.length() * 6 * TEMP_TEXT_SIZE;  // Each character width is approximately 6px in size
  int tempX = 8; //(SCREEN_WIDTH - textWidth) / 2;  // Center horizontally based on text width
  int tempY = (SCREEN_HEIGHT / 2) - 25;        // Adjust vertical position slightly higher
  display.setCursor(tempX, tempY);
  display.print(_temperature, 1);  // Print temperature with 1 decimal place
  
  // Add "°C" after the temperature 
  display.drawCircle(108, 8, 3, SSD1306_WHITE);  // Draw a small circle (3px radius)
  display.setTextSize(2);  // Medium text size for the unit
  display.setCursor(113, 8);  // Position after the circle
  display.print("C");  // Print the unit (Celsius)

  // Display "SET:" in the bottom left corner with smaller text size
  display.setTextSize(SET_LABEL_TEXT_SIZE);  // Text size for "SET:"
  display.setCursor(5, SCREEN_HEIGHT - 18);  // Position at the bottom-left
  display.print("SET:");

  // Display the setpoint in the bottom left corner, below the horizontal line with larger text
  display.setTextSize(SETPOINT_TEXT_SIZE);  // Larger text size for setpoint
  display.setCursor(35, SCREEN_HEIGHT / 2 + 15);  // Position just below the line
  display.print(_setpoint);  // Display setpoint as an integer

  // Add "°C" after the setpoint
  display.drawCircle(65, SCREEN_HEIGHT / 2 + 15, 3, SSD1306_WHITE);  // Draw a small circle (3px radius)

  // Add "C" after the circle to indicate Celsius
  display.setTextSize(2);  // Medium text size for the unit
  display.setCursor(70, SCREEN_HEIGHT / 2 + 15);  // Position after the circle
  display.print("C");  // Print the unit (Celsius)


  // Display relay state (white dot or circle in bottom right)
  if (relayOn) {
    display.fillCircle(SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 5, SSD1306_WHITE);  // Glowing dot effect
  } else {
    display.drawCircle(SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10, 5, SSD1306_WHITE);  // Hollow circle
  }

  // Update the display
  display.display();
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
      if (timerCounter % (5000/10) == 0)
      {
        Serial.println("Trying to reconnect via Ethernet..");
        MQTT_reconnect(eth_mqttClient);

        if (eth_mqttClient.connected())
          wifi_mqttClient.disconnect();
      }
    }
    else
    {
      //Nothing to do 
    }
  }
  else if (WL_CONNECTED == WiFi.status())
  {
    /* --------------------- Try to reconnect to MQTT Broker -------------------- */
    if (!wifi_mqttClient.connected())
    {
      if (timerCounter % (5000/10) == 0)
      {
        Serial.println("Trying to reconnect via WiFi..");
        MQTT_reconnect(wifi_mqttClient);

        if (wifi_mqttClient.connected())
          eth_mqttClient.disconnect();
      }
    }
    else
    {
      //Nothing to do 
    }
  }

  if (timerCounter % (1000/10) == 0)  { //1000ms / 10ms

    TemperatureSensor->setStatus(thermo.temperature(RNOMINAL, RREF));
    
    deviceUpdate();
    
    // Update the OLED display
    updateDisplay(atof(TemperatureSensor->getStatus().c_str()), 25);

    relayOn = !relayOn;  // Toggle relay state

  }

  eth_mqttClient.loop();
  wifi_mqttClient.loop();
}

void IoT_device_init()
{

  /* ---------------- add all entities to the iot device object --------------- */
  myIoTdevice.addEntity(TemperatureSensor);



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
  /*if (String(topic) == mySwitch->getCommandTopic())
  {

    if (messageTemp == mySwitch->getPayloadOn())
    {
      digitalWrite(LED, HIGH);
      myAction->setStatus("led_on");
    }
    else if (mySwitch->getPayloadOff())
    {
      digitalWrite(LED, LOW);
      myAction->setStatus("led_off");
    }
  }

  if (String(topic) == myLight->getCommandTopic())
  {
    myLight->setStatus(messageTemp);
  }*/

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

void MQTT_reconnect(PubSubClient &_client)
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
    }
  }
}

int randomInt()
{
  return rand();
}

bool randomBool()
{
  return rand() % 2;
}

