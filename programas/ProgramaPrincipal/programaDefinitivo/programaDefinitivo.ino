// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra

#include <WiFi.h>
#include "AsyncMqttClient.h"
#include <Wire.h>
#include "time.h"
#include "Arduino.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>
#include <ld2410.h>

// Wi-Fi y MQTT
const char* ssid = "MECA-IoT-V2";
const char* password = "IoT$2026";
const uint8_t name_device = 134;

// Timers
unsigned long now = millis();    ///valor actual
unsigned long lastMeasure1 = 0;  ///variable para contar el tiempo actual
unsigned long lastMeasure2 = 0;  ///variable para contar el tiempo actual
const unsigned long interval_envio = 30000;  //Intervalo de envio de datos mqtt. Valor inicial
const unsigned long interval_leeo = 60000;   //Intervalo de lectura de datos y guardado en la cola. Valor inicial
long cambiar_periodo_envio;
int i = 0;

// Time
long unsigned int timestamp;  // hora
const char* ntpServer = "south-america.pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

// Variables ingresar a la cola struct
int indice_entra = 0;
int indice_saca = 0;
bool flag_vacio = 1;

// MQTT
#define MQTT_HOST IPAddress(192, 168, 10, 151)  // Se debe cambiar por el ip de meca o hall del 4
#define MQTT_PORT 1884
#define MQTT_USERNAME "esp32"
#define MQTT_PASSWORD "mirko15"
char mqtt_payload[150];

// Test MQTT Topic
#define MQTT_PUB "/esp32/datos_sensores"
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

typedef struct {
  long time;
  float T1;  // temperatura en grados
  float P1;  // valor presion entre 0 y 99 
  float luz;  // valor entre 0 y 99, si hay luz es cercano al 100 
  float G1; // valor gas entre 0 y 99 
  float cor; // valor corriente entre 0 y 99
  bool pre; // Lectura sensores de presencia 1 presencia 0 no presencia
  bool oct; // Lectura sensores de presencia 1 presencia 0 no presencia
  bool Alarma; // si se supera un umbral
}
const int valor_max_struct = 1000;          ///valor vector de struct
estructura datos_struct[valor_max_struct];  ///Guardo valores hasta que lo pueda enviar
estructura aux2;

void setupmqtt() {
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
  connectToWifi();
}
void fun_envio_mqtt() {
  fun_saca();           // veo si hay valores nuevos
  if (flag_vacio == 0)  // si hay los envio
  {
    Serial.print("enviando");
    snprintf(mqtt_payload, sizeof(mqtt_payload), 
         "%d&%ld&%.2f&%.2f&%.2f&%.2f&%.2f&%u&%u&%u", 
         name_device, aux2.time, aux2.T1, aux2.P1, aux2.luz, 
         aux2.G1, aux2.cor, aux2.pre, aux2.oct, aux2.Alarma);
    aux2.time = 0;                                                                                                                                                   ///limpio valores
    aux2.T1 = 0;
    aux2.P1 = 0;
    aux2.luz = 0;
    aux2.G1 = 0;
    aux2.cor = 0;
    aux2.pre = 0;
    aux2.oct = 0;
    aux2.Alarma = 0;

    Serial.print("Publish message: ");
    Serial.println(mqtt_payload);
    // Publishes Temperature and Humidity values
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB, 1, true, mqtt_payload);
  } else {
    Serial.println("no hay valores nuevos");
  }
}  
void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
}  
void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}  
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0);  // Para evitar reconectarse al MQTT mientras se conecta al Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}
void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}
void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}
void fun_saca() {
  if (indice_saca != indice_entra) {
    aux2.time = datos_struct[indice_saca].time;
    aux2.T1 = datos_struct[indice_saca].T1;
    aux2.P1 = datos_struct[indice_saca].P1;
    aux2.luz = datos_struct[indice_saca].luz;
    aux2.G1 = datos_struct[indice_saca].G1;
    aux2.cor = datos_struct[indice_saca].cor;
    aux2.pre = datos_struct[indice_saca].pre;
    aux2.oct = datos_struct[indice_saca].oct;
    aux2.Alarma = datos_struct[indice_saca].Alarma;
    flag_vacio = 0;
    Serial.println(indice_saca);
    if (indice_saca >= (valor_max_struct - 1)) {
      indice_saca = 0;
    } else {
      indice_saca++;
    }
    Serial.print("saco valores de la struct isaca:");
    Serial.println(indice_saca);
  } else {
    flag_vacio = 1;  // no hay datos
  }
  return;
}
void fun_entra(void) {
  if (indice_entra >= valor_max_struct) {
    indice_entra = 0;  // si llego al maximo de la cola se vuelve a cero
  }
  Serial.print("> NTP Time:");
  timestamp = time(NULL);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;  // si no puede conseguir la hora
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  datos_struct[indice_entra].time = timestamp;
  datos_struct[indice_entra].T1 = 3;  // leeo los datos, acá va la funcion de cada sensor
  datos_struct[indice_entra].P1 = 4;  
  datos_struct[indice_entra].luz = 5;
  datos_struct[indice_entra].G1 = 6;
  datos_struct[indice_entra].cor = 7;
  datos_struct[indice_entra].pre = 8;
  datos_struct[indice_entra].oct = 9;
  datos_struct[indice_entra].Alarma = 1;
  indice_entra++;
  Serial.print("saco valores de la struct indice_entra");
  Serial.println(indice_entra);
}
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
  setupmqtt();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
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
      ultimoPulso1 == false;
      estadoActual = (estadoPrograma)((estadoActual - 1) % 10);
    }
    if (ultimoPulso2 == true) {
      ultimoPulso2 == false;
      estadoActual = (estadoPrograma)((estadoActual + 1) % 10);
    }
    if (ultimoPulso3 == true) {
      //Poner los 10 if para MN de 0 a 9
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
  now = millis();
  if (now - lastMeasure1 > interval_envio) {  // envio el doble de lectura por si falla algun envio
    lastMeasure1 = now;                       // cargo el valor actual de millis
    fun_envio_mqtt();                         // envio los valores por mqtt
  }
  if (now - lastMeasure2 > interval_leeo) {
    lastMeasure2 = now;  // cargo el valor actual de millis
    fun_entra();         // ingreso los valores a la cola struct
  }
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
