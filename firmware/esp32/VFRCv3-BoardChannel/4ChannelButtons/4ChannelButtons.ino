#include <WiFi.h>

const char* ssid = "ApMoreno";
const char* password = "am480216";

#define DEBUG true

// Definición de pines
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
const unsigned long resetPressWindow = 15000; // 15 segundos

// Declaración de funciones
void readButtonState(int pin, bool &state, unsigned long &lastDebounceTime, const char* message, bool isResetButton = false);
void handleResetButtonPress();
void handleDebug(const char* message);

void setup() {
  // Inicialización del puerto serial
  Serial.begin(115200);

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Configuración de pines
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  pinMode(pinUndo, INPUT_PULLUP);
  pinMode(pinReset, INPUT_PULLUP);

  // Configuración del LED
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Lectura y manejo de los estados de los botones de manera asincrónica
  readButtonState(pinA, stateA, lastDebounceTimeA, "Button A Pressed");
  readButtonState(pinB, stateB, lastDebounceTimeB, "Button B Pressed");
  readButtonState(pinUndo, stateUndo, lastDebounceTimeUndo, "Button Undo Pressed");
  readButtonState(pinReset, stateReset, lastDebounceTimeReset, "Button Reset Pressed", true);

  // Verificar si el tiempo de la ventana para el botón de reset ha expirado
  if (resetPressCount > 0 && (millis() - firstResetPressTime > resetPressWindow)) {
    resetPressCount = 0;  // Reiniciar el contador si han pasado más de 15 segundos
  }
}

void readButtonState(int pin, bool &state, unsigned long &lastDebounceTime, const char* message, bool isResetButton) {
  bool reading = digitalRead(pin) == LOW; // LOW significa que el botón está presionado

  if (reading != state && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    state = reading;

    if (state) {
      handleDebug(message);
      if (isResetButton) {
        handleResetButtonPress();
      }
    }
  }
}

void handleResetButtonPress() {
  unsigned long currentTime = millis();

  if (resetPressCount == 0) {
    firstResetPressTime = currentTime;  // Iniciar el tiempo de la ventana
  }

  resetPressCount++;

  if (resetPressCount >= 10 && (currentTime - firstResetPressTime <= resetPressWindow)) {
    handleDebug("Button Reset Pressed 10 Times");
    resetPressCount = 0;  // Reiniciar el contador después de ejecutar
  }
}

void handleDebug(const char* message) {
  if (DEBUG) {
    Serial.println(message);
  }
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
    delay(150);  // Wait for 150 milliseconds
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off
    delay(150);  // Wait for 150 milliseconds
  }
}
