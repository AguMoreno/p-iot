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

// Definición de pines para botones
const int pinA = 18;   // D18
const int pinB = 19;   // D19
const int pinUndo = 21;  // D21
const int pinReset = 5;  // D5

// Variables para guardar el estado de los botones
bool stateA = false;
bool stateB = false;
bool stateUndo = false;
bool stateReset = false;

// Variables para el debouncing
unsigned long lastDebounceTimeA = 0;
unsigned long lastDebounceTimeB = 0;
unsigned long lastDebounceTimeUndo = 0;
unsigned long lastDebounceTimeReset = 0;
const unsigned long debounceDelay = 100; // 100 ms de retardo para el debouncing

// Variables para el manejo del botón Reset
unsigned int resetPressCount = 0;
unsigned long firstResetPressTime = 0;
const unsigned long resetPressWindow = 20000; // 20 segundos

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

  // Secuencia de parpadeo al enviar el mensaje por MQTT
  for (int i = 0; i < 3; i++) {
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

// Esta función lee el estado de un botón, aplica un debounce y maneja el evento de presionado.
void readButtonState(int pin, bool &state, unsigned long &lastDebounceTime, const char* message, bool isResetButton = false) {
  bool reading = digitalRead(pin) == LOW; // LOW significa que el botón está presionado

  if (reading != state && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    state = reading;

    if (state) {
      handleButtonPress(message, isResetButton);
    }
  }
}

// Maneja las acciones cuando el botón de reset se ha presionado 6 veces en 20 segundos.
void handleResetButtonPress() {
  unsigned long currentTime = millis();

  if (resetPressCount == 0) {
    firstResetPressTime = currentTime;  // Iniciar el tiempo de la ventana
  }

  resetPressCount++;
  Serial.printf("Reset press count: %d\n", resetPressCount);

  if (resetPressCount >= 6 && (currentTime - firstResetPressTime <= resetPressWindow)) {
    handleAction("Reset");
    resetPressCount = 0;  // Reiniciar el contador después de ejecutar
    firstResetPressTime = 0;  // Reiniciar el tiempo de la ventana
  } else if (currentTime - firstResetPressTime > resetPressWindow) {
    resetPressCount = 1;  // Reiniciar el contador si han pasado más de 20 segundos y contar la actual
    firstResetPressTime = currentTime;  // Reiniciar el tiempo de la ventana
  }
}

// Maneja la acción del botón presionado
void handleButtonPress(const char* message, bool isResetButton) {
  // Secuencia de parpadeo al presionar un botón
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
    delay(350);  // Wait for 350 milliseconds
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
    delay(350);  // Wait for 350 milliseconds
  }
  handleDebug(message);
  if (!isResetButton) {
    handleAction(message);
  } else {
    handleResetButtonPress();
  }
}

// Muestra un mensaje de debug y hace parpadear un LED.
void handleDebug(const char* message) {
  if (DEBUG) {
    Serial.println(message);
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set up pin modes
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  // Ensure LED is off initially

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(pinUndo, INPUT_PULLUP);
  pinMode(pinReset, INPUT_PULLUP);

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
  // Lectura y manejo de los estados de los botones de manera asincrónica
  readButtonState(pinA, stateA, lastDebounceTimeA, "A");
  readButtonState(pinB, stateB, lastDebounceTimeB, "B");
  readButtonState(pinUndo, stateUndo, lastDebounceTimeUndo, "Undo");
  readButtonState(pinReset, stateReset, lastDebounceTimeReset, "Reset", true);

  // Verificar si el tiempo de la ventana para el botón de reset ha expirado
  if (resetPressCount > 0 && (millis() - firstResetPressTime > resetPressWindow)) {
    resetPressCount = 0;  // Reiniciar el contador si han pasado más de 20 segundos
    firstResetPressTime = 0;  // Reiniciar el tiempo de la ventana
  }
}
