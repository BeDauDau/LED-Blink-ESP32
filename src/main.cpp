#include <WiFi.h>
#include <PubSubClient.h>
#include <secretKey.h>

// Hardware configuration
constexpr uint8_t LDR_PIN = 34;
constexpr uint8_t LED_PIN = 15;
constexpr uint8_t BUTTON_PIN = 5;
constexpr uint16_t THRESHOLD = 500;
constexpr unsigned long MANUAL_TIMEOUT = 2000000;
constexpr unsigned long STATUS_INTERVAL = 1000;
constexpr unsigned long BLINK_INTERVAL = 500;

// State variables
struct State
{
    bool isAutoMode = true;
    bool ledState = false;
    unsigned long manualStartTime = 10000;
    String currentLedStatus = "off";
} state;

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi()
{
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected successfully!");
    Serial.println("IP address: " + WiFi.localIP().toString());
}

// Modify reconnectMQTT function to include credentials
void reconnectMQTT()
{
    while (!client.connected())
    {
        if (client.connect("ESP32Client", MQTT_USERNAME, MQTT_PASSWORD))
        {
            client.subscribe(MQTT_COMMAND_TOPIC);
            return;
        }
        delay(5000);
    }
}

// Modify sendStatus function to send simpler messages matching chatbot expectations
void sendStatus()
{
    if (state.isAutoMode)
    {
        client.publish(MQTT_STATUS_TOPIC, "auto");
    }
    else
    {
        client.publish(MQTT_STATUS_TOPIC, state.ledState ? "on" : "off");
    }
}

void enterManualMode(bool turnOn)
{
    state.isAutoMode = false;
    state.manualStartTime = millis();
    state.ledState = turnOn;
    digitalWrite(LED_PIN, state.ledState);
    state.currentLedStatus = turnOn ? "light" : "off";
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
        state.currentLedStatus = "blink";
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
        state.currentLedStatus = "off";
    }
}

// Update handleCommand to handle the chatbot commands
void handleCommand(const String &command)
{
    if (command == "on")
        enterManualMode(true);
    else if (command == "off")
        enterManualMode(false);
    else if (command == "auto")
        state.isAutoMode = true;
    else if (command == "manual")
        state.isAutoMode = false;
    else if (command == "state")
        sendStatus();
}

// Update handleCommand to send status after changes
// void handleCommand(const String &command)
// {

//     if (command == "on")
//     {
//         enterManualMode(true);
//         sendStatus();
//     }
//     else if (command == "off")
//     {
//         enterManualMode(false);
//         sendStatus();
//     }
//     else if (command == "auto")
//     {
//         state.isAutoMode = true;
//         sendStatus();
//     }
//     else if (command == "manual")
//     {
//         state.isAutoMode = false;
//         sendStatus();
//     }
//     else if (command == "state")
//     {
//         sendStatus();
//     }
// }

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    handleCommand(String((char *)payload, length));
}

void printMessage(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message content: ");
    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
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
    // client.subscribe(MQTT_COMMAND_TOPIC);
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

// // Remove static lastStatusTime variable from loop()
// void loop()
// {
//     if (!client.connected())
//         reconnectMQTT();
//     client.loop();

//     // Button handling with debounce
//     if (digitalRead(BUTTON_PIN) == LOW)
//     {
//         delay(50);
//         if (digitalRead(BUTTON_PIN) == LOW)
//         {
//             state.isAutoMode = !state.isAutoMode;
//             if (!state.isAutoMode)
//                 state.manualStartTime = millis();
//             sendStatus(); // Send status when button changes mode
//             while (digitalRead(BUTTON_PIN) == LOW)
//                 ;
//         }
//     }

//     // Mode handling
//     state.isAutoMode ? autoMode() : digitalWrite(LED_PIN, state.ledState);

//     // Remove periodic status update
//     // if (millis() - lastStatusTime >= STATUS_INTERVAL) {
//     //     lastStatusTime = millis();
//     //     sendStatus();
//     // }
// }
