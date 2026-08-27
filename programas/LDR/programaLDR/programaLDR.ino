// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#define LED 2
#define LDR 34

const int umbral = 500;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(LDR, INPUT);
}


void loop() {
  int valorInicial = analogRead(LDR);
  int valorFinal = map(valorInicial, 0, 4095, 0, 100);
  Serial.print("Valor del LDR: ");
  Serial.println(valorFinal);
  if (valorFinal < umbral) {
    digitalWrite(LED, HIGH);  // Prender LED
  } else {
    digitalWrite(LED, LOW);   // Apagar LED
  }
  delay(200); // Pausa
}


