#include <U8g2lib.h>
#include <EEPROM.h>

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R2, /* clock=*/ D0, /* data=*/ D1, /* CS=*/ D2, /* reset=*/ 10);

const int upButton = D4;
const int downButton = D5;
const int setButton = D6;
const int pumpPin = D8;
const int waterFlowPin = D7;
const int waterFullPin = A0;
const int waterEmptyPin = D3;

float currentSettingLiters = 10.0;
int currentMode = 0;
int pumpState = LOW;
float waterFlowRate = 0.0;
float pumpLiters = 0.0;
int j = 0;
unsigned long setButtonPressStartTime = 0;
bool setButtonLongPressStarted = false;

void setup() {
  u8g2.begin();
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(setButton, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
  pinMode(waterFlowPin, INPUT_PULLUP);
  pinMode(waterFullPin, INPUT_PULLUP);
  pinMode(waterEmptyPin, INPUT_PULLUP);
  
  EEPROM.begin(512);
  EEPROM.get(0, waterFlowRate);
  EEPROM.get(sizeof(float), pumpLiters);
  EEPROM.get(sizeof(float) * 2, currentSettingLiters);
  EEPROM.end();
}

int readButton(int buttonPin) {
  int buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    delay(50);
    while (digitalRead(buttonPin) == HIGH) {}
    return 1;
  }
  return 0;
}

void displayStatus() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 12);
    u8g2.print("Flow Rate: ");
    u8g2.print(waterFlowRate, 2);
    u8g2.setCursor(0, 24);
    u8g2.print("Pump Liters: ");
    u8g2.print(pumpLiters, 2);
  } while (u8g2.nextPage());
}

void resetValues() {
  pumpLiters = 0.0;
  currentSettingLiters = 0.0;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 12);
  u8g2.print("All values reset");
  u8g2.sendBuffer();
  delay(1000);
}

void loop() {
  int upButtonPressed = readButton(upButton);
  int downButtonPressed = readButton(downButton);
  int setButtonPressed = readButton(setButton);
  int waterFull = digitalRead(waterFullPin);
  int waterEmpty = digitalRead(waterEmptyPin);
  
  waterFlowRate = pulseIn(waterFlowPin, HIGH, 1000) / 1000.0;

  if (setButtonPressed) {
    if (!setButtonLongPressStarted) {
      setButtonLongPressStarted = true;
      setButtonPressStartTime = millis();
    }
  } else {
    if (setButtonLongPressStarted && (millis() - setButtonPressStartTime >= 1000)) {
      resetValues();
    }
    setButtonLongPressStarted = false;
  }

  if (currentMode == 0) {
    if (upButtonPressed) {
      currentSettingLiters += 0.1;
    }
    if (downButtonPressed) {
      currentSettingLiters -= 0.1;
    }
    if (currentSettingLiters < 0.0) {
      currentSettingLiters = 0.0;
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 12);
    u8g2.print("Volume: ");
    u8g2.print(currentSettingLiters, 1);
    u8g2.print(" L");
    u8g2.sendBuffer();
  } else {
    if (upButtonPressed) {
      pumpState = HIGH;
      j = 1;
    }
    if (downButtonPressed) {
      pumpState = LOW;
      j = 0;
    }
    
    if (j == 0) {
      if (waterEmpty == LOW) {
        digitalWrite(pumpPin, HIGH);
      } else if (waterFull == LOW) {
        digitalWrite(pumpPin, LOW);
      }
    } else {
      if (waterFlowRate < 0.1) {
        digitalWrite(pumpPin, HIGH);
      } else {
        digitalWrite(pumpPin, LOW);
      }
    }
    
    displayStatus();
    digitalWrite(pumpPin, pumpState);

    if (pumpState == HIGH && pumpLiters > currentSettingLiters) {
      pumpState = LOW;
      pumpLiters = 0.0;
      digitalWrite(pumpPin, pumpState);
    }
  }
  
  // Save waterFlowRate, pumpLiters, and currentSettingLiters to EEPROM
  EEPROM.begin(512);
  EEPROM.put(0, waterFlowRate);
  EEPROM.put(sizeof(float), pumpLiters);
  EEPROM.commit();
  EEPROM.end();
}
