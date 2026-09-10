// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#include <Wire.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;
#define INTERVALO 3000
unsigned long tiempoAnterior = 0;

void setup() {
  Serial.begin(115200);  
  while (!bmp.begin(0x76)) {}
  Serial.println("Sensor BMP280 inicializado correctamente");
}

void loop() {
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= INTERVALO) {
    tiempoAnterior = tiempoActual;
    float temperatura = bmp.readTemperature();
    float presion = bmp.readPressure() / 100.0F; // Convertir Pa a hPa
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");
    Serial.print("Presión: ");
    Serial.print(presion);
    Serial.println(" hPa");
  }
}
