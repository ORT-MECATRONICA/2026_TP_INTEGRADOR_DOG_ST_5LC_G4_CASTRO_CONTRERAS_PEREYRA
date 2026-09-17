// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#include <Wire.h>
#include <Adafruit_INA219.h>

#define ESPERA 1000
Adafruit_INA219 ina219;
unsigned long tiempo;

void setup() {
  Serial.begin(115200);
  while (!ina219.begin()) {
    Serial.println("Aún no conecta");
    delay(1000);
  }
  Serial.println("INA219 inicializado");
}

void loop() {
  if (millis() - tiempo >= ESPERA) {
    float corriente = ina219.getCurrent_mA();
    Serial.println("Corriente: ");
    Serial.print(corriente);
    Serial.println(" mA");
    tiempo = millis();
  }
}