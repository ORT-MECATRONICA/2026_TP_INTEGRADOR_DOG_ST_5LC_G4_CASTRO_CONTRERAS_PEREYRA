// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
// Este no es el código definitivo, simplemente sirve para probar todos los componentes a la vez
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>
#include <ld2410.h>

// Defines pines
#define LEDR 12
#define LEDA 13
#define LEDV 14
#define B1 5
#define B2 23
#define B3 32
#define SDA5V 18
#define SCL5V 19
#define RXD2 16 // TX del LD2410
#define TXD2 17 // RX del LD2410
#define OPTO 33
#define LDR 34
#define MQ2 35
#define INTERVALO 5000

// Definición objetos
Adafruit_BMP280 bmp;
Adafruit_INA219 ina219;
LiquidCrystal_I2C lcd(0x27, 16, 2);
ld2410 radar;

unsigned long tiempoAnterior = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDA, OUTPUT);  
  pinMode(LEDV, OUTPUT);  
  pinMode(B1, INPUT);
  pinMode(B2, INPUT);  
  pinMode(B3, INPUT);  
  pinMode(OPTO, INPUT_PULLUP);
  while (!bmp.begin(0x76)) {}
  Serial.println("Sensor BMP280 inicializado correctamente");
  while (!ina219.begin()) {}
  Serial.println("INA219 inicializado");
  Wire1.begin(SDA5V, SCL5V);
  lcd.init();          
  lcd.backlight();     
  lcd.setCursor(0, 0);
  Serial2.begin(256000, SERIAL_8N1, RXD2, TXD2);
  radar.begin(Serial2);

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
    float corriente = ina219.getCurrent_mA();
    Serial.print("Corriente: ");
    Serial.print(corriente);
    Serial.println(" mA");
    int valorInicialLDR = analogRead(LDR);
    int valorFinalLDR = map(valorInicialLDR, 0, 4095, 0, 100);
    Serial.print("Valor del LDR: ");
    Serial.print(valorFinalLDR);
    Serial.println("%");
    int valorInicialMQ2 = analogRead(MQ2);
    int valorFinalMQ2 = map(valorInicialMQ2, 0, 4095, 0, 100);
    Serial.print("Valor del MQ2: ");
    Serial.print(valorFinalMQ2);
    Serial.println("%");
    if (radar.presenceDetected()) {
      Serial.print("Distancia: ");
      Serial.print(radar.detectionDistance());
      Serial.println(" cm");
    } else {
      Serial.println("No se detectó a nadie")
    }
    if (digitalRead(OPTO) == LOW) {}
      Serial.println("El opto recibió una señal);
    } else {
      Serial.println("El opto no recibió ninguna señal);
    }
  }
  void pulsadores();
}

void pulsadores () {
  if (digitalRead(B1) == LOW && digitalRead(B2) == HIGH && digitalRead(B3) == HIGH) {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDV, LOW);
    lcd.print("                ");
    lcd.print("Botón 1");
  }
  else if (digitalRead(B1) == HIGH && digitalRead(B2) == LOW && digitalRead(B3) == HIGH) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDV, LOW);
    lcd.print("                ");
    lcd.print("Botón 2");
  }
  else if (digitalRead(B1) == HIGH && digitalRead(B2) == HIGH && digitalRead(B3) == LOW) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDV, HIGH);
    lcd.print("                ");
    lcd.print("Botón 3");
  }
  else if (digitalRead(B1) == LOW && digitalRead(B2) == LOW && digitalRead(B3) == HIGH) {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDV, LOW);
    lcd.print("                ");
    lcd.print("Botones 1 y 2");
  }
  else if (digitalRead(B1) == LOW && digitalRead(B2) == HIGH && digitalRead(B3) == LOW) {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDV, HIGH);
    lcd.print("                ");
    lcd.print("Botones 1 y 3");
  }
  else if (digitalRead(B1) == HIGH && digitalRead(B2) == LOW && digitalRead(B3) == LOW) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDV, HIGH);
    lcd.print("                ");
    lcd.print("Botones 2 y 3");
  }
  else if (digitalRead(B1) == LOW && digitalRead(B2) == LOW && digitalRead(B3) == LOW) {
    digitalWrite(LEDR, HIGH);
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDV, HIGH);
    lcd.print("                ");
    lcd.print("Botones 1, 2 y 3");
  }
  else if (digitalRead(B1) == HIGH && digitalRead(B2) == HIGH && digitalRead(B3) == HIGH) {
    digitalWrite(LEDR, LOW);
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDV, LOW);
    lcd.print("                ");
  }
}
