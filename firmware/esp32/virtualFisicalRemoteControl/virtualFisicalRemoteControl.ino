#include <WiFi.h>
#include <AsyncMQTT_ESP32.h>
#include <Ticker.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>

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

// Variables for debounce and long press detection
unsigned long lastSignalTime = 0;
unsigned long signalDStartTime = 0;
const unsigned long debounceDelay = 3000;  // 3 seconds debounce
const unsigned long longPressDelay = 10000;  // 10 seconds long press for reset

Ticker rfCheckTicker;  // Ticker for checking RF signals

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
    mqttClient.publish(topicPointA, 2, true, msgPointA);
    Serial.printf("Message sent to %s: %s\n", topicPointA, msgPointA);
  } else if (action == "B") {
    mqttClient.publish(topicPointB, 2, true, msgPointB);
    Serial.printf("Message sent to %s: %s\n", topicPointB, msgPointB);
  } else if (action == "Undo") {
    mqttClient.publish(topicUndo, 2, true, msgUndo);
    Serial.printf("Message sent to %s: %s\n", topicUndo, msgUndo);
  } else if (action == "Reset") {
    mqttClient.publish(topicReset, 2, true, msgReset);
    Serial.printf("Message sent to %s: %s\n", topicReset, msgReset);
  } else {
    Serial.println("Unknown action received");
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

  // Blink the LED twice
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
    delay(150);  // Wait for 150 milliseconds
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
    delay(150);  // Wait for 150 milliseconds
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

      unsigned long currentMillis = millis();

      // Check for signal A
      if (receivedValue == SIGNAL_A && (currentMillis - lastSignalTime > debounceDelay)) {
        handleAction("A");
        lastSignalTime = currentMillis;
      }

      // Check for signal B
      if (receivedValue == SIGNAL_B && (currentMillis - lastSignalTime > debounceDelay)) {
        handleAction("B");
        lastSignalTime = currentMillis;
      }

      // Check for signal C
      if (receivedValue == SIGNAL_C && (currentMillis - lastSignalTime > debounceDelay)) {
        handleAction("Undo");
        lastSignalTime = currentMillis;
      }

      // Check for signal D (long press for reset)
      if (receivedValue == SIGNAL_D) {
        if (signalDStartTime == 0) {
          signalDStartTime = currentMillis;
        } else if (currentMillis - signalDStartTime >= longPressDelay) {
          handleAction("Reset");
          signalDStartTime = 0;
        }
      } else {
        signalDStartTime = 0;  // Reset the timer if signal D is not detected
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
  mySwitch.enableReceive(4);  // Receiver on interrupt pin 4

  // Set up ticker to check RF signals periodically
  rfCheckTicker.attach_ms(100, checkRFSignal);  // Check RF signals every 100 ms

  if (DEBUG) {
    Serial.println("Setup complete. Waiting for MQTT messages...");
  }
}

void loop() {
  // No need to use loop() with AsyncMqttClient and Ticker
}
