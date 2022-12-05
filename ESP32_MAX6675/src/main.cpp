#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27);

void setup()
{
  lcd.begin(16,2);
  lcd.clear();         
  lcd.backlight();      // Make sure backlight is on
  lcd.setCursor(0,0);
  lcd.print("  I2C LCD with ");
  lcd.setCursor(0,1);
  lcd.print("  ESP32 DevKit ");
  //delay(2000);
}


void loop()
{
  
}