// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#define LED 2
#define POTE 34

const int umbral = 2048;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(POTE, INPUT);
}


void loop() {
  int valor = analogRead(POTE);
  Serial.print("Valor del POTE: ");
  Serial.println(valor);
  if (valor < umbral) {
    digitalWrite(LED, HIGH);  // Prender LED
  } else {
    digitalWrite(LED, LOW);   // Apagar LED
  }
  delay(200); // Pausa
}