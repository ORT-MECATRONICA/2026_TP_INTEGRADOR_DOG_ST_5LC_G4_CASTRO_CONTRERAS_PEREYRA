// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#define LED 2
#define LDR 34

const int umbral = 2500;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(LDR, INPUT);
}


void loop() {
  int valor = analogRead(LDR);
  Serial.print("Valor del LDR: ");
  Serial.println(valor);
  if (valor < umbral) {
    digitalWrite(LED, HIGH);  // Prender LED
  } else {
    digitalWrite(LED, LOW);   // Apagar LED
  }
  delay(200); // Pausa
}


