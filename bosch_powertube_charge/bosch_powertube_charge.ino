#include <Wire.h> // Wire Bibliothek einbinden
#include <LiquidCrystal_I2C.h> // Vorher hinzugefügte LiquidCrystal_I2C Bibliothek einbinden

#define SLEEP_TIME 50
#define BUTTON_LONGPRESS_TIME 2000

#define ASCII_NUMBER '0'
#define fTS_LENGTH 20

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

#define STATE_INITIAL           0b0000
#define STATE_BATTERY_CONNECTED 0b0001
#define STATE_BATTERY_CHARGING  0b0010
#define STATE_BATTERY_CHARGED   0b0100
#define STATE_BATTERY_ERROR     0b1000

//                          0123456789abcdef
#define TEXT_CONNECT_BAT   "Connect Battery "
#define TEXT_BAT_CONNECTED "Connected       "
#define TEXT_BAT_CHARGING  "Charging        "
#define TEXT_BAT_CHARGED   "Battery charged "
#define TEXT_BAT_ERROR     "ERROR           "

#define MOVE_DOT_LCD_POS 8
#define MOVE_DOT_CNT 8

#define CELL_PERCENTAGE_MAX 0.8

const uint32_t LOOP_MASK_all    = 0b11111111111111111111111111111111;
const uint32_t LOOP_MASK_half   = 0b01010101010101010101010101010101;
const uint32_t LOOP_MASK_4th    = 0b00010001000100010001000100010001;
const uint32_t LOOP_MASK_8th    = 0b00000001000000010000000100000001;
const uint32_t LOOP_MASK_16th   = 0b00000000000000010000000000000001;
const uint32_t LOOP_MASK_32th   = 0b00000000000000000000000000000001;

typedef unsigned int state;
typedef unsigned long time_t;

LiquidCrystal_I2C lcd(LCD_DISPLAY_ADDR, LCD_DISPLAY_LEN, LCD_DISPLAY_LINES);

volatile state gState = STATE_INITIAL;

volatile time_t t_startup;
volatile time_t t_loop;
volatile time_t t_loop_last = 0;
volatile long t_loopdelta = SLEEP_TIME;
volatile long t_loopdelta_corrector = 0;
volatile time_t t_sleep_time = SLEEP_TIME;
volatile time_t t_charge_begin;
volatile time_t t_charge_end;
volatile time_t t_charge_total = 0;
volatile time_t t_button_pressed = 0;

int move_dot_pos = 0;

volatile bool bButtonPressed_last = false;
volatile bool bButtonPressed = false;
volatile bool bButtonPressed_short = false;
volatile bool bButtonPressed_long = false;
volatile float dVoltageMeasured = 0;
volatile float dVoltageMeasured_smoothed = 0;
volatile float fBatteryVoltage_V = 0;
volatile float fCellPercentage = 0;

const float fBatteryLevel[] = {0.04, 0.06, 0.08, 0.10, 0.12, 0.16, 0.2, 0.3, 0.4, 0.52, 0.64, 0.74, 0.83, 0.96, 1};
const float fBatteryLevelVoltageOffset = 2.8;
const int fBatteryLevelVoltageMultiplier = 10;
const int nMaxIndex = 14;

char * floatToString(char * cString, float fNumber, int nDigits);
float getCellLevel(float fVoltage);
void mngLoopTime();
void wipeDisplayTop();
void wipeDisplayBottom();
void wipeDisplay();
void startCharging();
void stopCharging();
bool battConnected();
bool battCharged();
bool consumeButtonShortPress();
bool consumeButtonLongPress();
void mgnButton();


uint32_t _loop;

void setup() 
{
Serial.begin(9600);
Serial.print("Setup\n");

lcd.init(); //Im Setup wird der LCD gestartet 
lcd.backlight(); //Hintergrundbeleuchtung einschalten (lcd.noBacklight(); schaltet die Beleuchtung aus).

pinMode(PIN_RELAIS, OUTPUT);
pinMode(PIN_LED_RED, OUTPUT);
pinMode(PIN_LED_GREEN, OUTPUT);
pinMode(PIN_LED_BLUE, OUTPUT);
pinMode(PIN_BUTTON, INPUT);

t_startup = millis();
Serial.print("startup time: ");
Serial.print(t_startup);
Serial.print("\n");

wipeDisplay();
lcd.setCursor(0, 0);
lcd.print(TEXT_CONNECT_BAT);
_loop = 0b10000000;
}

void loop() 
{
_loop = _loop << 1;
if(_loop == 0) _loop = 1;

Serial.print("Loop\n");
Serial.print(_loop);
Serial.print("\n");
mngLoopTime();

char cText_lcd[LCD_DISPLAY_LEN]; 
char fTS_tmp[fTS_LENGTH];

// Read Inputs
mgnButton();

if(_loop & LOOP_MASK_4th)
{
dVoltageMeasured = analogRead(PIN_MEAS_VOLTAGE) * REF_VOLTAGE / PIN_STEPS;
dVoltageMeasured_smoothed = (dVoltageMeasured_smoothed + dVoltageMeasured * 49) / 50;

fBatteryVoltage_V = dVoltageMeasured * VOLTAGE_DIVIDER;
fCellPercentage = getCellLevel(dVoltageMeasured);

// WRITE VOLTAGE
floatToString(fTS_tmp, fBatteryVoltage_V , 2);
sprintf(cText_lcd, "%s   ", fTS_tmp);
lcd.setCursor(0, 1);
lcd.print(cText_lcd);
lcd.setCursor(7, 1);
lcd.print("V");

// WRITE PERCENTAGE
floatToString(fTS_tmp, fCellPercentage*100, 2);
sprintf(cText_lcd, "%s   ", fTS_tmp);
lcd.setCursor(9, 1);
lcd.print(cText_lcd);
lcd.setCursor(15, 1);
lcd.print("%%");
}

// ---------- STATE MACHINE ---------- 
  switch(gState)
  {
    case STATE_INITIAL:
      if(battConnected()) switchState(STATE_BATTERY_CONNECTED);
      break;
      
    case STATE_BATTERY_CONNECTED:
      if(!battConnected())
      {
        switchState(STATE_INITIAL);
        break;
      }
      if(battCharged())
      {
        switchState(STATE_BATTERY_CHARGED);
        break;
      }
      if(bButtonPressed && !battCharged())
      {
        if(consumeButtonShortPress()) switchState(STATE_BATTERY_CHARGING);
      }
      break;
      
    case STATE_BATTERY_CHARGING:
      if(!battConnected())
      {
        switchState(STATE_INITIAL);
        break;
      }
      if(battCharged())
      {
        switchState(STATE_BATTERY_CHARGED);
        break;
      }
      else if(bButtonPressed)
      {
        if(consumeButtonShortPress()) switchState(STATE_BATTERY_CONNECTED);
      }

      if(_loop & LOOP_MASK_16th)
      {
        char dots[MOVE_DOT_CNT] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
        for(int i = 0; i < move_dot_pos; ++i)
        {
          dots[i] = '.';
        }
        move_dot_pos++;
        if(move_dot_pos > MOVE_DOT_CNT) move_dot_pos = 1;
        
        lcd.setCursor(MOVE_DOT_LCD_POS, 0);
        lcd.print(dots);
      }
      break;
      
    case STATE_BATTERY_CHARGED:
      if(!battConnected())
      {
        switchState(STATE_INITIAL);
        break;
      }
      if(!battCharged())
      {
        switchState(STATE_BATTERY_CONNECTED);
        break;
      }
      break;
      
    case STATE_BATTERY_ERROR:
      break;
      
    default:
      break;
  }

digitalWrite(PIN_LED_RED, LOW);
digitalWrite(PIN_LED_GREEN, HIGH);
digitalWrite(PIN_LED_BLUE, LOW);

//digitalRead(PIN_BUTTON);

delay(t_sleep_time);  //500ms Pause
}

char * floatToString(char * cString, float fNumber, int nDigits)
{
  //char cString[20];
  for(int n = 0; n < fTS_LENGTH; ++n)
  {
    cString[n] = 0;
  }
  
  int i = 0;
  int nNumber = (int) fNumber;
  float fNumber_part = fNumber - nNumber;
  sprintf(cString, "%d", nNumber);
  do
  {
    nNumber = nNumber/10;
    ++i;
  }
  while (nNumber > 0);
  
  if( nDigits > 0)
  {
    int nNumber_part;
    cString[i++] = '.';
    for( int digit = 0; digit < nDigits; ++digit)
    {
      fNumber_part = fNumber_part*10;
      nNumber_part = fNumber_part;
      fNumber_part = fNumber_part - nNumber_part;
      
      cString[i++] = ASCII_NUMBER + nNumber_part;
      
    }    
    cString[i++] = '\0';
  }
  return cString;
}

float getCellLevel(float fVoltage)
{
  float result;
  float fIndex = (fVoltage - fBatteryLevelVoltageOffset) * fBatteryLevelVoltageMultiplier;
  int index = fIndex;
  
  if( index < 0 ) result = -1;
  else if( index >= nMaxIndex) result = 1;
  else
  {
    result = fBatteryLevel[index] + (fBatteryLevel[index+1] - fBatteryLevel[index]) * (fIndex-index);
  }
  return result;
}

void mngLoopTime()
{
  t_loop_last = t_loop;
  t_loop = millis() - t_startup;
  //Serial.print("t_loop: ");
  //Serial.print(t_loop);
  //Serial.print("\n");
  t_loopdelta = t_loop - t_loop_last;
  Serial.print("t_loopdelta: ");
  Serial.print(t_loopdelta);
  Serial.print("\n");
  if(t_loopdelta == 0) t_loopdelta_corrector = 0;
  else t_loopdelta_corrector = SLEEP_TIME - t_loopdelta;
  //Serial.print("t_loopdelta_corrector: ");
  //Serial.print(t_loopdelta_corrector);
  //Serial.print("\n");
  
  t_sleep_time = t_sleep_time + t_loopdelta_corrector;
  if(t_sleep_time > 2 * SLEEP_TIME) t_sleep_time = 0;
  //Serial.print("t_sleep_time: ");
  //Serial.print(t_sleep_time);
  //Serial.print("\n");
}

void wipeDisplayTop()
{
 lcd.setCursor(0, 0);
 lcd.print("                ");
}
void wipeDisplayBottom()
{
 lcd.setCursor(0, 1);
 lcd.print("                ");
}

void wipeDisplay()
{
  wipeDisplayTop();
  wipeDisplayBottom();
}

void startCharging()
{
  digitalWrite(PIN_RELAIS, HIGH);
}

void stopCharging()
{
  digitalWrite(PIN_RELAIS, LOW);
}

bool battConnected()
{
  return fCellPercentage >= 0;
}

bool battCharged()
{
  return battConnected && fCellPercentage >= CELL_PERCENTAGE_MAX;
}

bool consumeButtonShortPress()
{
  bool consume = false;
  if(bButtonPressed_short)
  {
    consume = !(bButtonPressed_short = false);
  }
  return consume;
}

bool consumeButtonLongPress()
{
  bool consume = false;
  if(bButtonPressed_long)
  {
    consume = !(bButtonPressed_long = false);
  }
  return consume;
}

void mgnButton()
{
  bButtonPressed_last = bButtonPressed;
  bButtonPressed = digitalRead(PIN_BUTTON) == 0;

  if(bButtonPressed)
  {
    if(!bButtonPressed_last)
    {
      t_button_pressed = millis();
    }
  }
  
  if(!bButtonPressed)
  {
    if(bButtonPressed_last)
    {
       if((millis() - t_button_pressed) < BUTTON_LONGPRESS_TIME)
       {
          bButtonPressed_short = true;
       }
       else
       {
          bButtonPressed_long = true;
       }       
    }
    if(!bButtonPressed_last)
    {
     if((millis() - t_button_pressed) > BUTTON_LONGPRESS_TIME*2)
     {
      consumeButtonShortPress();
      consumeButtonLongPress();
     }
    }
  }
}

bool switchState(state pState)
{
  bool stateSwitched = false;

  switch(pState)
    {
      case STATE_INITIAL:
        switch(gState)
        {
          case STATE_BATTERY_CONNECTED:
          case STATE_BATTERY_CHARGING:
          case STATE_BATTERY_CHARGED:
          case STATE_BATTERY_ERROR:
            gState = pState;
            wipeDisplayTop();
            lcd.setCursor(0, 0);
            lcd.print(TEXT_CONNECT_BAT);
            stopCharging();
            break;
          default:
            break;
        }
        break;
        
      case STATE_BATTERY_CONNECTED:
        switch(gState)
        {
          case STATE_INITIAL:
          case STATE_BATTERY_CHARGING:
          case STATE_BATTERY_CHARGED:
            gState = pState;
            wipeDisplayTop();
            lcd.setCursor(0, 0);
            lcd.print(TEXT_BAT_CONNECTED);
            stopCharging();
            break;
          default:
            break;
        }
        break;
        
      case STATE_BATTERY_CHARGING:
        switch(gState)
        {
          case STATE_BATTERY_CONNECTED:
            gState = pState;
            wipeDisplayTop();
            lcd.setCursor(0, 0);
            lcd.print(TEXT_BAT_CHARGING);
            startCharging();
            break;
          default:
            break;
        }
        break;
        
      case STATE_BATTERY_CHARGED:
        switch(gState)
        {
          case STATE_INITIAL:
          case STATE_BATTERY_CONNECTED:
            gState = pState;
            wipeDisplayTop();
            lcd.setCursor(0, 0);
            lcd.print(TEXT_BAT_CHARGED);
            stopCharging();
            break;
          default:
            break;
        }
        break;
        
      case STATE_BATTERY_ERROR:
        switch(gState)
        {
          case STATE_INITIAL:
          case STATE_BATTERY_CONNECTED:
          case STATE_BATTERY_CHARGING:
          case STATE_BATTERY_CHARGED:
            gState = pState;
            wipeDisplayTop();
            lcd.setCursor(0, 0);
            lcd.print(TEXT_BAT_ERROR);
            stopCharging();
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }  
    return stateSwitched;
}
