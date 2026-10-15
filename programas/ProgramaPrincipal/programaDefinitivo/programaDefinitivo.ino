// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>
#include <ld2410.h>

typedef enum {
  // Control
  RST,
  soltarPulsador,
  // Menú
  MN0, // guía y GMT
  MN1, // GMT y MQTT
  MN2, // MQTT y temperatura
  MN3, // temperatura y presión
  MN4, // presión y gas
  MN5, // gas y luz
  MN6, // luz y corriente
  MN7, // corriente y presencia humana
  MN8, // presencia humana y opto
  MN9, // opto y guía
  // Umbrales y pantallas (0 es la pantalla principal, 1 para aumentar el umbral y 2 para disminuirlo)
  guia,
  GMT0,
  GMT1,
  GMT2,
  MQTT0,
  MQTT1,
  MQTT2,
  temperatura0,
  temperatura1,
  temperatura2,
  presion0,
  presion1,
  presion2,
  gas0,
  gas1,
  gas2,
  luz0,
  luz1,
  luz2,
  corriente0,
  corriente1,
  corriente2,
  presencia0,
  presencia1,
  presencia2,
  opto0,
  opto1,
  opto2,
} estadoPrograma;
estadoPrograma estadoActual = RST;
estadoPrograma estadoPrevio = RST;

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
#define ESPERA_PULSO 30
#define INTERVALO 5000

// Definición objetos
Adafruit_BMP280 bmp;
Adafruit_INA219 ina219;
LiquidCrystal_I2C lcd(0x27, 16, 2);
ld2410 radar;

// Variables
unsigned long tiempoActual;
unsigned long tiempoAnterior = 0;
bool ultimoPulso1 = false;
bool ultimoPulso2 = false;
bool ultimoPulso3 = false;
bool primerPulso1 = false;
bool primerPulso2 = false;
bool primerPulso3 = false;
unsigned long tiempoPulso1 = 0;
unsigned long tiempoPulso2 = 0;
unsigned long tiempoPulso3 = 0;
unsigned long desfasaje = 0;
unsigned long tiempoMillis = 0;
unsigned long tiempoLiberacion = 0;
float temperatura;
float presion;
float corriente;
int valorInicialLDR;
float valorFinalLDR;
int valorInicialMQ2;
float valorFinalMQ2;
String presenciaHumana;
String opto;

void setup() {
  Serial.begin(115200);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDA, OUTPUT);  
  pinMode(LEDV, OUTPUT);  
  pinMode(B1, INPUT);
  pinMode(B2, INPUT);  
  pinMode(B3, INPUT);  
  pinMode(OPTO, INPUT_PULLUP);
  Wire1.begin(SDA5V, SCL5V);
  while (!bmp.begin(0x76)) {}
  Serial.println("Sensor BMP280 inicializado correctamente");
  while (!ina219.begin()) {}
  Serial.println("INA219 inicializado");
  lcd.init();          
  lcd.backlight();     
  Serial2.begin(256000, SERIAL_8N1, RXD2, TXD2);
  radar.begin(Serial2);
}

void loop() {
  switch (estadoActual) {
    case(RST):
    estadoActual = MN0;
    break;

    case(soltarPulsador):
    if (ultimoPulso1 == true) {

    }


    case(MN0):
    if (estadoPrevio == RST || estadoPrevio == MN1 || estadoPrevio == MN9) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Guia <");
      lcd.setCursor(0, 1);
      lcd.print("GMT");
      estadoPrevio = MN0;
    }
    pulsosMenu();
    break;
    

  }
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
    float valorFinalLDR = map(valorInicialLDR, 0, 4095, 0, 100);
    Serial.print("Valor del LDR: ");
    Serial.print(valorFinalLDR);
    Serial.println("%");
    int valorInicialMQ2 = analogRead(MQ2);
    float valorFinalMQ2 = map(valorInicialMQ2, 0, 4095, 0, 100);
    Serial.print("Valor del MQ2: ");
    Serial.print(valorFinalMQ2);
    Serial.println("%");
    if (radar.presenceDetected()) {
      Serial.print("Distancia: ");
      Serial.print(radar.detectionDistance());
      Serial.println(" cm");
    } else {
      Serial.println("No se detectó a nadie");
    }
    if (digitalRead(OPTO) == LOW) {
      Serial.println("El opto recibió una señal");
    } else {
      Serial.println("El opto no recibió ninguna señal");
    }
  }
  lecturas();
}

void lecturas() {
  tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= INTERVALO) {
    tiempoAnterior = tiempoActual;
    temperatura = bmp.readTemperature();
    presion = bmp.readPressure() / 100.0F; // Convertir Pa a hPa
    corriente = ina219.getCurrent_mA();
    valorInicialLDR = analogRead(LDR);
    valorFinalLDR = map(valorInicialLDR, 0, 4095, 0, 100);
    valorInicialMQ2 = analogRead(MQ2);
    valorFinalMQ2 = map(valorInicialMQ2, 0, 4095, 0, 100);
    if (radar.presenceDetected()) {
      presenciaHumana = "Distancia " + radar.detectionDistance() + " cm";
    } else {
      presenciaHumana = "No se detectó a nadie";
    }
    if (digitalRead(OPTO) == LOW) {
      opto = "El opto recibió una señal";
    } else {
      opto = "El opto no recibió ninguna señal";
    }
  }
}

void pulsosMenu() {
  if (digitalRead(B1) == LOW && primerPulso1 == false) {
    tiempoPulso1 = millis();
    primerPulso1 = true;
  }
  if (primerPulso1 == true) {
    if (millis() - tiempoPulso1 >= ESPERA_PULSO) {
      if (digitalRead(B1) == LOW) {
        primerPulso1 = false;
        ultimoPulso1 = true;
        tiempoMillis = 0;
        desfasaje = millis();
        estadoPrevio = estadoActual;
        estadoActual = soltarPulsador;
      }
    } else {
      if (digitalRead(B1) == HIGH) {
        primerPulso1 = false;
      }
    }
  }

  if (digitalRead(B2) == LOW && primerPulso2 == false) {
    tiempoPulso2 = millis();
    primerPulso2 = true;
  }
  if (primerPulso2 == true) {
    if (millis() - tiempoPulso2 >= ESPERA_PULSO) {
      if (digitalRead(B2) == LOW) {
        primerPulso2 = false;
        ultimoPulso2 = true;
        tiempoMillis = 0;
        desfasaje = millis();
        estadoPrevio = estadoActual;
        estadoActual = soltarPulsador;
      }
    } else {
      if (digitalRead(B2) == HIGH) {
        primerPulso2 = false;
      }
    }
  }

  if (digitalRead(B3) == LOW && primerPulso3 == false) {
    tiempoPulso3 = millis();
    primerPulso3 = true;
  }
  if (primerPulso3 == true) {
    if (millis() - tiempoPulso3 >= ESPERA_PULSO) {
      if (digitalRead(B3) == LOW) {
        primerPulso3 = false;
        ultimoPulso3 = true;
        tiempoMillis = 0;
        desfasaje = millis();
        estadoPrevio = estadoActual;
        estadoActual = soltarPulsador;
      }
    } else {
      if (digitalRead(B3) == HIGH) {
        primerPulso3 = false;
      }
    }
  }
}
