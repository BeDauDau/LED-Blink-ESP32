// #include <Arduino.h>
// #include <WiFi.h>
// #include <PubSubClient.h>

// // Pin definitions
// const int LIGHT_SENSOR_PIN = 36; // Light sensor analog input
// const int LED_PIN = 2;           // Built-in LED
// const int BUTTON_PIN = 4;        // Manual control button

// // WiFi credentials
// const char *ssid = "WiFiSSID";
// const char *password = "WiFiPassword";

// // MQTT Broker settings
// const char *mqtt_server = "MQTTBroker";
// const int mqtt_port = 1883;
// const char *mqtt_topic = "smart_light/control";

// // Operating parameters
// const int LIGHT_THRESHOLD = 2000;              // Light sensor threshold
// const int BLINK_INTERVAL = 1000;               // Blink interval in ms
// const unsigned long AUTO_REVERT_TIME = 300000; // 5 minutes

// // System states
// enum Mode
// {
//     AUTO,
//     MANUAL
// };
// Mode currentMode = AUTO;
// bool ledState = false;
// unsigned long lastBlinkTime = 0;
// unsigned long lastModeChangeTime = 0;
// bool buttonState = false;
// bool lastButtonState = false;

// // Objects
// WiFiClient espClient;
// PubSubClient mqtt(espClient);

// void setupWiFi()
// {
//     WiFi.begin(ssid, password);
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//     }
// }

// void mqttCallback(char *topic, byte *payload, unsigned int length)
// {
//     String message = "";
//     for (unsigned int i = 0; i < length; i++)
//     {
//         message += (char)payload[i];
//     }

//     if (message == "AUTO")
//     {
//         currentMode = AUTO;
//     }
//     else if (message == "MANUAL")
//     {
//         currentMode = MANUAL;
//         lastModeChangeTime = millis();
//     }
//     else if (message == "TOGGLE" && currentMode == MANUAL)
//     {
//         ledState = !ledState;
//         digitalWrite(LED_PIN, ledState);
//     }
// }

// void reconnectMQTT()
// {
//     while (!mqtt.connected())
//     {
//         if (mqtt.connect("ESP32Client"))
//         {
//             mqtt.subscribe(mqtt_topic);
//         }
//         else
//         {
//             delay(5000);
//         }
//     }
// }

// void setup()
// {
//     pinMode(LED_PIN, OUTPUT);
//     pinMode(BUTTON_PIN, INPUT_PULLUP);
//     pinMode(LIGHT_SENSOR_PIN, INPUT);

//     setupWiFi();
//     mqtt.setServer(mqtt_server, mqtt_port);
//     mqtt.setCallback(mqttCallback);
// }

// void handleAutoMode()
// {
//     int lightLevel = analogRead(LIGHT_SENSOR_PIN);

//     if (lightLevel > LIGHT_THRESHOLD)
//     { // Daytime
//         digitalWrite(LED_PIN, LOW);
//         ledState = false;
//     }
//     else
//     { // Nighttime
//         if (millis() - lastBlinkTime >= BLINK_INTERVAL)
//         {
//             ledState = !ledState;
//             digitalWrite(LED_PIN, ledState);
//             lastBlinkTime = millis();
//         }
//     }
// }

// void handleManualMode()
// {
//     // Check for auto-revert timeout
//     if (millis() - lastModeChangeTime >= AUTO_REVERT_TIME)
//     {
//         currentMode = AUTO;
//         return;
//     }

//     // Handle button press
//     buttonState = !digitalRead(BUTTON_PIN); // Inverted because of pull-up
//     if (buttonState != lastButtonState)
//     {
//         if (buttonState)
//         { // Button pressed
//             ledState = !ledState;
//             digitalWrite(LED_PIN, ledState);
//         }
//         lastButtonState = buttonState;
//     }
// }

// void loop()
// {
//     if (!mqtt.connected())
//     {
//         reconnectMQTT();
//     }
//     mqtt.loop();

//     // Mode handling
//     if (currentMode == AUTO)
//     {
//         handleAutoMode();
//     }
//     else
//     {
//         handleManualMode();
//     }

//     // Publish status periodically (you can add this if needed)
//     static unsigned long lastPublish = 0;
//     if (millis() - lastPublish >= 5000)
//     {
//         String status = String("Mode:") + (currentMode == AUTO ? "AUTO" : "MANUAL") +
//                         ",LED:" + (ledState ? "ON" : "OFF");
//         mqtt.publish("smart_light/status", status.c_str());
//         lastPublish = millis();
//     }
// }

#include <WiFi.h>
#include <PubSubClient.h>
#include <secretKey.h>

// Configuration constants
constexpr char SSID[] = "SamSua 2G";
constexpr char PASSWORD[] = "samthui2014";
constexpr char MQTT_SERVER[] = "broker.hivemq.com";
constexpr char MQTT_STATUS_TOPIC[] = "home/light/status";
constexpr char MQTT_COMMAND_TOPIC[] = "home/light/command";

// Hardware configuration
constexpr uint8_t LDR_PIN = 34;
constexpr uint8_t LED_PIN = 2;
constexpr uint8_t BUTTON_PIN = 5;
constexpr uint16_t THRESHOLD = 500;
constexpr unsigned long MANUAL_TIMEOUT = 20000;
constexpr unsigned long STATUS_INTERVAL = 1000;
constexpr unsigned long BLINK_INTERVAL = 500;

// State variables
struct State
{
    bool isAutoMode = true;
    bool ledState = false;
    unsigned long manualStartTime = 0;
    String currentLedStatus = "Tắt";
} state;

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi()
{
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
}

void reconnectMQTT()
{
    while (!client.connected())
    {
        if (client.connect("ESP32Client"))
        {
            client.subscribe(MQTT_COMMAND_TOPIC);
            return;
        }
        delay(5000);
    }
}

void sendStatus()
{
    String status = "Mode: " + String(state.isAutoMode ? "Auto" : "Manual") +
                    ", LED: " + state.currentLedStatus;
    client.publish(MQTT_STATUS_TOPIC, status.c_str());
}

void enterManualMode(bool turnOn)
{
    state.isAutoMode = false;
    state.manualStartTime = millis();
    state.ledState = turnOn;
    digitalWrite(LED_PIN, state.ledState);
    state.currentLedStatus = turnOn ? "Sáng" : "Tắt";
    sendStatus();
}

void blinkLED()
{
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= BLINK_INTERVAL)
    {
        previousMillis = currentMillis;
        state.ledState = !state.ledState;
        digitalWrite(LED_PIN, state.ledState);
    }
}

void autoMode()
{
    int lightValue = 4095 - analogRead(LDR_PIN);
    if (lightValue < THRESHOLD)
    {
        blinkLED();
        state.currentLedStatus = "Nhấp nháy";
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
        state.currentLedStatus = "Tắt";
    }
}

void handleCommand(const String &command)
{
    if (command == "on")
        enterManualMode(true);
    else if (command == "off")
        enterManualMode(false);
    else if (command == "auto")
        state.isAutoMode = true;
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    handleCommand(String((char *)payload, length));
}

void setup()
{
    Serial.begin(9600);
    pinMode(LDR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    setupWiFi();
    client.setServer(MQTT_SERVER, 1883);
    client.setCallback(mqttCallback);
}

void loop()
{
    static unsigned long lastStatusTime = 0;

    if (!client.connected())
        reconnectMQTT();
    client.loop();

    // Button handling with debounce
    if (digitalRead(BUTTON_PIN) == LOW)
    {
        delay(50);
        if (digitalRead(BUTTON_PIN) == LOW)
        {
            state.isAutoMode = !state.isAutoMode;
            if (!state.isAutoMode)
                state.manualStartTime = millis();
            while (digitalRead(BUTTON_PIN) == LOW)
                ;
        }
    }

    // Mode handling
    if (!state.isAutoMode && millis() - state.manualStartTime >= MANUAL_TIMEOUT)
    {
        state.isAutoMode = true;
    }

    state.isAutoMode ? autoMode() : digitalWrite(LED_PIN, state.ledState);

    // Status update
    if (millis() - lastStatusTime >= STATUS_INTERVAL)
    {
        lastStatusTime = millis();
        sendStatus();
    }
}
