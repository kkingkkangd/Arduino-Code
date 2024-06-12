#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#ifdef __AVR__
#include <avr/power.h> // 16 MHz Adafruit Trinket을 위한 필요 코드
int vib = 13;
#endif
unsigned long startTime = 0;
bool conditionMet = false;
unsigned long helStartTime = 0;
bool helConditionMet = false;
#define PIN 8 // NeoPixel 데이터 라인을 위한 핀 정의 몸왼쪽 
#define NUMPIXELS 11 // 일반적인 NeoPixel 링 크기

#define PIN2 9 // NeoPixel 데이터 라인을 위한 핀 정의 몸오른쪽
#define NUMPIXELS2 11 // 일반적인 NeoPixel 링 크기

#define PIN3 10 // NeoPixel 데이터 라인을 위한 핀 정의  헬멧
#define NUMPIXELS3 8 // 일반적인 NeoPixel 링 크기

#define DELAYVAL 500 // NeoPixel 애니메이션을 위한 지연 시간

const int BUTTON_PIN = 7; // Button pin
SoftwareSerial mySerial(2, 3); // TX2번, RX3번
String input;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2(NUMPIXELS2, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels3(NUMPIXELS3, PIN3, NEO_GRB + NEO_KHZ800);

int bodyon = 0;
unsigned long lastDebounceTime = 0; // for button debouncing
unsigned long debounceDelay = 50; // debounce time in milliseconds
int lastButtonState = HIGH;
int buttonState;
//가속도  아날로그 01 
// ADXL345 register address
#define POWER_CTL 0x2D
#define DATA_FORMAT 0x31
#define X_axis 0x32
#define Y_axis 0x34
#define Z_axis 0x36

#define Range_2g 0
#define Range_4g 1
#define Range_8g 2
#define Range_16g 3

#define I2C_Address 0x53
int prevX, prevY, prevZ; 

void setup() {
  Wire.begin();
  Serial.begin(9600);
  mySerial.begin(9600);
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
  #endif
  pixels.begin();
  pixels.clear();
  pixels.show(); // Initialize all pixels to 'off'
  
  pixels2.begin();
  pixels2.clear();
  pixels2.show(); // Initialize all pixels to 'off'
  
  pixels3.begin();
  pixels3.clear();
  pixels3.show(); // Initialize all pixels to 'off'
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Init_ADXL345(Range_2g); 
  
  // 초기 축 값 읽기
  prevX = Read_Axis(X_axis);
  prevY = Read_Axis(Y_axis);
  prevZ = Read_Axis(Z_axis);

  pinMode(vib, INPUT);    // 센서 핀 입력 설정
}

void loop() {
  // HC-12 모듈로부터 데이터 수신 및 처리
  if (mySerial.available() > 0) { // Read from HC-12 and send to serial monitor
    input = readString(mySerial);
    Serial.println(input);
  }

  // 받은 입력이 설정한 값과 동일한지 확인하여 LED를 제어
  if (input == "13" && !helConditionMet) {
    pixels3.setBrightness(100);
    for (int i = 0; i < NUMPIXELS3; i++) {
      pixels3.setPixelColor(i, pixels3.Color(0, 150, 0));
    }
    pixels3.show();
    Serial.println("Hel GREEN");

    helStartTime = millis();
    helConditionMet = true;
    input="asd";
  }

  if (helConditionMet && millis() - helStartTime >= 5000) {
    // 5초가 지났으면 LED 끄기
    for (int i = 0; i < NUMPIXELS3; i++) {
      pixels3.setPixelColor(i, pixels3.Color(0, 0, 0));
    }
    pixels3.show();
    Serial.println("Hel OFF");

    helConditionMet = false;
    delay(20);
  }

  // 버튼 입력에 대한 채터링 방지
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // 버튼 상태가 안정화되었을 때만 처리
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        if (bodyon == 0) {
          mySerial.print("13\n");
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 150, 0));
          }
          for (int i = 0; i < NUMPIXELS2; i++) {
            pixels2.setPixelColor(i, pixels2.Color(0, 150, 0));
          }
          pixels.show(); // 업데이트된 픽셀 색상을 하드웨어에 전송
          pixels2.show(); // 업데이트된 픽셀 색상을 하드웨어에 전송
          Serial.println("BODYLED ON");
          bodyon = 1;
        } else {
          mySerial.print("13\n");
          for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          }
          for (int i = 0; i < NUMPIXELS2; i++) {
            pixels2.setPixelColor(i, pixels2.Color(0, 0, 0));
          }
          pixels.show(); // 업데이트된 픽셀 색상을 하드웨어에 전송
          pixels2.show(); // 업데이트된 픽셀 색상을 하드웨어에 전송
          Serial.println("BODYLED OFF");
          bodyon = 0;
        }
      }
    }
  }

  lastButtonState = reading;

  // 시리얼 모니터로부터 입력된 정보를 HC-12 모듈로 보내기
  if (Serial.available() > 0) {
    String output = Serial.readStringUntil('\n');
    mySerial.println(output);
  }

  int currX = Read_Axis(X_axis);
  int currY = Read_Axis(Y_axis);
  int currZ = Read_Axis(Z_axis);

  //3축 출력
  // Serial.print("X: ");
  // Serial.print(currX);
  // Serial.print("  Y: ");
  // Serial.print(currY);
  // Serial.print("  Z: ");
  // Serial.print(currZ);
  // Serial.println();

  // 변화량 계산
  int deltaX = abs(currX - prevX);
  int deltaY = abs(currY - prevY);
  int deltaZ = abs(currZ - prevZ);

  long measurement = TP_init();
  // 변화량이 7 미만일 때 LED 켜기
  if (deltaX < 7 && deltaY < 7 && deltaZ < 7 && measurement == 0) {
    if (!conditionMet) {
      // 조건이 처음으로 만족되면 타이머 시작
      startTime = millis();
      conditionMet = true;
    } else if (millis() - startTime >= 5000 && bodyon == 0) {
      // 조건이 만족되고 5초가 경과했으면
      mySerial.print("13\n");
      Serial.println("warning");
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 150, 0));
      }
      for (int i = 0; i < NUMPIXELS2; i++) {
        pixels2.setPixelColor(i, pixels2.Color(0, 150, 0));
      }
      pixels.show(); // 업데이트된 픽셀 색상을 하드웨어에 전송
      pixels2.show(); // 업데이트된 픽셀 색상을 하드웨어에 전송
      Serial.println("BODYLED ON");
      bodyon = 1;
    }
  } else {
    // 조건이 만족되지 않으면 타이머와 상태를 리셋
    conditionMet = false;
  }

  // 이전 값 업데이트
  prevX = currX;
  prevY = currY;
  prevZ = currZ;
  // Serial.println(TP_init()); //진동센서 test

  delay(100);
}

String readString(SoftwareSerial& serial) {
  String result = "";
  while (serial.available() > 0) {
    char ch = serial.read();
    if (isPrintable(ch) && ch != '\n' && ch != '\r') {
      result += ch;
    }
  }
  return result;
}

// I2C 인터페이스를 통해 축을 읽음
int Read_Axis(byte a) {
  int data;

  Wire.beginTransmission(I2C_Address); 
  Wire.write(a); 
  Wire.endTransmission(); 

  Wire.beginTransmission(I2C_Address); 
  Wire.requestFrom(I2C_Address, 2);

  if (Wire.available()) {
    data = (int)Wire.read();
    data = data | (Wire.read() << 8);
  } else {
    data = 0;
  }

  Wire.endTransmission();
  return data;
}

// ADXL345 초기화
void Init_ADXL345(byte r) {
  Wire.beginTransmission(I2C_Address);

  // 감도설정
  Wire.write(DATA_FORMAT);
  Wire.write(r);
  Wire.endTransmission();

  // 측정모드로 전환
  Wire.beginTransmission(I2C_Address);
  Wire.write(POWER_CTL);
  Wire.write(0x08);
  Wire.endTransmission();
}

long TP_init() {
  long measurement = pulseIn(vib, HIGH);
  return measurement;
}
