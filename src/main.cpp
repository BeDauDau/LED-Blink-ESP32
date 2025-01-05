#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Pin definitions
const int LIGHT_SENSOR_PIN = 36; // Light sensor analog input
const int LED_PIN = 2;           // Built-in LED
const int BUTTON_PIN = 4;        // Manual control button

// WiFi credentials
const char *ssid = "WiFiSSID";
const char *password = "WiFiPassword";

// MQTT Broker settings
const char *mqtt_server = "MQTTBroker";
const int mqtt_port = 1883;
const char *mqtt_topic = "smart_light/control";

// Operating parameters
const int LIGHT_THRESHOLD = 2000;              // Light sensor threshold
const int BLINK_INTERVAL = 1000;               // Blink interval in ms
const unsigned long AUTO_REVERT_TIME = 300000; // 5 minutes

// System states
enum Mode
{
    AUTO,
    MANUAL
};
Mode currentMode = AUTO;
bool ledState = false;
unsigned long lastBlinkTime = 0;
unsigned long lastModeChangeTime = 0;
bool buttonState = false;
bool lastButtonState = false;

// Objects
WiFiClient espClient;
PubSubClient mqtt(espClient);

void setupWiFi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    if (message == "AUTO")
    {
        currentMode = AUTO;
    }
    else if (message == "MANUAL")
    {
        currentMode = MANUAL;
        lastModeChangeTime = millis();
    }
    else if (message == "TOGGLE" && currentMode == MANUAL)
    {
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }
}

void reconnectMQTT()
{
    while (!mqtt.connected())
    {
        if (mqtt.connect("ESP32Client"))
        {
            mqtt.subscribe(mqtt_topic);
        }
        else
        {
            delay(5000);
        }
    }
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LIGHT_SENSOR_PIN, INPUT);

    setupWiFi();
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(mqttCallback);
}

void handleAutoMode()
{
    int lightLevel = analogRead(LIGHT_SENSOR_PIN);

    if (lightLevel > LIGHT_THRESHOLD)
    { // Daytime
        digitalWrite(LED_PIN, LOW);
        ledState = false;
    }
    else
    { // Nighttime
        if (millis() - lastBlinkTime >= BLINK_INTERVAL)
        {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
            lastBlinkTime = millis();
        }
    }
}

void handleManualMode()
{
    // Check for auto-revert timeout
    if (millis() - lastModeChangeTime >= AUTO_REVERT_TIME)
    {
        currentMode = AUTO;
        return;
    }

    // Handle button press
    buttonState = !digitalRead(BUTTON_PIN); // Inverted because of pull-up
    if (buttonState != lastButtonState)
    {
        if (buttonState)
        { // Button pressed
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
        lastButtonState = buttonState;
    }
}

void loop()
{
    if (!mqtt.connected())
    {
        reconnectMQTT();
    }
    mqtt.loop();

    // Mode handling
    if (currentMode == AUTO)
    {
        handleAutoMode();
    }
    else
    {
        handleManualMode();
    }

    // Publish status periodically (you can add this if needed)
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish >= 5000)
    {
        String status = String("Mode:") + (currentMode == AUTO ? "AUTO" : "MANUAL") +
                        ",LED:" + (ledState ? "ON" : "OFF");
        mqtt.publish("smart_light/status", status.c_str());
        lastPublish = millis();
    }
}