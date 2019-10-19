#define blinkTime 2125
#define shortPause 2687
#define longPause 15625

uint8_t blink_setting;
uint8_t blink_counter;
uint8_t blink_pin;
bool blink_on;

void ledBlinkSetup() {
  TCCR3B=0b00001101;//(1<<WGM32)|(1<<CS32)|(1<<CS30); //ctc, 1024 prescaler
  TCCR3A=0;
  blink_on = false;
}

void blinkLED(int8_t pin, uint8_t count){
  TIMSK3&=~(1<<OCIE3A); //off
  if(!(pin < 0 || count == 0)){
    blink_pin = pin;
    blink_setting = count;
    blink_counter = 0;
    blink_on = false;
    OCR3A = 1;
    TIMSK3|=(1<<OCIE3A); //on
    TCNT3 = 0;
  }
}

ISR(TIMER3_COMPA_vect){ 
  if(blink_on){
    blink_on = false;
    digitalWrite(blink_pin, LOW);
    if(blink_counter <= 1){
      blink_counter = blink_setting;
      OCR3A = longPause;
    }
    else{
      --blink_counter;
      OCR3A = shortPause;
    }
  }else{
    digitalWrite(blink_pin, HIGH);
    blink_on = true;
    OCR3A = blinkTime;
  }
}
