// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#include <ld2410.h>
ld2410 radar;

#define RXD2 16 // TX del LD2410
#define TXD2 17 // RX del LD2410

void setup() {
  Serial.begin(115200);
  Serial2.begin(256000, SERIAL_8N1, RXD2, TXD2);
  radar.begin(Serial2);

  Serial.println("Probando Radar");
}

void loop() {
  radar.read();

  static unsigned long ultimoRefresco = 0;
  if (millis() - ultimoRefresco > 500) {
    ultimoRefresco = millis();
    if (radar.isConnected()) {
      if (radar.presenceDetected()) {
        if (radar.movingTargetDetected() && radar.stationaryTargetDetected()) {
          Serial.print("Estado: Movimiento + Estatico");
        } else if (radar.movingTargetDetected()) {
          Serial.print("Estado: Solo Movimiento");
        } else if (radar.stationaryTargetDetected()) {
          Serial.print("Estado: Solo Estatico");
        }
        Serial.print("Distancia: ");
        Serial.print(radar.detectionDistance());
        Serial.println(" cm");
      } else {
        Serial.println("Estado: Libre (Sin presencia)");
      }
    } else {
      Serial.println("Buscando radar / Sin respuesta...");
    }
  }
}