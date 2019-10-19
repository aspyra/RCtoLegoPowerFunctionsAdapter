//libraries
#include "PFControl.h"
#include "Blink.h"
#include "EEPROM.h"

//#define debug //serial on/off

//constants
#define NoOfChannels 3
#define CH1 0
#define CH2 1
#define CH3 2
#define LED_RED 10
#define LED_BLUE LED_BUILTIN
#define buttonPin 11                                                                                                                                  

//variables
volatile uint16_t reading[NoOfChannels];
uint16_t reading_safe[NoOfChannels];
uint16_t maxSig[NoOfChannels], minSig[NoOfChannels], zeroSig[NoOfChannels];
int16_t normalized[NoOfChannels];
bool conn[2*4][NoOfChannels];
bool rev[2*4][NoOfChannels];
bool chIRActive[4];
bool chRCActive[NoOfChannels];
bool multipleCars;

//functions
void Ch1(){
  static bool new_read;
  static uint16_t risetime;
  if(digitalRead(CH1)){
    //rising
    risetime = micros();
    new_read = true;
  }
  else{
    //falling
    if(new_read){
      reading[0] = micros()-risetime;
      new_read = false;
    }
  }
}

void Ch2(){
  static bool new_read;
  static uint16_t risetime;
  if(digitalRead(CH2)){
    //rising
    risetime = micros();
    new_read = true;
  }
  else{
    //falling
    if(new_read){
      reading[1] = micros()-risetime;
      new_read = false;
    }
  }
}

void Ch3(){
  static bool new_read;
  static uint16_t risetime;
  if(digitalRead(CH3)){
    //rising
    risetime = micros();
    new_read = true;
  }
  else{
    //falling
    if(new_read){
      reading[2] = micros()-risetime;
      new_read = false;
    }
  }
}

void loadDefaults(){
    for(uint8_t i = 0; i < NoOfChannels; ++i){
      maxSig[i] = 0;
      zeroSig[i] = 1500;
      minSig[i] = 4000;
  } 
}

void backupReadings(){
  for(uint8_t i = 0; i < NoOfChannels; ++i)
        reading_safe[i] = reading[i];
}

void normalizeAll(){
  for(uint8_t i = 0; i < NoOfChannels; ++i){
     //maxSig, minSig, zeroSig, sig turn into -100, 100
  if(reading_safe[i] > zeroSig[i])
    normalized[i] = int16_t(uint32_t(reading_safe[i] - zeroSig[i])*100/(maxSig[i] - zeroSig[i]));
  else
    normalized[i] = -int8_t(uint32_t(zeroSig[i] - reading_safe[i])*100/(zeroSig[i] - minSig[i]));
  #ifdef debug
    Serial.print("CH" + String(i+1) + ": " + String(normalized[i]) + "  ");
  #endif
  }
  #ifdef debug
    Serial.println("");
  #endif
}

void waitForDepress(uint8_t pin){
  uint8_t counter = 0;
  while(counter != 200){
    if(digitalRead(pin))
      ++counter;
    else
      counter = 0;
    }
}

void findMinMax(){
  for(uint8_t i = 0; i < NoOfChannels; ++i){
    if(maxSig[i]<reading_safe[i] && reading_safe[i]<2500) //make sure signal makes sense
      maxSig[i] = reading_safe[i];
    if(minSig[i]>reading_safe[i] && reading_safe[i]>500) //make sure signal makes sense
      minSig[i] = reading_safe[i];
    #ifdef debug
      Serial.print("CH" + String(i+1) + ": " + String(minSig[i]) + ' ' + String(reading_safe[i]) + ' ' + String(maxSig[i]) + "  ");
    #endif
  }
  #ifdef debug
    Serial.println("");
  #endif
}

void setup() {
  #ifdef debug
    delay(10000);
    Serial.begin(115200);
  #endif

  //led_ set-up
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);

  //channels set-up
  pinMode(CH1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CH1), Ch1, CHANGE);
  pinMode(CH2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CH2), Ch2, CHANGE);
  pinMode(CH3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CH3), Ch3, CHANGE);
  
  //button setup
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  //if button pressed -> setup!
  if(!digitalRead(buttonPin)){
    //setup operation!
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    #ifdef debug
      Serial.println("Setup mode");
    #endif
    waitForDepress(buttonPin);
    //1. Learn max, min, middle values for each channel
    loadDefaults();
    while(digitalRead(buttonPin)){
      backupReadings();
      findMinMax();
    }
    //2. Button pressed again - save neutral values!
    for(int8_t i = 0; i < NoOfChannels; ++i)
        zeroSig[i] = reading_safe[i];
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_BLUE, LOW);
    waitForDepress(buttonPin);
    //3. Now, each channel and canal should be set up on its own!
    ledBlinkSetup();
    for(int8_t chIR = 0; chIR < 4; ++chIR){
      bool loopsie = true;
      for(bool blue = true; loopsie; blue = false){
        //flash the led showing channel and canal
        if(blue){
          blinkLED(LED_BLUE, chIR+1);
          digitalWrite(LED_RED, LOW);
        }else{
          blinkLED(LED_RED, chIR+1);
          digitalWrite(LED_BLUE, LOW);
          loopsie = false;
        }
        #ifdef debug
          Serial.print("Setting up IR channel " + String(chIR+1));
          if(blue) Serial.println(" blue ");
          else Serial.println(" red ");
        #endif
        //let user test the correct controls
        while(digitalRead(buttonPin)){
          backupReadings();
          normalizeAll();
          //all controls should be nonnegative, to find the correct direction
          int16_t sum = 0;
          for(uint8_t i = 0; i < NoOfChannels; ++i){
            if(normalized[i] > 0)
              sum += normalized[i];
            else
              sum -= normalized[i];
          }
          //now, connect the sum to the channel that is being set up
          if(blue)
            sendPF(chIR, scaleToPF(sum), 0);
          else
            sendPF(chIR, 0, scaleToPF(sum));
          delay(1);
        }
        //save the data
        int8_t mod;
        if(blue)
          mod = 0;
        else
          mod = 4;
        for(uint8_t i = 0; i < NoOfChannels; ++i){
          if(normalized[i]>50 || normalized[i] < -50){
            conn[chIR+mod][i] = true;
            if(normalized[i] < -50)
              rev[chIR+mod][i] = true;
          }
        }
        waitForDepress(buttonPin);
      }
    }
    blinkLED(0, 0);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_BLUE, LOW);
    #ifdef debug
      Serial.println("Saving to EEPROM...");
    #endif
    ///all data is set - save it to EEPROM and continue norma operation
    saveSettings();
  }
  else{
    #ifdef debug
      Serial.println("loading from EEPROM...");
    #endif
    ///normal operation - load stored setup in EEPROM!
    loadSettings();
    
  }
  #ifdef debug
    Serial.println("Calculating unused channels");
  #endif
  calculateUnused();
  #ifdef debug
    Serial.println("Entering normal mode");
  #endif
  digitalWrite(LED_RED, HIGH);

  multipleCars = false;
}



void loop() {
  //normal operation
  for(int8_t chIR = 0; chIR < 4; ++chIR){
    backupReadings();
    normalizeAll();
    if(chIRActive[chIR]){
      int16_t red_sum = 0, blue_sum = 0;
      for(uint8_t i = 0; i < NoOfChannels; ++i){
        if(chRCActive[i]){
          //blue
          if(conn[chIR][i]){
            if(rev[chIR][i])
              blue_sum -= normalized[i];
            else
              blue_sum += normalized[i];
          }
          //red
          if(conn[chIR+4][i]){
            if(rev[chIR+4][i])
              red_sum -= normalized[i];
            else
              red_sum += normalized[i];
          }
        }
      }
      sendPF(chIR, scaleToPF(blue_sum), scaleToPF(red_sum));
      if(multipleCars)
        delay(40);
      else
        delay(1);
    }
  }
  if(!multipleCars && !digitalRead(buttonPin)){
    multipleCars = true;
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_BLUE, HIGH);
  }
}

/////////////////////////////////////////////////////////////////////
/* TO BE SAVED:
uint16_t maxSig[NoOfChannels], minSig[NoOfChannels], zeroSig[NoOfChannels]; 2 + 2 + 2
bool conn[2*4][NoOfChannels]; 1
bool rev[2*4][NoOfChannels]; 1
*/

void EEPROMWrite16bit(int16_t address, int16_t value){
 byte low = ((value >> 0) & 0xFF);
 byte high = ((value >> 8) & 0xFF);
 EEPROM.write(address, low);
 EEPROM.write(address + 1, high);
}

void saveSettings(){
  for(uint8_t i = 0; i < NoOfChannels; ++i){
    int16_t address = i*8;
    EEPROMWrite16bit(address, maxSig[i]);
    EEPROMWrite16bit(address+2, zeroSig[i]);
    EEPROMWrite16bit(address+4, minSig[i]);
    byte c = 0;
    byte r = 0;
    for(uint8_t shift = 0; shift < 8; ++shift){
      c |= (conn[shift][i]<<shift);
      r |= (rev[shift][i]<<shift);
    }
    EEPROM.write(address+6, c);
    EEPROM.write(address+7, r);
  }
}

void loadSettings(){
  for(uint8_t i = 0; i < NoOfChannels; ++i){
    int16_t address = i*8;
    byte low, high;
    low = EEPROM.read(address);
    high = EEPROM.read(address+1);
    maxSig[i] = (uint16_t(high)<<8 | uint16_t(low));
    low = EEPROM.read(address+2);
    high = EEPROM.read(address+3);
    zeroSig[i] = (uint16_t(high)<<8 | uint16_t(low));
    low = EEPROM.read(address+4);
    high = EEPROM.read(address+5);
    minSig[i] = (uint16_t(high)<<8 | uint16_t(low));
    byte c = EEPROM.read(address+6);
    byte r = EEPROM.read(address+7);
    for(uint8_t shift = 0; shift < 8; ++shift){
      conn[shift][i] = (c>>shift) & 0x01;
      rev[shift][i] = (r>>shift) & 0x01;
    }
  }
}

void calculateUnused(){
  //finding chIRActive[4] chRCActive[NoOfChannels]
  for(uint8_t i = 0; i < NoOfChannels; ++i){
    chRCActive[i]=false;
    for(uint8_t j = 0; j < 8; ++j){
      if(conn[j][i]==true){
         chRCActive[i]=true;
         break;
      }
    }
  }

  for(uint8_t i = 0; i < 4; ++i){
    chIRActive[i]=false;
    for(uint8_t j = 0; j < NoOfChannels; ++j){
      if(conn[i][j]==true || conn[i+4][j]==true){
         chIRActive[i]=true;
         break;
      }
    }
  }
}
