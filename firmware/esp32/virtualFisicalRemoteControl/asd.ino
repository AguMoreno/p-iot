#include <WiFi.h>
#include <AsyncMQTT_ESP32.h>
#include <Ticker.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

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

#define DEBUG true  // Set to true to enable debug prints, false to disable

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

  // Definir los tópicos y mensajes
  const char* topicReset = "cbpadel/quadra/reset";
  const char* topicPointA = "cbpadel/quadra/point_a";
  const char* topicPointB = "cbpadel/quadra/point_b";
  const char* topicUndo = "cbpadel/quadra/undo";

  const char* msgReset = "{\"action\": \"Reset\"}";
  const char* msgPointA = "{\"action\": \"A\"}";
  const char* msgPointB = "{\"action\": \"B\"}";
  const char* msgUndo = "{\"action\": \"Undo\"}";

  if (action == "A") {
    Serial.println("Action A received");
    mqttClient.publish(topicPointA, 2, true, msgPointA);
    Serial.printf("Message sent to %s: %s\n", topicPointA, msgPointA);

  } else if (action == "B") {
    Serial.println("Action B received");
    mqttClient.publish(topicPointB, 2, true, msgPointB);
    Serial.printf("Message sent to %s: %s\n", topicPointB, msgPointB);

  } else if (action == "Undo") {
    Serial.println("Action Undo received");
    mqttClient.publish(topicUndo, 2, true, msgUndo);
    Serial.printf("Message sent to %s: %s\n", topicUndo, msgUndo);

  } else if (action == "Reset") {
    Serial.println("Action Reset received");
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

  if (DEBUG) {
    Serial.println("Setup complete. Waiting for MQTT messages...");
  }
}

void loop() {
  // No need to use loop() with AsyncMqttClient
}
