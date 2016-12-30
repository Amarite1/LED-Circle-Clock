#include <Adafruit_NeoPixel.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <Wire.h>

#define RING_PIN 6
#define RIGHT_PIN 5
#define LEFT_PIN 4
#define ENTER_PIN 3  

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, RING_PIN, NEO_GRB + NEO_KHZ800);

float lightIntensity = 1; //0 to 1, where 0 is off
int inputMode  = 0; //0 for no input, 1 for set time, 2 for general menu
int setMode = 0; //0 for hour, 1 for min, 2 for sec
int timeInput[] = {0, 0, 0, 0};
bool displayOn = true;
bool debounce = false;
int ls = -1;

void setup() {

  pinMode(RIGHT_PIN, INPUT);
  pinMode(LEFT_PIN, INPUT);
  pinMode(ENTER_PIN, INPUT);
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  setSyncProvider(RTC.get); //get time from the RTC
  setSyncInterval(60); //sync with RTC every x seconds

  if(timeStatus() != timeSet){
    for(int i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(255*lightIntensity, 0, 0));
    }
    strip.show();
  }else{
    for(int i=0; i< strip.numPixels(); i++) {
       strip.setPixelColor(i, strip.Color(0, 255*lightIntensity, 0));
    }
    strip.show();
  }

  delay(2000);
  
}

void loop(){

  if(inputMode == 0){
    if(digitalRead(RIGHT_PIN) == HIGH && digitalRead(LEFT_PIN) == HIGH){
      inputMode = 1;
      timeInput[0] = hour();
      if(timeInput[0] >= 12) timeInput[0] = timeInput[0]-12;
      timeInput[1] = minute();
      timeInput[2] = second();
      rainbowCycle(1, 2);
      return;
    }

    if(digitalRead(ENTER_PIN) == HIGH){
      if(!debounce){
        displayOn = !displayOn;
        debounce=true;
      }
    }else{
      debounce = false;
    }

    if(displayOn){
      //show time on ring
      int h = hour();
      int m = minute();
      int s = second();

      if(ls != s){
        ls=s;
        if(h >= 9 && h <20) lightIntensity = 1;
        else if((h >= 20 && h < 22) || (h >= 7 && h < 9)) lightIntensity = 0.5;
        else lightIntensity = 0.2;
  
        if(h>=12) h=h-12;
        int hp = h*5+floor(m/12);
        int hi = 255;
        
        for(int i=0; i< strip.numPixels(); i++) {
           int r = 0;
           int g = 0;
           int b = 0;
           //if(i>=h*5 && i<=h*5+4) r=255;
           if(i==hp)r=hi;
           if(i==m) g=255;
           if(i==s) b=255;
           strip.setPixelColor(i, strip.Color(r*lightIntensity, g*lightIntensity, b*lightIntensity));
        }
        strip.show();
      }
    }else{
      for(int i=0; i< strip.numPixels(); i++) {
         strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
      strip.show();
    }

    
    
  }else if(inputMode == 1){ //setting the time
    if(digitalRead(RIGHT_PIN) == HIGH){
      timeInput[setMode] ++;
      if(setMode != 0 && setMode != 3 && timeInput[setMode] == 60){
        timeInput[setMode] = 0;
      }else if(setMode == 0 && timeInput[0] == 12){
        timeInput[0] = 0;
      }else if(setMode == 3 && timeInput[3] == 2){
        timeInput[3] = 0;
      }
    }
    if(digitalRead(LEFT_PIN) == HIGH){
      timeInput[setMode] --;
      if(setMode != 0 && setMode != 3 && timeInput[setMode] == -1){
        timeInput[setMode] = 59;
      }else if(setMode == 0 && timeInput[0] == -1){
        timeInput[0] = 11;
      }else if(setMode == 3 && timeInput[3] == -1){
        timeInput[3] = 1;
      }
    }
    if(digitalRead(ENTER_PIN) == HIGH){
      if(setMode == 3){
        setMode = 0;

        if(timeInput[3] == 1) timeInput[0] = timeInput[0] + 12;
        
        tmElements_t tm;
        tm.Hour = timeInput[0];
        tm.Minute = timeInput[1];
        tm.Second = timeInput[2];

        time_t t = makeTime(tm);
        RTC.set(t);
        setTime(t);
        inputMode = 0;
      }else{
        setMode++;
      }
    }

    if(setMode != 3){
      for(int i=0; i< strip.numPixels(); i++) {
         int r = 0;
         int g = 0;
         int b = 0;
         if(i>=timeInput[0]*5 && i<= timeInput[0]*5+4) r=255;
         if(i==timeInput[1]) g=255;
         if(i==timeInput[2]) b=255;
         strip.setPixelColor(i, strip.Color(r*lightIntensity, g*lightIntensity, b*lightIntensity));
      }
      strip.show();
    }else{
      if(timeInput[3] == 0){
        for(int i=0; i< strip.numPixels(); i++) {
           strip.setPixelColor(i, strip.Color(255*lightIntensity, 0, 0));
        }
      }else if(timeInput[3] == 1){
        for(int i=0; i< strip.numPixels(); i++) {
           strip.setPixelColor(i, strip.Color(0, 0, 255*lightIntensity));
        }
      }
      
      strip.show();
    }
    
    delay(100);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, int cycles) {
  uint16_t i, j;

  for(j=0; j<256*cycles; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
