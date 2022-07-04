#include "src/Cmd/Cmd.h"
#include <SoftwareSerial.h>

#define DEVICE "VapAuSa_Controller"
#define MODEL "A"
#define VERSION "1.0"

/*-----( Declare Constants and Pin Numbers )-----*/

#define VALVE_UPDATE_INTERVAL 1000

// software serial
#define RS485_RX  4  //Serial Receive pin
#define RS485_TX 10  //Serial Transmit pin
#define RS485_EN A0  //RS485 Direction control
#define RS485_BAUDRATE 1200

// shift registers
#define PIN_DS    2  //data in pin on the 75HC595
#define PIN_ST_CP 8  //latch pin of the 75HC595
#define PIN_SH_CP 9  //clock pin of the 75HC595
#define PIN_OE 7    // output enable pin of the 75HC595
#define SHIFT_REGISTER_NUM 3  // number of shift registers
#define REGISTER_PIN_NUM SHIFT_REGISTER_NUM * 8

/*-----( Declare objects and variables )-----*/
SoftwareSerial RS485 (RS485_RX, RS485_TX);
boolean registers[REGISTER_PIN_NUM];
unsigned int latestValveUpdate;

/*------------------------------------*/
struct valveSlotType{
  byte box;
  byte slot;  
};

//------- valve control stuff ----------

valveSlotType primaryValve   = {0,1};
valveSlotType groupValve   = {0,0};

//.............................
valveSlotType parse_valveSlot(char *arg){
  valveSlotType valveSlot;
  byte value;
  char *pch;
  pch = strtok(arg,"#");
  for(byte i=0; i<2; i++){
    if(pch == NULL) break;
    value =  atoi(pch);
    
    if(i==0){
      valveSlot.slot = value;
      valveSlot.box = 0;
    }else{
      valveSlot.box = valveSlot.slot;
      valveSlot.slot = value;
    }   
    pch = strtok(NULL, "#");
  }
  return valveSlot;
}

//.........................................
void update_valves(){

  for(int i = REGISTER_PIN_NUM - 1; i >=  0; i--){
     registers[i] = (primaryValve.box==0 & (primaryValve.slot-1)==i) ||
                    (groupValve.box==0   & (groupValve.slot-1)==i);
  }

  digitalWrite(PIN_ST_CP, LOW);
  for(int i = REGISTER_PIN_NUM - 1; i >=  0; i--){
    digitalWrite(PIN_SH_CP, LOW);
    int val = registers[i];
    digitalWrite(PIN_DS, val);
    digitalWrite(PIN_SH_CP, HIGH);
  }
  digitalWrite(PIN_ST_CP, HIGH);

  digitalWrite(RS485_EN, HIGH);
  delay(50);
  RS485.print("valve ");
  RS485.print(primaryValve.box);
  RS485.print('#');
  RS485.print(primaryValve.slot);
  RS485.print("\r\n");
  RS485.print(' ');
  RS485.print(groupValve.box);
  RS485.print('#');
  RS485.print(groupValve.slot);
  RS485.print("\r\n");
  RS485.flush();
  digitalWrite(RS485_EN, LOW);

  latestValveUpdate = millis();  
}

//.........................................
void get_activeValve(){
  Serial.print("valve>");
  Serial.print(primaryValve.box);
  Serial.print('#');
  Serial.print(primaryValve.slot);  

  Serial.print(' ');
  Serial.print(groupValve.box);
  Serial.print('#');
  Serial.print(groupValve.slot);
  
  Serial.println();
}

void cmd_valve(int arg_cnt, char **args){

  // in case no second argument was passed, just print the active valve
  if(arg_cnt < 2){
    get_activeValve();
    return;
  }
  primaryValve = parse_valveSlot(args[1]);
  
  if(arg_cnt > 2)
    groupValve = parse_valveSlot(args[2]);
  
  update_valves();  
}

/*-------------------------------------*/


//==========================================

void cmd_identify(int arg_cnt, char **args){
  Serial.print(DEVICE);
  Serial.print(' ');
  Serial.print(MODEL);
  Serial.print(' ');
  Serial.println(VERSION);  
}

void setup(){

  // first measure: disable shift register outputs
  pinMode(PIN_OE, OUTPUT);
  digitalWrite(PIN_OE, HIGH);

  cmdInit(9600);  
  cmdAdd("?", cmd_identify);
  cmdAdd("valve", cmd_valve);

  pinMode(RS485_EN, OUTPUT);
  RS485.begin(RS485_BAUDRATE); // Start the software serial port
 
  pinMode(PIN_DS, OUTPUT);
  pinMode(PIN_ST_CP, OUTPUT);
  pinMode(PIN_SH_CP, OUTPUT);
 
  update_valves();
  digitalWrite(7, LOW); // finally enable shift register outputs

   cmd_identify(0,NULL);
  
}


void loop(){
  cmdPoll();
}
