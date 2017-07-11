#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>  
#include <SdFat.h>  
#include <Wire.h>
#include <RTClib.h>
#include <RFID.h>
#include <DHT.h>
#include <ModbusMaster.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>


//hodiny
RTC_DS3231 rtc;
char daysOfTheWeek[7][8] = {"Nedele", "Pondeli", "Utery", "Streda", "Ctvrtek", "Patek", "Sobota"};


//teplomer
const int DHT_PIN = 33;
DHT dhtSensor(DHT_PIN, DHT11);


//senzor prekazky
const int OBSTACLE_PIN = 35;


//display
MCUFRIEND_kbv tft;

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


//dotykovy display
const uint8_t YP = A2;  // must be an analog pin, use "An" notation!
const uint8_t XM = A1;  // must be an analog pin, use "An" notation!
const uint8_t YM = 6;   // can be a digital pin
const uint8_t XP = 7;   // can be a digital pin

const uint16_t TS_LEFT = 85;
const uint16_t TS_RIGHT  = 940;
const uint16_t TS_BOTTOM = 130;
const uint16_t TS_TOP = 890;

#define MINPRESSURE 50
#define MAXPRESSURE 10000

#define SWAP(a, b) {uint16_t tmp = a; a = b; b = tmp;}

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


//epsolar komunikace
ModbusMaster node;


//SD
#define SD_CS 5
SdFat sd;
SdFile file;


//RFID
#define SS_PIN 23
#define RST_PIN 31
RFID rfid(SS_PIN, RST_PIN);


//promenne pro teplomer a vlhkomer
int temperature, temperaturePrev, humidity, humidityPrev;

//promenne pro datum a cas
int hours, hoursPrev, minutes, minutesPrev, day, date, datePrev, month, year;

//promenne pro data z regulatoru
float bVoltage, bVoltagePrev, bCurrent, bCurrentPrev, bRemaining, bRemainingPrev;
float lVoltage, lVoltagePrev, lCurrent, lCurrentPrev, lPower, lPowerPrev;
float pvVoltage, pvVoltagePrev, pvCurrent, pvCurrentPrev, pvPower, pvPowerPrev;
float generatedToday, generatedTotal, consumedToday, consumedTotal;

//promenne pro dotykovy display
int x, y;
bool touched;

//aktualni obrazovka
int screen;
bool screenChanged = true;

int selectedYear = -1;
int selectedMessage = -1;

#define MAIN_SCREEN 0     //hlavni obrazovka
#define SOLAR_SCREEN 1    //obrazovka s daty z regulatoru
#define MENU_SCREEN 2     //menu pro vyber roku
#define YEAR_SCREEN 3     //zobrazeni roku
#define MESSAGE_SCREEN 4  //zobrazeni zpravy od vedce

Adafruit_GFX_Button yearButtons[11];
Adafruit_GFX_Button messageButtons[10];
Adafruit_GFX_Button backButton;

//pocitadlo cyklu pro cekani
int counter;

//roky
String years[] = {"ERROR", "-7420", "122", "864", "1158", "1356", "1423", "1827", "1942", "3017", "2017"};

//zpravy
String messages[] = {
  "Co nejrychleji se   dostante pryc!",
  "Ziskejte od pralidi zvlastni sosku,     ktera je z          budoucnosti.",
  "Hledejte jmena,     ktera do teto doby  nepatri.",
  "Ziskejte kus        meteoritu, ktery    pred casem spadl v  tomto obdobi.",
  "Zucastnete se       korunovace krale    Vladislava II.",
  "Karel IV. schoval v jedne ze svych      staveb palivo do    stroje casu,       najdete jej!",
  "Mimozemstane        pomohli krizakum    porazit husity,     musite napravit    historii.",
  "Palivo do stroje    casu ziskate v      nedalekem meste.",
  "Udrzujte spojeni s  Londynem. Pripravte atentat na risskeho protektora.",
  "Najdete me ve       vezeni, pospeste    si!"
};

//ikony
const unsigned char panel [] PROGMEM = {
  0x0,0x0,0xf,0xfc,0x9,0x24,0x12,0x48,0x12,0x48,0x1f,0xf8,0x24,0x90,0x24,
  0x90,0x24,0x90,0x7f,0xe0,0x49,0x20,0x49,0x20,0x92,0x40,0xff,0xc0
};

const unsigned char baterie [] PROGMEM = {
  0x0,0x0,0x0,0x0,0x0,0x0,0x70,0x38,0xff,0xfc,0x80,0x4,0x80,0x4,0xff,0xfc,
  0xff,0xfc,0xff,0xfc,0xff,0xfc,0xff,0xfc,0xff,0xfc,0xff,0xfc
};

const unsigned char zatez [] PROGMEM = {
  0x0,0x0,0x7,0x80,0x8,0x40,0x10,0x20,0x20,0x10,0x23,0x10,0x24,0x90,0x24,
  0x90,0x14,0xa0,0xf,0xc0,0x7,0x80,0x7,0x80,0x7,0x80,0x3,0x0
};

const unsigned char teplota [] PROGMEM = {
  0x0,0x0,0x0,0x0,0x70,0x0,0x0,0x88,0x0,0x0,0x88,0x0,0x0,0x88,0x0,0x0,0x88,
  0x0,0x0,0xa8,0x0,0x0,0xa8,0x0,0x0,0xa8,0x0,0x0,0xa8,0x0,0x0,0xa8,0x0,0x0,
  0xa8,0x0,0x0,0xa8,0x0,0x1,0x24,0x0,0x2,0x72,0x0,0x4,0xf9,0x0,0x5,0xfd,0x0,
  0x5,0xfd,0x0,0x5,0xfd,0x0,0x4,0xf9,0x0,0x2,0x72,0x0,0x1,0x4,0x0,0x0,0xf8,
  0x0
};

const unsigned char vlhkost [] PROGMEM = {
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x20,0x0,0x0,0x20,0x0,0x0,0x70,
  0x0,0x0,0x70,0x0,0x0,0xf8,0x0,0x0,0xf8,0x0,0x1,0xfc,0x0,0x3,0xfe,0x0,
  0x3,0xfe,0x0,0x7,0xff,0x0,0x7,0xff,0x0,0xf,0xff,0x80,0xf,0xff,0x80,0xf,
  0xff,0x80,0xf,0xff,0x80,0x7,0xff,0x0,0x7,0xff,0x0,0x3,0xfe,0x0,0x1,0xfc,
  0x0,0x0,0x70,0x0
};

void preTransmission() {}

void postTransmission() {}

void readTouch() {
  TSPoint tp = ts.getPoint();

  //nastaveni pinu zpet
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);

  if (tp.z > MINPRESSURE) {
    touched = true;
    x = map(tp.x, TS_LEFT, TS_RIGHT, 0, tft.width());
    y = map(tp.y, TS_TOP, TS_BOTTOM, 0, tft.height());
  }
  else {
    touched = false;
  }
}

void readTemperature() {
  temperaturePrev = temperature;
  temperature = dhtSensor.readTemperature();
}

void readHumidity() {
  humidityPrev = humidity;
  humidity = dhtSensor.readHumidity();
}

void readDateTime() {
  hoursPrev = hours;
  minutesPrev = minutes;
  datePrev = date;

  DateTime now = rtc.now();
  
  hours = now.hour();
  minutes = now.minute();
  day = now.dayOfTheWeek();
  date = now.day();
  month = now.month();
  year = now.year();
}

void readSolar() {
  uint8_t result;

  result = node.readInputRegisters(0x3100, 4);
  if (result == node.ku8MBSuccess) {
    pvVoltagePrev = pvVoltage;
    pvCurrentPrev = pvCurrent;
    pvPowerPrev = pvPower;
    
    pvVoltage = (int)node.getResponseBuffer(0x00)/100.0f;
    pvCurrent = (int)node.getResponseBuffer(0x01)/100.0f;
    pvPower = ((long)node.getResponseBuffer(0x03)<<16|node.getResponseBuffer(0x02))/100.0f;
  }
  else {
    return;
  }
    
  result = node.readInputRegisters(0x331A, 2);
  if (result == node.ku8MBSuccess) {
    bVoltagePrev = bVoltage;
    bCurrentPrev = bCurrent;
    
    bVoltage = (int)node.getResponseBuffer(0x00)/100.0f; 
    bCurrent = (int)node.getResponseBuffer(0x01)/100.0f; 
  } 
    
  result = node.readInputRegisters(0x310C, 4);
  if (result == node.ku8MBSuccess) {
    lVoltagePrev = lVoltage;
    lCurrentPrev = lCurrent;
    lPowerPrev = lPower;
    
    lVoltage = (int)node.getResponseBuffer(0x00)/100.0f;
    lCurrent = (int)node.getResponseBuffer(0x01)/100.0f;
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

void changeScreen(int newScreen) {
  if (newScreen != screen) {
    screen = newScreen;
    screenChanged = true;
    tft.fillScreen(BLACK);
    refreshScreen();
  }
}

void refreshScreen() {
  switch(screen) {
    case MAIN_SCREEN:
      refreshMainScreen();
      break;
    case SOLAR_SCREEN:
      refreshSolarScreen();
      break;
    case MENU_SCREEN:
      refreshMenuScreen();
      break;
    case YEAR_SCREEN:
      refreshYearScreen();
      break;
    case MESSAGE_SCREEN:
      refreshMessageScreen();
      break;  
  }
}

void refreshMainScreen() {
  if (screenChanged) {
    tft.setTextColor(WHITE);
    
    tft.setFont(&FreeMonoBold24pt7b);
    tft.setTextSize(2);
    tft.setCursor((400 - 5 * 56) / 2 + 2 * 56, 10 + 62);
    tft.print(":");      

    tft.setFont(&FreeMonoBold18pt7b);
    tft.setTextSize(1);

    tft.drawBitmap((400 - 11 * 21) / 2, 10 + 62 + 15 + 23 + 15 + 1, teplota, 21, 23, WHITE);
    tft.setCursor((400 - 11 * 21) / 2 + 4 * 21, 10 + 62 + 15 + 23 + 15 + 23);
    tft.print("C");
    
    tft.drawBitmap((400 - 11 * 21) / 2 + 7 * 21, 10 + 62 + 15 + 23 + 15 + 1, vlhkost, 21, 23, WHITE);
    tft.setCursor((400 - 11 * 21) / 2 + 10 * 21, 10 + 62 + 15 + 23 + 15 + 23);
    tft.print("%");

    tft.setFont(&FreeMonoBold12pt7b);
    tft.setCursor((400 - 11 * 21) / 2 + 3 * 21 + 2, 10 + 62 + 15 + 23 + 15 + 12);
    tft.print("o");
  
    tft.drawBitmap((400 - 19 * 16) / 2, 170, panel, 14, 14, WHITE);
    tft.drawBitmap((400 - 19 * 16) / 2, 190, baterie, 14, 14, WHITE);
    tft.drawBitmap((400 - 19 * 16) / 2, 210, zatez, 14, 14, WHITE);  
  }  
  
  if (counter % 10 == 0 || screenChanged) {
    tft.setTextColor(WHITE);
    
    if (hours != hoursPrev || screenChanged) {
      tft.setFont(&FreeMonoBold24pt7b);
      tft.setTextSize(2);
      char hoursString[3];
      sprintf(hoursString, "%02d", hours);
      tft.fillRect((400 - 5 * 56) / 2, 10, 56 * 2, 66, BLACK);
      tft.setCursor((400 - 5 * 56) / 2, 10 + 62);
      tft.print(String(hoursString));
    }
    
    if (minutes != minutesPrev || screenChanged) {
      tft.setFont(&FreeMonoBold24pt7b);
      tft.setTextSize(2);
      char minutesString[3];
      sprintf(minutesString, "%02d", minutes);
      tft.fillRect((400 - 5 * 56) / 2 + 3 * 56, 10, 56 * 2, 66, BLACK);
      tft.setCursor((400 - 5 * 56) / 2 + 3 * 56, 10 + 62);
      tft.print(String(minutesString));
    }

    if (date != datePrev || screenChanged) {
      tft.setFont(&FreeMonoBold18pt7b);
      tft.setTextSize(1);
      String dateString = String(daysOfTheWeek[day]) + " " + String(date) + ". " + String(month) + ".";
      tft.fillRect(0, 10 + 62 + 15, 400, 24, BLACK);
      tft.setCursor((400 - dateString.length() * 21) / 2, 10 + 62 + 15 + 23);
      tft.print(dateString);
    }

    if (temperature != temperaturePrev || screenChanged) {
      tft.setFont(&FreeMonoBold18pt7b);
      tft.setTextSize(1);
      char temperatureStr[3];
      dtostrf(temperature, 2, 0, temperatureStr);
      tft.fillRect((400 - 11 * 21) / 2 + 21, 10 + 62 + 15 + 23 + 15, 2*21, 24, BLACK);
      tft.setCursor((400 - 11 * 21) / 2 + 21, 10 + 62 + 15 + 23 + 15 + 23);
      tft.print(String(temperatureStr));
    }

    if (humidity != humidityPrev || screenChanged) {
      tft.setFont(&FreeMonoBold18pt7b);
      tft.setTextSize(1);
      char humidityStr[3];
      dtostrf(humidity, 2, 0, humidityStr);
      tft.fillRect((400 - 11 * 21) / 2 + 8 * 21, 10 + 62 + 15 + 23 + 15, 2*21, 24, BLACK);
      tft.setCursor((400 - 11 * 21) / 2 + 8 * 21, 10 + 62 + 15 + 23 + 15 + 23);
      tft.print(String(humidityStr));
    }

    if (pvVoltage != pvVoltagePrev || pvCurrent != pvCurrentPrev || pvPower != pvPowerPrev || screenChanged) {
      tft.setFont(&FreeMono12pt7b);
      tft.setTextSize(1);
      char pvVoltageStr[5];
      dtostrf(pvVoltage, 4, 1, pvVoltageStr);
      char pvCurrentStr[5];
      dtostrf(pvCurrent, 4, 2, pvCurrentStr);
      char pvPowerStr[5];
      dtostrf(pvPower, 4, 1, pvPowerStr);
      tft.fillRect((400 - 19 * 16) / 2 + 2 * 16, 170, 17*16, 15, BLACK);
      tft.setCursor((400 - 19 * 16) / 2 + 2 * 16, 184);
      tft.print(String(pvVoltageStr) + "V" + "  " + String(pvCurrentStr) + "A" + "  " + String(pvPowerStr) + "W");
    }

    if (bVoltage != bVoltagePrev || bCurrent != bCurrentPrev || bRemaining != bRemainingPrev || screenChanged) {
      tft.setFont(&FreeMono12pt7b);
      tft.setTextSize(1);
      char bVoltageStr[5];
      dtostrf(bVoltage, 4, 1, bVoltageStr);
      char bCurrentStr[6];
      dtostrf(bCurrent, 5, 2, bCurrentStr);
      char bRemainingStr[5];
      dtostrf(bRemaining, 4, 0, bRemainingStr);
      tft.fillRect((400 - 19 * 16) / 2 + 2 * 16, 190, 17*16, 15, BLACK);
      tft.setCursor((400 - 19 * 16) / 2 + 2 * 16, 204);
      tft.print(String(bVoltageStr) + "V" + " " + String(bCurrentStr) + "A" + "  " + String(bRemainingStr) + "%");
    }

    if (lVoltage != lVoltagePrev || lCurrent != lCurrentPrev || lPower != lPowerPrev || screenChanged) {
      tft.setFont(&FreeMono12pt7b);
      tft.setTextSize(1);
      char lVoltageStr[5];
      dtostrf(lVoltage, 4, 1, lVoltageStr);
      char lCurrentStr[5];
      dtostrf(lCurrent, 4, 2, lCurrentStr);
      char lPowerStr[5];
      dtostrf(lPower, 4, 1, lPowerStr);
      tft.fillRect((400 - 19 * 16) / 2 + 2 * 16, 210, 17*16, 15, BLACK);
      tft.setCursor((400 - 19 * 16) / 2 + 2 * 16, 224);
      tft.print(String(lVoltageStr) + "V" + "  " + String(lCurrentStr) + "A" + "  " + String(lPowerStr) + "W");
    }
  }

  if (screenChanged)
    screenChanged = false;
}

void refreshSolarScreen() {
  if (screenChanged) {
    tft.setFont();
    
    backButton.initButton(&tft, 352, 22 + 200, 95, 35, 2, DARKGREEN, WHITE, "ZPET", 2);
    backButton.drawButton(false);
  }
  
  if (counter % 50 == 0 || screenChanged) {
    tft.setFont(&FreeMono12pt7b);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    tft.setCursor(17, 40);
    char pvVoltageStr[5];
    dtostrf(pvVoltage, 4, 1, pvVoltageStr);
    char pvCurrentStr[5];
    dtostrf(pvCurrent, 4, 2, pvCurrentStr);
    char pvPowerStr[5];
    dtostrf(pvPower, 4, 1, pvPowerStr);
    tft.fillRect(0, 26, 400, 15, BLACK);
    tft.print("PANEL:   " + String(pvVoltageStr) + "V" + " " + String(pvCurrentStr) + "A" + " " + String(pvPowerStr) + "W");

    tft.setCursor(17, 70);
    char bVoltageStr[5];
    dtostrf(bVoltage, 4, 1, bVoltageStr);
    char bCurrentStr[6];
    dtostrf(bCurrent, 5, 2, bCurrentStr);
    char bRemainingStr[4];
    dtostrf(bRemaining, 3, 0, bRemainingStr);
    tft.fillRect(0, 56, 400, 15, BLACK);
    tft.print("BATERIE: " + String(bVoltageStr) + "V" + " " + String(bCurrentStr) + "A" + " " + String(bRemainingStr) + "%");

    tft.setCursor(17, 100);
    char lVoltageStr[5];
    dtostrf(lVoltage, 4, 1, lVoltageStr);
    char lCurrentStr[5];
    dtostrf(lCurrent, 4, 2, lCurrentStr);
    char lPowerStr[5];
    dtostrf(lPower, 4, 1, lPowerStr);
    tft.fillRect(0, 86, 400, 15, BLACK);
    tft.print("ZATEZ:   " + String(lVoltageStr) + "V" + " " + String(lCurrentStr) + "A" + " " + String(lPowerStr) + "W");

    tft.setCursor(17, 130);
    tft.fillRect(0, 116, 400, 15, BLACK);
    tft.print("SPOTREBOVANO: " + String(consumedToday) + "/" + String(consumedTotal) + "kWh");
    
    tft.setCursor(17, 160);
    tft.fillRect(0, 146, 400, 15, BLACK);    
    tft.print("VYROBENO:     " + String(generatedToday) + "/" + String(generatedTotal) + "kWh");
  }

  if (screenChanged)
    screenChanged = false;

  readTouch();

  if (touched) {
    if (backButton.contains(x, y))
      changeScreen(MAIN_SCREEN);
  }
}

void refreshMenuScreen() {
  if (screenChanged) {
    screenChanged = false;

    tft.setFont();

    for (int i = 0; i < 6; i++) {
      yearButtons[i].initButton(&tft, 52, 22 + i * 40, 95, 35, 2, BLUE, WHITE, years[i].c_str(), 2);
      yearButtons[i].drawButton(selectedYear == i);
    }

    for (int i = 0; i < 5; i++) {
      yearButtons[i+6].initButton(&tft, 152, 22 + i * 40, 95, 35, 2, BLUE, WHITE, years[i+6].c_str(), 2);
      yearButtons[i+6].drawButton(selectedYear == i + 6);
    }

    for (int i = 0; i < 6; i++) {
      messageButtons[i].initButton(&tft, 252, 22 + i * 40, 95, 35, 2, RED, WHITE, years[i].c_str(), 2);
      messageButtons[i].drawButton(selectedMessage == i);
    }

    for (int i = 0; i < 4; i++) {
      messageButtons[i+6].initButton(&tft, 352, 22 + i * 40, 95, 35, 2, RED, WHITE, years[i+6].c_str(), 2);
      messageButtons[i+6].drawButton(selectedMessage == i + 6);
    }

    backButton.initButton(&tft, 352, 22 + 200, 95, 35, 2, DARKGREEN, WHITE, "ZPET", 2);
    backButton.drawButton(false);
  }

  readTouch();

  if (touched) {
    for (int i = 0; i < 11; i++) {
      if (yearButtons[i].contains(x, y))
        yearButtons[i].press(true);
      else
        yearButtons[i].press(false);
      
      if (yearButtons[i].justPressed()) {
        tft.setFont();
        
        if (selectedYear != -1) {
          yearButtons[selectedYear].drawButton(false);
        }
        
        if (selectedMessage != -1) {
          messageButtons[selectedMessage].drawButton(false);
          selectedMessage = -1;
        }
  
        selectedYear = i;
        yearButtons[i].drawButton(true);
      }
    }
  
    for (int i = 0; i < 10; i++) {
      if (messageButtons[i].contains(x, y))
        messageButtons[i].press(true);
      else
        messageButtons[i].press(false);
        
      if (messageButtons[i].justPressed()) {
        tft.setFont();
        
        if (selectedYear != -1) {
          yearButtons[selectedYear].drawButton(false);
          selectedYear = -1;
        }
        
        if (selectedMessage != -1) {
          messageButtons[selectedMessage].drawButton(false);
        }
        
        selectedMessage = i;
        messageButtons[i].drawButton(true);
      }
    }

    if (backButton.contains(x, y))
      changeScreen(MAIN_SCREEN);
  }
}

void refreshYearScreen() {
  if (screenChanged) {
    screenChanged = false;

    tft.setFont(&FreeMonoBold24pt7b);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor((400 - years[selectedYear].length() * 56) / 2, 140);
    tft.print(years[selectedYear]);
  }

  if (digitalRead(OBSTACLE_PIN) == HIGH)
    changeScreen(MAIN_SCREEN);
}

void refreshMessageScreen() {
  if (screenChanged) {
    screenChanged = false;

    tft.setFont(&FreeMono18pt7b);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(0, 40);
    tft.print(messages[selectedMessage]);
  }
  
  if (digitalRead(OBSTACLE_PIN) == HIGH)
    changeScreen(MAIN_SCREEN);
}

void writeSD() {
  if (sd.exists("data.csv")) {
    file.open("data.csv", FILE_WRITE);
    
    char datetime[18];
    sprintf(datetime, "%04d-%02d-%02d %02d:%02d", year, month, date, hours, minutes);
    
    file.println(String(datetime) + ";" + String(temperature) + ";" + String(humidity) + ";" + 
            String(pvVoltage) + ";" + String(pvCurrent) + ";" + String(pvPower) + ";" +
            String(bVoltage) + ";" + String(bCurrent) + ";" + String(bRemaining) + ";" + 
            String(lVoltage) + ";" + String(lCurrent) + ";" + String(lPower));
    
    file.close();

    if (screen == MAIN_SCREEN)
      tft.fillRect(350, 10, 28, 14, BLACK);
  }
  else if(screen == MAIN_SCREEN) {
    tft.setFont(&FreeMono12pt7b);
    tft.setTextSize(1);
    tft.setTextColor(RED);
    tft.setCursor(350, 24);
    tft.print("SD");
  }
}

void setup(void) {
  Wire.begin();

  rtc.begin();
    
  tft.reset();
  uint16_t identifier = tft.readID();
  tft.begin(identifier);
  tft.setRotation(1);
  tft.invertDisplay(1);
  tft.fillScreen(BLACK);

  pinMode(OBSTACLE_PIN, INPUT);

  analogWrite(3, 50);

  Serial2.begin(115200);
  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);  

  screen = MAIN_SCREEN;
  delay(1000);

  sd.begin(SD_CS, SD_SCK_MHZ(50));

  rfid.init();
}

void loop(void) {
  for (counter = 0; counter < 600; counter++) {
    if (counter % 10 == 0) {
      readDateTime();
      readTemperature();
      readHumidity();
      readSolar();
    }
    
    if (counter == 0)
      writeSD();

    if (counter % 10 == 0 && rfid.isCard()) {
      rfid.readCardSerial();
      if (rfid.serNum[0] == 53)
        changeScreen(MENU_SCREEN);
      else if (rfid.serNum[0] == 136)
        changeScreen(SOLAR_SCREEN);
    }
    else if (digitalRead(OBSTACLE_PIN) == LOW) {
      if (selectedYear != -1)
        changeScreen(YEAR_SCREEN);
      else if (selectedMessage != -1)
        changeScreen(MESSAGE_SCREEN);
    }
      
    refreshScreen();

    if (minutes != minutesPrev)
      break;
    
    delay(100);
  }
}


