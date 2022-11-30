#include "M5CoreInk.h"
#include <Adafruit_NeoPixel.h>
#include "TimeFunctions.h"

Ink_Sprite PageSprite(&M5.M5Ink);

RTC_TimeTypeDef RTCtime, RTCTimeSave;
RTC_TimeTypeDef AlarmTime;
uint8_t second = 0, minutes = 0;

const int STATE_DEFAULT = 0;
const int STATE_EDIT_HOURS = 1;
const int STATE_EDIT_MINUTES = 2;
const int STATE_ALARM = 4;
const int STATE_ALARM_FINISHED = 5;
int program_state = STATE_DEFAULT;

unsigned long rtcTimer = 0;

unsigned long underlineTimer = 0;
bool underlineOn = false;

unsigned long ledBlinkTimer = 0;
bool ledBlinkOn = false;

unsigned long beepTimer = 0;

// Neopixel strip
int LED_PIN = 25;
int LED_COUNT = 30;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// IR Sensor
int sensorPin = 26;
int sensorVal = 0;
int finalVal = 0;
unsigned long sensorTimer = 0;
const int ledSensorPin = 33;  // 4-wire bottom connector input pin used by M5 units
int brightnessVal = 0;

const int rgbledPin = 32; 

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(
    3, // number of LEDs
    rgbledPin, // pin number
    NEO_GRB + NEO_KHZ800);  // LED type

void setup() {
  M5.begin(true, true, true);
  Serial.begin(9600);

  M5.rtc.GetTime(&RTCTimeSave);
  AlarmTime = RTCTimeSave;
  AlarmTime.Minutes = AlarmTime.Minutes + 0;  // set alarm 2 minutes ahead 
  M5.update();
  
  M5.M5Ink.clear();
  delay(500);

  checkRTC();
  PageSprite.creatSprite(0, 0, 200, 200);
  drawTime();

  strip.begin();           // initialize NeoPixel strip object 
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  pinMode(sensorPin, INPUT_PULLUP);     // declare sensor as input
}

void loop() {
  // check if data has been received on the Serial port:
  if(Serial.available() > 0)
  {
    // input format is "hh:mm" 
    char input[6];
    int charsRead = Serial.readBytes(input, 6);    // read 6 characters 
    if(charsRead == 6 && input[2] == ':') {
      int mm = int(input[4] - '0') + int(input[3] - '0')*10; 
      Serial.print("minutes: ");
      Serial.println(mm);
      int hr = int(input[1] - '0') + int(input[0] - '0')*10; 
      Serial.print("hours: ");
      Serial.println(hr);
      RTCtime.Minutes = mm;
      RTCtime.Hours = hr;
      M5.rtc.SetTime(&RTCtime);      
    }
    else {
      Serial.print("received wrong time format.. ");
      Serial.println(input);
    }
  }
  
  if( program_state == STATE_DEFAULT) {
    // state behavior: check and update time every second
    if(millis() > rtcTimer + 1000) {
      updateTime();
      drawTime();
      drawTimeToAlarm();      
      rtcTimer = millis();
    }
    // state behavior: read sensor and print its value to Serial port
    if(millis() > sensorTimer + 100) {
      sensorVal = analogRead(sensorPin);
      finalVal = map(sensorVal, 0, 4095, 0, 255);
      Serial.println(finalVal);
      if(finalVal > 100) {
        theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness        
      }
      sensorTimer = millis();    
    }
    // (OPTIONAL) state behavior: change RGB LEDs green level according to sensor value:
    for( int i=0; i<30; i++) {
      strip.setPixelColor(i, strip.Color(0, brightnessVal, 0)); 
    }
    strip.show(); 
    // state transition: MID button 
    if ( M5.BtnMID.wasPressed()) {
      AlarmTime = RTCtime;
      program_state = STATE_EDIT_MINUTES;
      Serial.println("program_state => STATE_EDIT_MINUTES");
    }
    // state transition: alarm time equals real time clock 
    if(AlarmTime.Hours == RTCtime.Hours && AlarmTime.Minutes == RTCtime.Minutes) {
      program_state = STATE_ALARM;
      Serial.println("program_state => STATE_ALARM");
    }
  }
  else if( program_state == STATE_EDIT_MINUTES) {
    // state behavior: blink underline under alarm minutes
    if( millis() > underlineTimer + 250 ) {
      underlineOn = !underlineOn;
      PageSprite.drawString(30, 20, "SET ALARM MINUTES:");
      drawAlarmTime();      
      if(underlineOn)
        PageSprite.FillRect(110, 110, 80, 6, 0); // underline minutes black
      else
        PageSprite.FillRect(110, 110, 80, 6, 1); // underline minutes white
      PageSprite.pushSprite();
      underlineTimer = millis();
    }
    // state transition: UP button to increase alarm minutes
    if( M5.BtnUP.wasPressed()) {
      Serial.println("BtnUP wasPressed!");
      if(AlarmTime.Minutes < 59) {
        AlarmTime.Minutes++;
        drawAlarmTime();
      }        
    }
    // another state transition: DOWN button to decrease alarm minutes
    else if( M5.BtnDOWN.wasPressed()) {
      Serial.println("BtnDOWN wasPressed!");
      if(AlarmTime.Minutes > 0) {
        AlarmTime.Minutes--;
        drawAlarmTime();
      }        
    }
    // another state transition: MID button to edit alarm hour
    else if (M5.BtnMID.wasPressed()) {
      PageSprite.FillRect(110, 110, 80, 6, 1); // underline minutes white
      program_state = STATE_EDIT_HOURS;
      Serial.println("program_state => STATE_EDIT_HOURS");
    }
  }
  else if(program_state == STATE_EDIT_HOURS) {
    // state behavior: blink underline under alarm hours
    if(millis() > underlineTimer + 250) {
      PageSprite.drawString(30, 20, " SET ALARM HOURS: ");
      underlineOn = !underlineOn;
      drawAlarmTime();
      if(underlineOn)
        PageSprite.FillRect(10, 110, 80, 6, 0); // underline hours black
      else
        PageSprite.FillRect(10, 110, 80, 6, 1); // underline hours white
      PageSprite.pushSprite();
      underlineTimer = millis();
    }
    // state behavior: increase alarm hour with UP button
    if( M5.BtnUP.wasPressed()) {
      Serial.println("BtnUP wasPressed!");
      if(AlarmTime.Hours < 23) {
        AlarmTime.Hours++;
        drawAlarmTime();
      }        
    }
    // state behavior: decrease alarm hour with DOWN button 
    else if( M5.BtnDOWN.wasPressed()) {
      Serial.println("BtnDOWN wasPressed!");
      if(AlarmTime.Hours > 0) {
        AlarmTime.Hours--;
        drawAlarmTime();
      }        
    }
    // state transition: MID button to go back to default state 
    else if (M5.BtnMID.wasPressed()) {
      //PageSprite.FillRect(10, 110, 80, 6, 1); // underline hours white
      M5.M5Ink.clear();
      PageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
      M5.rtc.GetTime(&RTCtime);
      drawTime();
      
      program_state = STATE_DEFAULT;
      Serial.println("program_state => STATE_DEFAULT");
    }
  }
  else if( program_state == STATE_ALARM) {
    sensorVal = analogRead(sensorPin);
    finalVal = map(sensorVal, 0, 4095, 0, 255);
    Serial.println(finalVal);
    // state behavior: check and update time every second
    if(millis() > rtcTimer + 1000) {
      M5.Speaker.setBeep(500,100);
      M5.Speaker.beep();
      updateTime();
      drawTime();
      drawTimeToAlarm();      
      rtcTimer = millis();
    }
    // (OPTIONAL) state behavior: blink RGB LEDs red every 500ms
    if(millis() > ledBlinkTimer + 500) {
      if(ledBlinkOn) {
        // turn all pixels red:
        for( int i=0; i<30; i++) {
          if(i%2 == 0) 
            strip.setPixelColor(i, strip.Color(255, 0, 0)); 
        }
        strip.show(); 
        ledBlinkOn = false;        
      }
      else {
        // turn all pixels off:
        for( int i=0; i<30; i++) {
          strip.setPixelColor(i, strip.Color(0, 0, 0)); 
        }
        strip.show(); 
        ledBlinkOn = true;
      }
      ledBlinkTimer = millis();
    }
    // state transition: top button press to finish alarm
    sensorVal = analogRead(sensorPin);
    finalVal = map(sensorVal, 0, 4095, 0, 255);
    Serial.println(finalVal);
    if ( finalVal > 100) {
      theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness    
      strip.show();  
      Serial.println("Basket was made!");
      M5.M5Ink.clear();
      PageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
      program_state = STATE_ALARM_FINISHED;
      Serial.println("program_state => STATE_ALARM_FINISHED");
    }
  }
  else if(program_state == STATE_ALARM_FINISHED) {
    M5.Speaker.end();
    theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness    
    strip.show();  
    // state behavior: check and update time every second
    if(millis() > rtcTimer + 1000) {
      updateTime();
      drawTime();
      PageSprite.drawString(50, 120, "ALARM IS OFF");
      PageSprite.pushSprite();    
      rtcTimer = millis();
    }
    // state transition: MID button 
    if ( M5.BtnMID.wasPressed()) {
      AlarmTime = RTCtime;
      program_state = STATE_EDIT_MINUTES;
      Serial.println("program_state => STATE_EDIT_MINUTES");
    }
  }
  M5.update();
}

void theaterChase(int color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}  

