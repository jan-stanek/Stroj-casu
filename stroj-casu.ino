#include <Adafruit_GFX.h>
#include <Wire.h>
#include <DHT.h>
#include <MCUFRIEND_kbv.h>
#include <DS1302.h>
#include <ModbusMaster.h>
#include <TouchScreen.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>

#include <Fonts/Liberation_Mono_10.h>

typedef struct {
  int x, y;
  bool touched;
} Point;

MCUFRIEND_kbv tft;


const int DHT_PIN = 22;
const int BUZZER_PIN = 23;
const int OBSTACLE_PIN = 24;


DHT dhtSensor(DHT_PIN, DHT11);

const int kCePin   = 25;  // Chip Enable
const int kIoPin   = 26;  // Input/Output
const int kSclkPin = 27;  // Serial Clock
DS1302 rtc(kCePin, kIoPin, kSclkPin);



const int debug = 1;

ModbusMaster node;


uint8_t YP = A2;  // must be an analog pin, use "An" notation!
uint8_t XM = A1;  // must be an analog pin, use "An" notation!
uint8_t YM = 6;   // can be a digital pin
uint8_t XP = 7;   // can be a digital pin

uint16_t TS_LEFT = 85;
uint16_t TS_RIGHT  = 940;
uint16_t TS_BOTTOM = 130;
uint16_t TS_TOP = 890;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define MINPRESSURE 30
#define MAXPRESSURE 10000

#define SWAP(a, b) {uint16_t tmp = a; a = b; b = tmp;}


void preTransmission() {}
void postTransmission() {}



#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F


void setup(void) {
  Serial.begin(115200);

  
  Wire.begin();

  tft.reset();

  uint16_t identifier = tft.readID();

  tft.begin(identifier);

  tft.setRotation(1);
  tft.invertDisplay(1);
  
  tft.fillScreen(BLACK);


  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(OBSTACLE_PIN, INPUT);

  analogWrite(3, 50);

  Serial2.begin(115200);

  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);  



int numLoops = 10000;
}
  bool rs485DataReceived = true;


String dayOfWeekName(int day) {
  switch (day) {
    case Time::kSunday:
      return "Nedele";
    case Time::kMonday:
      return "Pondeli";
    case Time::kTuesday:
      return "Utery";
    case Time::kWednesday:
      return "Streda";
    case Time::kThursday:
      return "Ctvrtek";
    case Time::kFriday: 
      return "Patek";
    case Time::kSaturday: 
      return "Sobota";
  }
}



Point readTouch() {
  TSPoint tp = ts.getPoint();

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);

  Point point;

  if (tp.z > MINPRESSURE) {
    point.touched = true;
    point.x = map(tp.x, TS_LEFT, TS_RIGHT, 0, tft.width());
    point.y = map(tp.y, TS_TOP, TS_BOTTOM, 0, tft.height());
  }
  else {
    point.touched = false;
  }
  
  return point;
}


int temperature, temperaturePrev, humidity, humidityPrev;
int hours, hoursPrev, minutes, minutesPrev, day, dayPrev, date, datePrev, month, monthPrev;
float bVoltage, bVoltagePrev, bCurrent, bRemaining, bRemainingPrev;
float lVoltage, lCurrent, lPower, lPowerPrev;
float pvVoltage, pvCurrent, pvPower, pvPowerPrev;
float generatedToday, generatedTotal, consumedToday, consumedTotal;
int screen;

 
void readTemperature() {
  temperaturePrev = temperature;
  temperature = dhtSensor.readTemperature();
}

void readHumidity() {
  humidityPrev = humidity;
  humidity = dhtSensor.readHumidity();
}

void readDateTime() {
  Time t = rtc.time();

  hoursPrev = hours;
  minutesPrev = minutes;
  dayPrev = day;
  datePrev = date;
  monthPrev = month;

  hours = t.hr;
  minutes = t.min;
  day = t.day;
  date = t.date;
  month = t.mon;
}

void readSolar() {
  uint8_t result;

  result = node.readInputRegisters(0x3100, 4);
  if (result == node.ku8MBSuccess) {
    pvPowerPrev = pvPower;
    
    pvVoltage = (long)node.getResponseBuffer(0x00)/100.0f;
    pvCurrent = (long)node.getResponseBuffer(0x01)/100.0f;
    pvPower = ((long)node.getResponseBuffer(0x03)<<16|node.getResponseBuffer(0x02))/100.0f;
  }
    
  result = node.readInputRegisters(0x331A, 2);
  if (result == node.ku8MBSuccess) {
    bVoltagePrev = bVoltage;
    
    bVoltage = (long)node.getResponseBuffer(0x00)/100.0f; 
    bCurrent = (long)node.getResponseBuffer(0x01)/100.0f; 
  } 
    
  result = node.readInputRegisters(0x310C, 4);
  if (result == node.ku8MBSuccess) {
    lPowerPrev = lPower;
    
    lVoltage = (long)node.getResponseBuffer(0x00)/100.0f;
    lCurrent = (long)node.getResponseBuffer(0x01)/100.0f;
    lPower = ((long)node.getResponseBuffer(0x03)<<16|node.getResponseBuffer(0x02))/100.0f;
  }  
    
  result = node.readInputRegisters(0x311A, 1);
  if (result == node.ku8MBSuccess) {
    bRemainingPrev = bRemaining;
    
    bRemaining = node.getResponseBuffer(0x00)/1.0f;
  } 
    
  result = node.readInputRegisters(0x3304, 2);
  if (result == node.ku8MBSuccess) {
    consumedToday = ((long)node.getResponseBuffer(0x01)<<16|node.getResponseBuffer(0x00))/100.0f;
  }
    
  result = node.readInputRegisters(0x330A, 4);
  if (result == node.ku8MBSuccess) {
    consumedTotal = ((long)node.getResponseBuffer(0x01)<<16|node.getResponseBuffer(0x00))/100.0f;
    generatedToday = ((long)node.getResponseBuffer(0x03)<<16|node.getResponseBuffer(0x02))/100.0f;
  }
    
  result = node.readInputRegisters(0x3312, 2);
  if (result == node.ku8MBSuccess) {
    generatedTotal = ((long)node.getResponseBuffer(0x01)<<16|node.getResponseBuffer(0x00))/100.0f;
  }  
}

void showMainScreen() {
  tft.fillScreen(BLACK);
    
  tft.setFont(&FreeMonoBold24pt7b);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
      
  char timeString[10];
  sprintf(timeString, "%02d:%02d", hours, minutes);
  String timeString2 = timeString;
  tft.setCursor((400 - timeString2.length() * 56) / 2, 10 + 62);
  tft.println(timeString);
    
    
  tft.setFont(&FreeMonoBold18pt7b);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
    
  String dateString = dayOfWeekName(day) + " " + String(date) + ". " + String(month) + ".";
  tft.setCursor((400 - dateString.length() * 21) / 2, 10 + 62 + 10 + 23);
  tft.println(dateString);
    
    
  tft.setFont(&FreeMonoBold18pt7b);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
    
  String tempString = String(temperature) + " C" + "  " + String(humidity) + "%";
  tft.setCursor((400 - tempString.length() * 21) / 2, 10 + 62 + 10 + 23 + 10 + 23);
  tft.println(tempString);

  tft.setFont(&FreeMonoBold12pt7b);
  tft.setCursor((400 - tempString.length() * 21) / 2 + String(temperature).length() * 21 + 2, 10 + 62 + 10 + 23 + 10 + 12);
  tft.println("o");

  tft.setFont(&FreeMono9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 200);
  tft.println(String(pvPower) + "W" + " " + String(bVoltage) + "V" + " " + String(bRemaining) + "%" + " " + String(lPower) + "W");
    
}

void touchMainScreen() {
  Point point = readTouch();
  if (point.touched) {
    screen = 2;
    refreshScreen();
  }
}

void showSolarScreen() {
  tft.fillScreen(BLACK);
  
  tft.setFont(&FreeMono12pt7b);
  tft.setTextSize(1);
  tft.setCursor(0, 30);
      
  tft.println("PANEL:   " + String(pvVoltage) + "V" + " " + String(pvCurrent) + "A" + " " + String(pvPower) + "W");
  tft.println("BATERIE: " + String(bVoltage) + "V" + " " + String(bCurrent) + "A" + " " + String(bRemaining) + "%");
  tft.println("ZATEZ:   " + String(lVoltage) + "V" + " " + String(lCurrent) + "A" + " " + String(lPower) + "W");
  tft.println("SPOTREBOVANO: " + String(consumedToday) + "/" + String(consumedTotal) + "kWh");
  tft.println("VYROBENO: " + String(generatedToday) + "/" + String(generatedTotal) + "kWh");

  tft.drawRect(200, 200, 60, 30, BLUE);
  tft.setCursor(200, 220);
  tft.print("ZPET");
}

void touchSolarScreen() {
  Point point = readTouch();
  if (point.touched && point.x >= 200 && point.x <= 260 && point.y >= 200 && point.y <= 230) {
    screen = 1;
    refreshScreen();
  }
}

void refreshScreen() {
  switch(screen) {
    case 1:
      showMainScreen();
      break;
    case 2:
      showSolarScreen();
      break;
  }
}

void touchScreen() {
  switch(screen) {
    case 1:
      touchMainScreen();
      break;
    case 2:
      touchSolarScreen();
      break;
  }
}

void loop(void) {
  screen = 1;

  delay(1000);

  while (true) {
    for (int i = 0; i < 100; i++) {
      touchScreen();
      
      if (i == 0) {
  //      bool obstacle = digitalRead(OBSTACLE_PIN);
  //  if(!obstacle) {
  //    tone(BUZZER_PIN, 5000);
  //    delay(50);
  //    noTone(BUZZER_PIN);
  //  }
  
        readTemperature();
        readHumidity();
        readDateTime();
        readSolar();
  
  //      writeSD();
  
        refreshScreen();
      }
  
      delay(100);
    }
  }


 
}


