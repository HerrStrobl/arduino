#include <Wire.h> // Wire Bibliothek einbinden
#include <LiquidCrystal_I2C.h> // Vorher hinzugefügte LiquidCrystal_I2C Bibliothek einbinden

#define SLEEP_TIME 500

#define LCD_DISPLAY_ADDR 0x27
#define LCD_DISPLAY_LEN 16
#define LCD_DISPLAY_LINES 2

#define PIN_RELAIS 4
#define PIN_LED_RED 5
#define PIN_LED_GREEN 6
#define PIN_LED_BLUE 7
#define PIN_BUTTON 8

#define PIN_MEAS_VOLTAGE A0
#define REF_VOLTAGE 5.0
#define PIN_STEPS 1024.0
#define VOLTAGE_DIVIDER 10.0

LiquidCrystal_I2C lcd(LCD_DISPLAY_ADDR, LCD_DISPLAY_LEN, LCD_DISPLAY_LINES);

volatile bool bButtonPressed = false;
volatile float dVoltageMeasured = 0;
int nVoltageMeasured_mV;
int nVoltageMeasured_V_pre;
int nVoltageMeasured_V_after;
int nBatteryVoltage_mV;
int nBatteryVoltage_V_pre;
int nBatteryVoltage_V_after;

const int nBatteryLevel[] = {0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.2, 0.3, 0.4, 0.52, 0.64, 0.74, 0.83, 0.96, 1};
const float fBatteryLevelVoltageOffset = 2.8;
const int nBatteryLevelVoltageMultiplier = 10;
const int nMaxIndex = 14;

float getBatteryLevel(float fVoltage);

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
char text[30]; 

// Read Inputs
bButtonPressed = digitalRead(PIN_BUTTON) == 0;
dVoltageMeasured = analogRead(PIN_MEAS_VOLTAGE) * REF_VOLTAGE / PIN_STEPS;

float percentage = getBatteryLevel(dVoltageMeasured);
int percentage_pre = (percentage*100);
int percentage_past = (percentage*100-percentage_pre)*100;

// CALC VOLTAGE
//int nVoltageMeasured_mV = dVoltageMeasured * 1000;
//int nVoltageMeasured_V_pre = dVoltageMeasured;
//int nVoltageMeasured_V_after = (dVoltageMeasured - nVoltageMeasured_V_pre) * 100;
int nBatteryVoltage_mV = nVoltageMeasured_mV * VOLTAGE_DIVIDER;
int nBatteryVoltage_V_pre = dVoltageMeasured * VOLTAGE_DIVIDER;
int nBatteryVoltage_V_after = (dVoltageMeasured * VOLTAGE_DIVIDER * 100) - (nBatteryVoltage_V_pre * 100);

// WRITE VOLTAGE
char sVoltMeas[30]; 
//sprintf(sVoltMeas, "U=%d.%dV", nVoltageMeasured_V_pre, nVoltageMeasured_V_after);
sprintf(sVoltMeas, "U=%d.%dV", nBatteryVoltage_V_pre, nBatteryVoltage_V_after);
lcd.setCursor(8, 0);
lcd.print(sVoltMeas);

// WRITE PERCENTAGE
char sPercMeas[30]; 
sprintf(sPercMeas, "%d.%d %%", percentage_pre, percentage_past);
lcd.setCursor(8, 1);
lcd.print(sPercMeas); 

sprintf(text, "U = %d mV \n", nVoltageMeasured_mV);
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

//lcd.setCursor(0, 0);//Hier wird die Position des ersten Zeichens festgelegt. In diesem Fall bedeutet (0,0) das erste Zeichen in der ersten Zeile. 
//lcd.print("Ich liebe dich !"); 
//lcd.setCursor(0, 1);// In diesem Fall bedeutet (0,1) das erste Zeichen in der zweiten Zeile. 
//lcd.print(" <3 <3 <3 <3 <3 ");


Serial.print("test");
sprintf(text, "U = %d.%dV \n", 41,5);
Serial.print(text);
sprintf(text, "Perc %d %% \n", (int) (getBatteryLevel(4.15)*100.0));
Serial.print(text);

}

float getBatteryLevel(float fVoltage)
{
  float result;
  float fIndex = (fVoltage - fBatteryLevelVoltageOffset) * nBatteryLevelVoltageMultiplier;
  int index = fIndex;
  
  if( index < 0 ) result = 0;
  else if( index >= nMaxIndex) result = 1;
  else
  {
    result = nBatteryLevel[index] + (nBatteryLevel[index+1] - nBatteryLevel[index]) * (fIndex-index);
  }
  return result;
}
