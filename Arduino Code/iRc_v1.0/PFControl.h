#include <IRremote.h>

IRsend IR;

int8_t scaleToPF(int16_t sum){
  int16_t output = sum;
  if(output > 100)
    output = 100;
  else if(output < -100)
    output = -100;
  return int8_t((output+100)*15/201-7); //MAGIC NUMBERS, YAAY!
}

uint8_t translateToLegoIR(int8_t signal_to_translate, bool if_float = false){ //takes values between -7 and 7 and returns correct bits for the sending
/* 
0000 Float
0001 PWM forward step 1
0010 PWM forward step 2
0011 PWM forward step 3
0100 PWM forward step 4
0101 PWM forward step 5
0110 PWM forward step 6
0111 PWM forward step 7
1000 Brake
1001 PWM backward step 7
1010 PWM backward step 6
1011 PWM backward step 5
1100 PWM backward step 4
1101 PWM backward step 3
1110 PWM backward step 2
1111 PWM backward step 1
*/
  switch(signal_to_translate){
    default: //float/brake
      if(if_float)
        return 0x00; //0000
      return 0x08; // 1000
    case 1:
      return 0x01; //0001
    case 2:
      return 0x02; //0010
    case 3:
      return 0x03; //0011
    case 4:
      return 0x04; //0100
    case 5:
      return 0x05; //0101
    case 6:
      return 0x06; //0110
    case 7:
      return 0x07; //0111
    case -1:
      return 0x0F; //1111
    case -2:
      return 0x0E; //1110
    case -3:
      return 0x0D; //1101
    case -4:
      return 0x0C; //1100
    case -5:
      return 0x0B; //1011
    case -6:
      return 0x0A; //1010
    case -7:
      return 0x09; //1001
  }
}

uint8_t ComboPWM_IRChannel(int IR_Channel){
  switch(IR_Channel){
    default:
      return 0x04;
    case 1:
      return 0x05;
    case 2:
      return 0x06;
    case 3:
      return 0x07;
  }
}

void sendPF(int8_t IR_Channel, int8_t sigBLUE, int8_t sigRED, bool if_float = false){
  uint8_t IRChannel = ComboPWM_IRChannel(IR_Channel);
  uint8_t IRsigA = translateToLegoIR(sigBLUE, if_float);
  uint8_t IRsigB = translateToLegoIR(sigRED, if_float);
  //Create the checksum bit
  uint16_t dataSend = 0x0F ^ IRChannel ^ IRsigA ^ IRsigB;
  //add the channel and motor data
  dataSend += (IRChannel << 12);
  dataSend += (IRsigB << 8);
  dataSend += (IRsigA << 4);
  //send the data
  IR.sendLegoPowerFunctions(dataSend, false);
}
