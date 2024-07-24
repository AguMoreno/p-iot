#include <RCSwitch.h>
#include <ArduinoJson.h>

RCSwitch mySwitch = RCSwitch();
const int ledPin = 2; // LED integrado en el pin GPIO 2
unsigned long previousMillis = 0; // Almacena el último tiempo en que el LED cambió de estado
const long interval = 2000; // Intervalo de parpadeo en milisegundos (2 segundos)

void setup() {
  // Inicializa la comunicación serial
  Serial.begin(115200);

  // Configura el LED integrado como salida
  pinMode(ledPin, OUTPUT);

  // Configura el receptor RF en el pin GPIO 4
  mySwitch.enableReceive(4);  // Pin DATA conectado a GPIO 4 (D4 en el ESP32)

  Serial.println("Receptor RF 433 MHz configurado. Esperando señales...");
}

void loop() {
  // Obtiene el tiempo actual
  unsigned long currentMillis = millis();

  // Parpadea el LED cada 2 segundos
  if (currentMillis - previousMillis >= interval) {
    // Guarda el último tiempo en que el LED cambió de estado
    previousMillis = currentMillis;

    // Cambia el estado del LED
    digitalWrite(ledPin, !digitalRead(ledPin));
  }

  // Verifica si hay una señal recibida
  if (mySwitch.available()) {
    int value = mySwitch.getReceivedValue();

    // Crea un objeto JSON para almacenar los datos recibidos
    StaticJsonDocument<200> doc;

    if (value == 0) {
      doc["status"] = "error";
      doc["message"] = "Unknown encoding";
    } else {
      doc["status"] = "success";
      doc["value"] = value;
      doc["bit_length"] = mySwitch.getReceivedBitlength();
      doc["protocol"] = mySwitch.getReceivedProtocol();
    }

    // Serializa el objeto JSON y lo imprime en el monitor serial
    String output;
    serializeJson(doc, output);
    Serial.println(output);

    // Reinicia la disponibilidad de la señal para la próxima recepción
    mySwitch.resetAvailable();
  }
}
