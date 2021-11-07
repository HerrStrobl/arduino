#include <Wire.h> // Wire Bibliothek einbinden
#include <LiquidCrystal_I2C.h> // Vorher hinzugefügte LiquidCrystal_I2C Bibliothek einbinden

#define SLEEP_TIME 50

#define LCD_DISPLAY_ADDR 0x27
#define LCD_DISPLAY_LEN 16
#define LCD_DISPLAY_LINES 2

#define PIN_RELAIS 4
#define PIN_LED_RED 5
#define PIN_LED_GREEN 6
#define PIN_LED_BLUE 7
#define PIN_BUTTON 8

#define PIN_AIN_VOLTAGE 0

LiquidCrystal_I2C lcd(LCD_DISPLAY_ADDR, LCD_DISPLAY_LEN, LCD_DISPLAY_LINES);

volatile bool bButtonPressed = false;

void setup() 
{
lcd.init(); //Im Setup wird der LCD gestartet 
lcd.backlight(); //Hintergrundbeleuchtung einschalten (lcd.noBacklight(); schaltet die Beleuchtung aus).

pinMode(PIN_RELAIS, OUTPUT);
pinMode(PIN_LED_RED, OUTPUT);
pinMode(PIN_LED_GREEN, OUTPUT);
pinMode(PIN_LED_BLUE, OUTPUT);
pinMode(PIN_BUTTON, INPUT);

}

void loop() 
{
Serial.begin(9600);

volatile bool bButtonPressed = digitalRead(PIN_BUTTON) == 0;

char text[20]; 
sprintf(text, "DigitalRead: %i\n", digitalRead(PIN_BUTTON));
Serial.print(text);

sprintf(text,"ButtonPressed: %b\n", bButtonPressed);
Serial.print(text);

if(bButtonPressed)
{
  digitalWrite(PIN_RELAIS, HIGH);
}
else 
{
  digitalWrite(PIN_RELAIS, LOW);
}
digitalWrite(PIN_LED_RED, LOW);
digitalWrite(PIN_LED_GREEN, HIGH);
digitalWrite(PIN_LED_BLUE, LOW);

//digitalRead(PIN_BUTTON);

delay(SLEEP_TIME);  //500ms Pause

lcd.setCursor(0, 0);//Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile. 
lcd.print("Ich liebe dich !"); 
lcd.setCursor(0, 1);// In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile. 
lcd.print(" <3 <3 <3 <3 <3 ");


}
