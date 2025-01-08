#include <WiFi.h>
#include <PubSubClient.h>
// #include <secretKey.h>

// Configuration constants
constexpr char SSID[] = "SamSua 2G";
constexpr char PASSWORD[] = "samthui2014";
constexpr char googleApiKey[] = "your_GOOGLE_API_KEY";

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
