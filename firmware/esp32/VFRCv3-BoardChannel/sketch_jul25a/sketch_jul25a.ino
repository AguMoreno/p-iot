#include <WiFi.h>
#include <AsyncMQTT_ESP32.h>
#include <Ticker.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>
#include <vector>

using namespace std;

// WiFi configuration
const char* ssid = "ApMoreno";
const char* password = "am480216";

// MQTT configuration
const char* mqtt_server = "broker.emqx.io";
const char* subscribe_topic = "cbpadel/quadra2/+";
const char* publish_topic = "cbpadel/quadra2/test";

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;
RCSwitch mySwitch = RCSwitch();

#define DEBUG true  // Set to true to enable debug prints, false to disable

// RF signals configuration
#define SIGNAL_A 753832
#define SIGNAL_B 753828
#define SIGNAL_C 753826
#define SIGNAL_D 753825

// Pin definitions
#define PIN_A 33
#define PIN_B 32
#define PIN_UNDO 35
#define PIN_RESET 34

// Flags for interrupt handling
volatile bool flagA = false;
volatile bool flagB = false;
volatile bool flagUndo = false;
volatile bool flagReset = false;

// Variables for debounce
volatile unsigned long lastInterruptTimeA = 0;
volatile unsigned long lastInterruptTimeB = 0;
volatile unsigned long lastInterruptTimeUndo = 0;
volatile unsigned long lastInterruptTimeReset = 0;
const unsigned long debounceDelay = 200;  // 200 ms debounce delay

vector<String> messageQueue;

unsigned long signalDStartTime = 0;
const unsigned long longPressDelay = 10000;  // 10 seconds long press for reset

Ticker rfCheckTicker;  // Ticker for checking RF signals

void IRAM_ATTR handleInterruptA() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTimeA > debounceDelay) {
    flagA = true;
    lastInterruptTimeA = currentTime;
  }
}

void IRAM_ATTR handleInterruptB() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTimeB > debounceDelay) {
    flagB = true;
    lastInterruptTimeB = currentTime;
  }
}

void IRAM_ATTR handleInterruptUndo() {
  unsigned long currentTime = millis();
  if (currentTime - lastInterruptTimeUndo > debounceDelay) {
    flagUndo = true;
    lastInterruptTimeUndo = currentTime;
  }
}

void IRAM_ATTR handleInterruptReset() {
  unsigned long currentTime = millis();
  if (digitalRead(PIN_RESET) == LOW) {
    if (signalDStartTime == 0) {
      signalDStartTime = currentTime;
    } else if (currentTime - signalDStartTime >= longPressDelay) {
      flagReset = true;
      signalDStartTime = 0;
    }
  } else {
    signalDStartTime = 0;  // Reset the timer if signal D is not detected
  }
  lastInterruptTimeReset = currentTime;
}

void setupWiFi() {
  if (DEBUG) {
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
  }
  WiFi.begin(ssid, password);
}

void connectToMqtt() {
  if (DEBUG) {
    Serial.println("Connecting to MQTT...");
  }
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  if (DEBUG) {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
  }
  uint16_t packetIdSub = mqttClient.subscribe(subscribe_topic, 2);
  if (DEBUG) {
    Serial.print("Subscribing to topic: ");
    Serial.println(subscribe_topic);
    Serial.print(" with QoS 2, packetId: ");
    Serial.println(packetIdSub);
  }

  // Publish a test message
  uint16_t packetIdPub = mqttClient.publish(publish_topic, 2, true, "Hello from ESP32");
  if (DEBUG) {
    Serial.print("Publishing test message, packetId: ");
    Serial.println(packetIdPub);
  }

  // Try to publish any stored messages
  while (!messageQueue.empty()) {
    String msg = messageQueue.front();
    mqttClient.publish(publish_topic, 2, true, msg.c_str());
    messageQueue.erase(messageQueue.begin());
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  if (DEBUG) {
    Serial.println("Disconnected from MQTT.");
  }
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if (DEBUG) {
    Serial.print("Subscribe acknowledged. PacketId: ");
    Serial.print(packetId);
    Serial.print(", QoS: ");
    Serial.println(qos);
  }
}

void handleAction(const String &action) {
  if (DEBUG) {
    Serial.print("Action: ");
    Serial.println(action);
  }

  // Define topics and messages
  const char* topicPointA = "cbpadel/quadra/point_a";
  const char* topicPointB = "cbpadel/quadra/point_b";
  const char* topicUndo = "cbpadel/quadra/undo";
  const char* topicReset = "cbpadel/quadra/reset";

  const char* msgPointA = "{\"action\": \"A\"}";
  const char* msgPointB = "{\"action\": \"B\"}";
  const char* msgUndo = "{\"action\": \"Undo\"}";
  const char* msgReset = "{\"action\": \"Reset\"}";

  if (action == "A") {
    if (mqttClient.connected()) {
      mqttClient.publish(topicPointA, 2, true, msgPointA);
      Serial.printf("Message sent to %s: %s\n", topicPointA, msgPointA);
    } else {
      messageQueue.push_back(String(msgPointA));
      Serial.println("Stored message for later sending.");
    }
  } else if (action == "B") {
    if (mqttClient.connected()) {
      mqttClient.publish(topicPointB, 2, true, msgPointB);
      Serial.printf("Message sent to %s: %s\n", topicPointB, msgPointB);
    } else {
      messageQueue.push_back(String(msgPointB));
      Serial.println("Stored message for later sending.");
    }
  } else if (action == "Undo") {
    if (mqttClient.connected()) {
      mqttClient.publish(topicUndo, 2, true, msgUndo);
      Serial.printf("Message sent to %s: %s\n", topicUndo, msgUndo);
    } else {
      messageQueue.push_back(String(msgUndo));
      Serial.println("Stored message for later sending.");
    }
  } else if (action == "Reset") {
    if (mqttClient.connected()) {
      mqttClient.publish(topicReset, 2, true, msgReset);
      Serial.printf("Message sent to %s: %s\n", topicReset, msgReset);
    } else {
      messageQueue.push_back(String(msgReset));
      Serial.println("Stored message for later sending.");
    }
  } else {
    Serial.println("Unknown action received");
  }

  // Blink the LED twice
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
    delay(50);  // Wait for 50 milliseconds
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
    delay(50);  // Wait for 50 milliseconds
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  if (DEBUG) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.write(payload, len);
    Serial.println();
  }

  // Convert payload to string
  String message;
  for (size_t i = 0; i < len; i++) {
    message += (char)payload[i];
  }

  // Parse the JSON message
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    if (DEBUG) {
      Serial.print("Failed to parse message: ");
      Serial.println(error.c_str());
    }
    return;
  }

  // Get the action
  const char* action = doc["action"];
  handleAction(String(action));
}

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      if (DEBUG) {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      }
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (DEBUG) {
        Serial.println("WiFi lost connection");
      }
      mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      wifiReconnectTimer.once(2, setupWiFi);
      break;
    default:
      break;
  }
}

void checkRFSignal() {
  if (mySwitch.available()) {
    unsigned long receivedValue = mySwitch.getReceivedValue();
    if (receivedValue == 0) {
      if (DEBUG) {
        Serial.println("Unknown encoding");
      }
    } else {
      if (DEBUG) {
        Serial.print("Received ");
        Serial.print(receivedValue);
        Serial.print(" / ");
        Serial.print(mySwitch.getReceivedBitlength());
        Serial.print("bit ");
        Serial.print("Protocol: ");
        Serial.println(mySwitch.getReceivedProtocol());
      }

      if (receivedValue == SIGNAL_A) {
        flagA = true;
      } else if (receivedValue == SIGNAL_B) {
        flagB = true;
      } else if (receivedValue == SIGNAL_C) {
        flagUndo = true;
      } else if (receivedValue == SIGNAL_D) {
        flagReset = true;
      }
    }
    mySwitch.resetAvailable();
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set up pin modes
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // Ensure LED is off initially

  // Set up WiFi and MQTT
  WiFi.onEvent(onWiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(mqtt_server, 1883);

  setupWiFi();

  // Initialize RF receiver
  mySwitch.enableReceive(14);  // Receiver on interrupt pin 14

  // Set up pin modes and attach interrupts
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_UNDO, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_A), handleInterruptA, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_B), handleInterruptB, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_UNDO), handleInterruptUndo, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_RESET), handleInterruptReset, FALLING);

  if (DEBUG) {
    Serial.println("Setup complete. Waiting for MQTT messages...");
  }
}

void loop() {
  checkRFSignal();  // Check RF signals continuously in the loop

  if (flagA) {
    flagA = false;
    handleAction("A");
  }

  if (flagB) {
    flagB = false;
    handleAction("B");
  }

  if (flagUndo) {
    flagUndo = false;
    handleAction("Undo");
  }

  if (flagReset) {
    flagReset = false;
    handleAction("Reset");
  }
}
