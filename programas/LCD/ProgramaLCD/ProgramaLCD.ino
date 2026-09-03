// Grupo 4: Tiziano Castro, Tomás Contreras y Tomas Pereyra
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA 21
#define SCL 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Inicialización explícita de I2C en pines 21 (SDA) y 22 (SCL)
  Wire.begin(SDA, SCL);
  lcd.init();          // Inicializa el LCD
  lcd.backlight();     // Enciende la luz de fondo
  lcd.setCursor(0, 0); // Columna 0, Fila 0
  lcd.print("Hola Mundo!");
}

void loop() {}

