// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "time.h"
#include "Arduino.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_INA219.h>
#include <LiquidCrystal_I2C.h>
#include <ld2410.h>
#include <Preferences.h>

// Definiciones de variación de umbrales
#define VARIACION_UMBRAL_TEMPERATURA 1   // 1 °C
#define VARIACION_UMBRAL_PRESION     1   // 1 hPa
#define VARIACION_UMBRAL_CORRIENTE   1   // 1 mA
#define VARIACION_UMBRAL_LUZ         1   // 1 %
#define VARIACION_UMBRAL_GAS         1   // 1 %
#define VARIACION_UMBRAL_PRESENCIA   5   // 5 cm
#define VARIACION_UMBRAL_GMT         1   // 1 hora
#define VARIACION_UMBRAL_MQTT        5   // 5 segundos

// Wi-Fi
const char* ssid = "MECA-IoT-V2";
const char* password = "IoT$2026";
const uint8_t name_device = 134;

// Timers
unsigned long now = millis();    /// valor actual
unsigned long lastMeasure1 = 0;  /// variable para contar el tiempo actual
unsigned long lastMeasure2 = 0;  /// variable para contar el tiempo actual
unsigned long interval_envio = 30000;  // Intervalo de envio de datos mqtt. Valor inicial
const unsigned long interval_leeo = 60000;   // Intervalo de lectura de datos y guardado en la cola. Valor inicial

// Time
long unsigned int timestamp;  // hora
const char* ntpServer = "south-america.pool.ntp.org";
long gmtOffset_sec = -10800; // GMT-3 por defecto (-10800 s)
const int daylightOffset_sec = 0;

// Variables ingresar a la cola struct
int indice_entra = 0;
int indice_saca = 0;
bool flag_vacio = 1;

// MQTT (EMQX CLOUD SSL/TLS)
#define MQTT_HOST "sa11b75f.ala.us-east-1.emqxsl.com"
#define MQTT_PORT 8883
#define MQTT_USERNAME "esp32"
#define MQTT_PASSWORD "mirko15"
#define MQTT_PUB "/esp32/datos_sensores"

char mqtt_payload[150];

// Objetos de Red y MQTT SSL
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Objeto para almacenamiento persistente en Flash
Preferences preferences;

// Estructura de datos
typedef struct {
  long time;
  float t1;   // temperatura en grados
  float P1;   // valor presion en hPa
  float luz;  // valor entre 0 y 100% 
  float G1;   // valor gas entre 0 y 100% 
  float cor;  // valor corriente en mA
  bool pre;   // Lectura sensores de presencia 1 presencia 0 no presencia
  bool oct;   // Lectura sensores de optoacoplador 1 presencia 0 no presencia
  bool Alarma; // si se supera un umbral
} estructura;

const int valor_max_struct = 1000;          /// valor vector de struct
estructura datos_struct[valor_max_struct];  /// Guardo valores hasta que lo pueda enviar
estructura aux2;

// Variables globales de mediciones de sensores
float t1 = 0;
float P1 = 0;
float luz = 0;
float G1 = 0;
float cor = 0;
bool pre = false;
bool oct = false;
bool Alarma = false;
int distancia_radar = 0;

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
  MN8, // presencia humana y guía
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
#define SDA5V 18 // Pin SDA para el LCD (5V)
#define SCL5V 19 // Pin SCL para el LCD (5V)
#define RXD2 16  // TX del LD2410
#define TXD2 17  // RX del LD2410
#define OPTO 33
#define LDR 34
#define MQ2 35
#define ESPERA_PULSO 30
#define INTERVALO 5000

// Definición de objetos
// Sensores en bus I2C predeterminado 'Wire' (GPIO 21 SDA, GPIO 22 SCL)
Adafruit_BMP280 bmp(&Wire);
Adafruit_INA219 ina219;

// Pantalla LCD en canal I2C secundario
LiquidCrystal_I2C lcd(0x27, 16, 2);
ld2410 radar;

// Variables de tiempo y rebote
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
float valorInicialP;
int valorInicialLDR;
int valorInicialMQ2;

// Umbrales configurables
int UT1 = 30;     // Umbral temperatura (°C)
int UP1 = 1013;   // Umbral presion (hPa)
int Uluz = 50;    // Umbral luz (%)
int UG1 = 30;     // Umbral gas (%)
int Ucor = 100;   // Umbral corriente (mA)
int Upre = 100;   // Umbral presencia humana (distancia en cm)

// Declaración de funciones
void connectToWifi();
void connectToMqtt();
void fun_envio_mqtt();
void fun_saca();
void fun_entra();
void lecturas();
void pulsosMenu();
void verificarUmbrales();
void cargarPreferencias();
void guardarPreferencias();

void connectToWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectToMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT (EMQX Cloud SSL)...");
    String clientId = "ESP32_Grupo_" + String(name_device) + "_" + WiFi.macAddress();
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println(" ¡Conectado a MQTT!");
    } else {
      Serial.print(" Falló conexión, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

void fun_envio_mqtt() {
  fun_saca();           // veo si hay valores nuevos
  if (flag_vacio == 0)  // si hay los envio
  {
    Serial.print("enviando ");
    snprintf(mqtt_payload, sizeof(mqtt_payload), 
             "%d&%ld&%.2f&%.2f&%.2f&%.2f&%.2f&%u&%u&%u", 
             name_device, aux2.time, aux2.t1, aux2.P1, aux2.luz, 
             aux2.G1, aux2.cor, aux2.pre, aux2.oct, aux2.Alarma);
    
    aux2.time = 0;      /// limpio valores
    aux2.t1 = 0;
    aux2.P1 = 0;
    aux2.luz = 0;
    aux2.G1 = 0;
    aux2.cor = 0;
    aux2.pre = 0;
    aux2.oct = 0;
    aux2.Alarma = 0;

    Serial.print("Publish message: ");
    Serial.println(mqtt_payload);
    mqttClient.publish(MQTT_PUB, mqtt_payload);
  } else {
    Serial.println("no hay valores nuevos");
  }
}

void fun_saca() {
  if (indice_saca != indice_entra) {
    aux2.time = datos_struct[indice_saca].time;
    aux2.t1 = datos_struct[indice_saca].t1;
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
  datos_struct[indice_entra].t1 = t1; 
  datos_struct[indice_entra].P1 = P1;  
  datos_struct[indice_entra].luz = luz;
  datos_struct[indice_entra].G1 = G1;
  datos_struct[indice_entra].cor = cor;
  datos_struct[indice_entra].pre = pre;
  datos_struct[indice_entra].oct = oct;
  datos_struct[indice_entra].Alarma = Alarma;
  indice_entra++;
  
  Serial.print("Ingresado a la struct, indice_entra: ");
  Serial.println(indice_entra);
}

// Cargar umbrales desde memoria NVS
void cargarPreferencias() {
  preferences.begin("umbrales", true);
  UT1 = preferences.getInt("UT1", 30);
  UP1 = preferences.getInt("UP1", 1013);
  Uluz = preferences.getInt("Uluz", 50);
  UG1 = preferences.getInt("UG1", 30);
  Ucor = preferences.getInt("Ucor", 100);
  Upre = preferences.getInt("Upre", 100);
  gmtOffset_sec = preferences.getLong("gmtOffset", -10800);
  interval_envio = preferences.getULong("intEnvio", 30000);
  preferences.end();
}

// Guardar umbrales en memoria NVS
void guardarPreferencias() {
  preferences.begin("umbrales", false);
  preferences.putInt("UT1", UT1);
  preferences.putInt("UP1", UP1);
  preferences.putInt("Uluz", Uluz);
  preferences.putInt("UG1", UG1);
  preferences.putInt("Ucor", Ucor);
  preferences.putInt("Upre", Upre);
  preferences.putLong("gmtOffset", gmtOffset_sec);
  preferences.putULong("intEnvio", interval_envio);
  preferences.end();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// Comparación de mediciones con umbrales y manejo de leds
void verificarUmbrales() {
  int contadorAlertas = 0;

  if (t1 > UT1) contadorAlertas++;
  if (P1 > UP1) contadorAlertas++;
  if (cor > Ucor) contadorAlertas++;
  if (luz > Uluz) contadorAlertas++;
  if (G1 > UG1) contadorAlertas++;
  if (pre) contadorAlertas++;
  if (oct) contadorAlertas++;

  if (contadorAlertas == 0) {
    digitalWrite(LEDV, HIGH);
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDR, LOW);
    Alarma = false;
  } else if (contadorAlertas == 1) {
    digitalWrite(LEDV, LOW);
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDR, LOW);
    Alarma = true;
  } else { // contadorAlertas > 1
    digitalWrite(LEDV, LOW);
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDR, HIGH);
    Alarma = true;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Cargar umbrales previamente guardados
  cargarPreferencias();
  
  // Configuración de Wi-Fi y MQTT SSL
  connectToWifi();
  espClient.setInsecure(); // Permite SSL sin verificar certificado CA en Flash
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  // Configuración del servidor de hora NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDA, OUTPUT);  
  pinMode(LEDV, OUTPUT);  
  pinMode(B1, INPUT);
  pinMode(B2, INPUT);  
  pinMode(B3, INPUT);  
  pinMode(OPTO, INPUT_PULLUP);

  // 1. Inicializar bus I2C predeterminado para BMP280 e INA219 (GPIO 21 / 22)
  Wire.begin();

  // 2. Inicializar bus I2C secundario para la pantalla LCD (GPIO 18 / 19)
  Wire1.begin(SDA5V, SCL5V);

  while (!bmp.begin(0x76)) {
    Serial.println("Esperando sensor BMP280...");
    delay(500);
  }
  Serial.println("Sensor BMP280 inicializado correctamente");

  while (!ina219.begin()) {
    Serial.println("Esperando sensor INA219...");
    delay(500);
  }
  Serial.println("INA219 inicializado");

  // Inicialización de pantalla LCD usando Wire1
  lcd.init();          
  lcd.backlight();     

  Serial2.begin(256000, SERIAL_8N1, RXD2, TXD2);
  radar.begin(Serial2);
}

void loop() {
  // Verificación constante de Wi-Fi y servidor MQTT
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }
  if (!mqttClient.connected()) {
    connectToMqtt();
  }
  mqttClient.loop();

  switch (estadoActual) {
    case(RST):
      estadoActual = MN0;
      break;

    case(soltarPulsador):
      if (ultimoPulso1 == true) { // Botón 1: Retroceder en menú o Aumentar umbral
        ultimoPulso1 = false;
        if (estadoPrevio >= MN0 && estadoPrevio <= MN8) {
          int idx = estadoPrevio - MN0;
          estadoActual = (estadoPrograma)(MN0 + (idx + 8) % 9);
        } else {
          switch (estadoPrevio) {
            case GMT0:         estadoActual = GMT1; break;
            case MQTT0:        estadoActual = MQTT1; break;
            case temperatura0: estadoActual = temperatura1; break;
            case presion0:     estadoActual = presion1; break;
            case gas0:         estadoActual = gas1; break;
            case luz0:         estadoActual = luz1; break;
            case corriente0:   estadoActual = corriente1; break;
            case presencia0:   estadoActual = presencia1; break;
            default:           break;
          }
        }
      }
      else if (ultimoPulso2 == true) { // Botón 2: Avanzar en menú o Disminuir umbral
        ultimoPulso2 = false;
        if (estadoPrevio >= MN0 && estadoPrevio <= MN8) {
          int idx = estadoPrevio - MN0;
          estadoActual = (estadoPrograma)(MN0 + (idx + 1) % 9);
        } else {
          switch (estadoPrevio) {
            case GMT0:         estadoActual = GMT2; break;
            case MQTT0:        estadoActual = MQTT2; break;
            case temperatura0: estadoActual = temperatura2; break;
            case presion0:     estadoActual = presion2; break;
            case gas0:         estadoActual = gas1; break;
            case luz0:         estadoActual = luz2; break;
            case corriente0:   estadoActual = corriente2; break;
            case presencia0:   estadoActual = presencia2; break;
            default:           break;
          }
        }
      }
      else if (ultimoPulso3 == true) { // Botón 3: Seleccionar o Volver guardando a MN0
        ultimoPulso3 = false;
        if (estadoPrevio >= MN0 && estadoPrevio <= MN8) {
          switch (estadoPrevio) {
            case MN0: estadoActual = guia; break;
            case MN1: estadoActual = GMT0; break;
            case MN2: estadoActual = MQTT0; break;
            case MN3: estadoActual = temperatura0; break;
            case MN4: estadoActual = presion0; break;
            case MN5: estadoActual = gas0; break;
            case MN6: estadoActual = luz0; break;
            case MN7: estadoActual = corriente0; break;
            case MN8: estadoActual = presencia0; break;
            default: break;
          }
        } else {
          // Al salir de cualquier submenú hacia el menú principal, guardamos preferencias
          guardarPreferencias();
          estadoActual = MN0;
        }
      }
      break;

    // --- Menú principal ---
    case(MN0):
      if (estadoPrevio != MN0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Guia <");
        lcd.setCursor(0, 1);
        lcd.print("GMT");
        estadoPrevio = MN0;
      }
      pulsosMenu();
      break;

    case(MN1):
      if (estadoPrevio != MN1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("GMT <");
        lcd.setCursor(0, 1);
        lcd.print("MQTT");
        estadoPrevio = MN1;
      }
      pulsosMenu();
      break;

    case(MN2):
      if (estadoPrevio != MN2) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MQTT <");
        lcd.setCursor(0, 1);
        lcd.print("Temperatura");
        estadoPrevio = MN2;
      }
      pulsosMenu();
      break;

    case(MN3):
      if (estadoPrevio != MN3) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temperatura <");
        lcd.setCursor(0, 1);
        lcd.print("Presion");
        estadoPrevio = MN3;
      }
      pulsosMenu();
      break;

    case(MN4):
      if (estadoPrevio != MN4) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Presion <");
        lcd.setCursor(0, 1);
        lcd.print("Gas");
        estadoPrevio = MN4;
      }
      pulsosMenu();
      break;

    case(MN5):
      if (estadoPrevio != MN5) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Gas <");
        lcd.setCursor(0, 1);
        lcd.print("Luz");
        estadoPrevio = MN5;
      }
      pulsosMenu();
      break;

    case(MN6):
      if (estadoPrevio != MN6) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Luz <");
        lcd.setCursor(0, 1);
        lcd.print("Corriente");
        estadoPrevio = MN6;
      }
      pulsosMenu();
      break;

    case(MN7):
      if (estadoPrevio != MN7) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Corriente <");
        lcd.setCursor(0, 1);
        lcd.print("Presencia");
        estadoPrevio = MN7;
      }
      pulsosMenu();
      break;

    case(MN8):
      if (estadoPrevio != MN8) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Presencia <");
        lcd.setCursor(0, 1);
        lcd.print("Guia");
        estadoPrevio = MN8;
      }
      pulsosMenu();
      break;

    // --- Guía ---
    case (guia):
      if (estadoPrevio != guia) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("B1:+ B2:-");
        lcd.setCursor(0, 1);
        lcd.print("B3: Guardar/MN0");
        estadoPrevio = guia;
      }
      pulsosMenu();
      break;

    // --- Submenú GMT ---
    case (GMT0):
      lcd.setCursor(0, 0);
      lcd.print("GMT (+/-1h)     ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(gmtOffset_sec / 3600);
      lcd.print("h R:");
      lcd.print(gmtOffset_sec / 3600);
      lcd.print("h   ");
      pulsosMenu();
      break;

    case (GMT1):
      gmtOffset_sec += (VARIACION_UMBRAL_GMT * 3600);
      estadoActual = GMT0;
      break;

    case (GMT2):
      gmtOffset_sec -= (VARIACION_UMBRAL_GMT * 3600);
      estadoActual = GMT0;
      break;

    // --- Submenú MQTT ---
    case (MQTT0):
      lcd.setCursor(0, 0);
      lcd.print("MQTT (+/-5s)    ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(interval_envio / 1000);
      lcd.print("s R:");
      lcd.print(interval_envio / 1000);
      lcd.print("s   ");
      pulsosMenu();
      break;

    case (MQTT1):
      interval_envio += (VARIACION_UMBRAL_MQTT * 1000);
      estadoActual = MQTT0;
      break;

    case (MQTT2):
      if (interval_envio > (VARIACION_UMBRAL_MQTT * 1000)) {
        interval_envio -= (VARIACION_UMBRAL_MQTT * 1000);
      }
      estadoActual = MQTT0;
      break;

    // --- Submenú Temperatura ---
    case (temperatura0):
      lcd.setCursor(0, 0);
      lcd.print("Temp (+/-1C)    ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(UT1);
      lcd.print("C R:");
      lcd.print(t1, 1);
      lcd.print("C  ");
      pulsosMenu();
      break;

    case (temperatura1):
      UT1 += VARIACION_UMBRAL_TEMPERATURA;
      estadoActual = temperatura0;
      break;

    case (temperatura2):
      UT1 -= VARIACION_UMBRAL_TEMPERATURA;
      estadoActual = temperatura0;
      break;

    // --- Submenú Presión ---
    case (presion0):
      lcd.setCursor(0, 0);
      lcd.print("Pres (+/-1hPa)  ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(UP1);
      lcd.print(" R:");
      lcd.print((int)P1);
      lcd.print("hPa ");
      pulsosMenu();
      break;

    case (presion1):
      UP1 += VARIACION_UMBRAL_PRESION;
      estadoActual = presion0;
      break;

    case (presion2):
      UP1 -= VARIACION_UMBRAL_PRESION;
      estadoActual = presion0;
      break;

    // --- Submenú Gas ---
    case (gas0):
      lcd.setCursor(0, 0);
      lcd.print("Gas (+/-1%)     ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(UG1);
      lcd.print("% R:");
      lcd.print((int)G1);
      lcd.print("%   ");
      pulsosMenu();
      break;

    case (gas1):
      if (UG1 < 100) UG1 += VARIACION_UMBRAL_GAS;
      estadoActual = gas0;
      break;

    case (gas2):
      if (UG1 > 0) UG1 -= VARIACION_UMBRAL_GAS;
      estadoActual = gas0;
      break;

    // --- Submenú Luz ---
    case (luz0):
      lcd.setCursor(0, 0);
      lcd.print("Luz (+/-1%)     ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(Uluz);
      lcd.print("% R:");
      lcd.print((int)luz);
      lcd.print("%   ");
      pulsosMenu();
      break;

    case (luz1):
      if (Uluz < 100) Uluz += VARIACION_UMBRAL_LUZ;
      estadoActual = luz0;
      break;

    case (luz2):
      if (Uluz > 0) Uluz -= VARIACION_UMBRAL_LUZ;
      estadoActual = luz0;
      break;

    // --- Submenú Corriente ---
    case (corriente0):
      lcd.setCursor(0, 0);
      lcd.print("Cor (+/-1mA)    ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(Ucor);
      lcd.print("mA R:");
      lcd.print((int)cor);
      lcd.print("mA ");
      pulsosMenu();
      break;

    case (corriente1):
      Ucor += VARIACION_UMBRAL_CORRIENTE;
      estadoActual = corriente0;
      break;

    case (corriente2):
      if (Ucor > 0) Ucor -= VARIACION_UMBRAL_CORRIENTE;
      estadoActual = corriente0;
      break;

    // --- Submenú Presencia Humana ---
    case (presencia0):
      lcd.setCursor(0, 0);
      lcd.print("Pres (+/-5cm)   ");
      lcd.setCursor(0, 1);
      lcd.print("U:");
      lcd.print(Upre);
      lcd.print("cm R:");
      lcd.print(distancia_radar);
      lcd.print("cm ");
      pulsosMenu();
      break;

    case (presencia1):
      Upre += VARIACION_UMBRAL_PRESENCIA;
      estadoActual = presencia0;
      break;

    case (presencia2):
      if (Upre >= VARIACION_UMBRAL_PRESENCIA) Upre -= VARIACION_UMBRAL_PRESENCIA;
      estadoActual = presencia0;
      break;
  }

  lecturas();
  now = millis();
  if (now - lastMeasure1 > interval_envio) {
    lastMeasure1 = now;        // cargo el valor actual de millis
    fun_envio_mqtt();          // envio los valores por mqtt
  }
  if (now - lastMeasure2 > interval_leeo) {
    lastMeasure2 = now;        // cargo el valor actual de millis
    fun_entra();               // ingreso los valores a la cola struct
  }
}

void lecturas() {
  tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= INTERVALO) {
    tiempoAnterior = tiempoActual;
    t1 = bmp.readTemperature();
    Serial.println(t1);
    valorInicialP = bmp.readPressure() / 100.0F; // Convertir Pa a hPa
    P1 = map(valorInicialP, 0, 1050, 0, 100);
    cor = ina219.getCurrent_mA();
    valorInicialLDR = analogRead(LDR);
    luz = map(valorInicialLDR, 0, 4095, 0, 100);
    valorInicialMQ2 = analogRead(MQ2);
    G1 = map(valorInicialMQ2, 0, 4095, 0, 100);
    
    // Lectura del radar LD2410
    radar.read();
    distancia_radar = 0;
    if (radar.presenceDetected()) {
      if (radar.movingTargetDetected()) {
        distancia_radar = radar.movingTargetDistance();
      } else if (radar.stationaryTargetDetected()) {
        distancia_radar = radar.stationaryTargetDistance();
      }
    }

    // Evaluación de presencia por distancia menor al umbral Upre
    if (distancia_radar > 0 && distancia_radar < Upre) {
      pre = true;
    } else {
      pre = false;
    }

    // Optoacoplador
    if (digitalRead(OPTO) == LOW) {
      oct = true;
    } else {
      oct = false;
    }

    // Evaluación de alertas y control de LEDs
    verificarUmbrales();
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
