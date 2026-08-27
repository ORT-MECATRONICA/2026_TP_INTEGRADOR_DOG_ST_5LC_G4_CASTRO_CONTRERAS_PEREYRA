// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#define B1 35
#define LED 34

void setup() {
  pinMode(B1, INPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  if (digitalRead(B1) == LOW) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}
