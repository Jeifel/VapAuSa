#include "src/Cmd/Cmd.h"

#define DEVICE "VapAuSa_Extension"
#define MODEL "A"
#define VERSION "1.0"

/*-----( Declare Constants and Pin Numbers )-----*/

#define RS485_BAUDRATE 1200
#define RS485_EN 10

// shift registers
#define PIN_DS    2  //data in pin on the 75HC595
#define PIN_ST_CP 8  //latch pin of the 75HC595
#define PIN_SH_CP 9  //clock pin of the 75HC595
#define PIN_OE 7    // output enable pin of the 75HC595
#define SHIFT_REGISTER_NUM 3  // number of shift registers
#define REGISTER_PIN_NUM SHIFT_REGISTER_NUM * 8
boolean registers[REGISTER_PIN_NUM];

#define INPUT_BUFFER_SIZE 32
char inputBuffer[INPUT_BUFFER_SIZE];

#define ID_PIN A3
#define ANALOG_ID_TOLERANCE 7
#define ID_TABLE_SIZE 16
const int analogIdVals[ID_TABLE_SIZE] = {   0,  45,  90, 129, 182, 214, 245, 272,
                                          330, 352, 374, 393, 420, 436, 453, 468};

/*------------------------------------*/
struct valveSlotType{
  byte box;
  byte slot;  
};
//--------------------------------------------
byte get_id(){
  int analogReading = analogRead(ID_PIN);
  for(byte i=0; i<ID_TABLE_SIZE; i++){
    if(analogReading >= (analogIdVals[i] - ANALOG_ID_TOLERANCE)&
       analogReading <= (analogIdVals[i] + ANALOG_ID_TOLERANCE)){
          return(i+1);          
       }
  }
  return(0);
}

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

  byte id = get_id();

  for(int i = REGISTER_PIN_NUM - 1; i >=  0; i--){
     registers[i] = (primaryValve.box==id & (REGISTER_PIN_NUM - primaryValve.slot)==i) ||
                    (groupValve.box==id   & (REGISTER_PIN_NUM - groupValve.slot)==i);
  }

  digitalWrite(PIN_ST_CP, LOW);
  for(int i = REGISTER_PIN_NUM - 1; i >=  0; i--){
    digitalWrite(PIN_SH_CP, LOW);
    int val = registers[i];
    digitalWrite(PIN_DS, val);
    digitalWrite(PIN_SH_CP, HIGH);
  }
  digitalWrite(PIN_ST_CP, HIGH);
}

void cmd_valve(int arg_cnt, char **args){

  // in case no second argument was passed, return
  if(arg_cnt < 2){
    return;
  }
  primaryValve = parse_valveSlot(args[1]);
  
  if(arg_cnt > 2)
    groupValve = parse_valveSlot(args[2]);
  
  update_valves();  
}

/*-------------------------------------*/

void openRS485(){
  digitalWrite(13,HIGH);
  digitalWrite(RS485_EN,HIGH);
  delay(50);
}

void closeRS485(){
  Serial.flush();
  digitalWrite(RS485_EN,LOW);
  digitalWrite(13,LOW);
}



//==========================================

void cmd_ignore(int arg_cnt, char **args){
  return;
}

void cmd_identify(int arg_cnt, char **args){

  if(arg_cnt>1){
    byte targetID = atoi(args[1]);
    if(targetID!=get_id()) return;
    
    openRS485();    
    Serial.print("< ");
  }
  
  Serial.print(DEVICE);
  Serial.print(' ');
  Serial.print(MODEL);
  Serial.print(' ');
  Serial.println(VERSION); 

  closeRS485();
}

void cmd_scan(int arg_cnt, char **args){

  if(arg_cnt<2) return;
  
  byte targetID = atoi(args[1]);
  if(targetID==get_id()){
    
    openRS485();    
      Serial.print("< ");
      Serial.print(targetID);      
      Serial.print(";");
      Serial.print(REGISTER_PIN_NUM);
      Serial.print(";");
      Serial.println(REGISTER_PIN_NUM+targetID);// checksum    
    closeRS485();
     
  }
}

void cmd_test(int arg_cnt, char **args){
  
  if(arg_cnt>1){
    if(get_id() != atoi(args[1])) return;
  }
  
  valveSlotType backupPrimaryValve = {primaryValve.box, primaryValve.slot};
  primaryValve.box=get_id();

  for(byte n=0; n<REGISTER_PIN_NUM; n++){    
    primaryValve.slot=(n+1);
    update_valves();
    delay(500);
  }
   primaryValve.box = backupPrimaryValve.box;
   primaryValve.slot = backupPrimaryValve.slot;
}

//--------------------------------------------
void RS485Poll(){

  if(!Serial.available()) return;
  
  int n = Serial.readBytesUntil(13,inputBuffer,INPUT_BUFFER_SIZE-2);  
  inputBuffer[n] = '\n';
  inputBuffer[n+1] = '\0';

  if(inputBuffer[n] != '\n' | inputBuffer[0] == '\n') return;
  
  byte i=0;
  if(inputBuffer[0]!='\n') while(i<INPUT_BUFFER_SIZE){
    if(inputBuffer[i]=='\0') break;
    //Serial.print(inputBuffer[i]);
    i++;       
  } 
  Serial.println(inputBuffer);     
  cmd_parse(inputBuffer);
  while(Serial.available()){
    Serial.read();
  }
}

void setup(){

  // first measure: disable shift register outputs
  pinMode(PIN_OE, OUTPUT);
  digitalWrite(PIN_OE, HIGH);

  cmdInit(RS485_BAUDRATE);
  cmdAdd("?", cmd_identify);
  cmdAdd("??", cmd_scan);
  cmdAdd("<", cmd_ignore);
  cmdAdd("valve", cmd_valve);
  cmdAdd("test", cmd_test);

  pinMode(RS485_EN, OUTPUT);
  digitalWrite(RS485_EN, LOW); // set RS485_module to receive mode  
 
  pinMode(PIN_DS, OUTPUT);
  pinMode(PIN_ST_CP, OUTPUT);
  pinMode(PIN_SH_CP, OUTPUT);
 
  update_valves();
  digitalWrite(PIN_OE, LOW); // finally enable shift register outputs

   cmd_identify(0,NULL);
  
}


void loop(){
  
  if(get_id()<16){  
    RS485Poll();
    update_valves();
  }else{
    cmd_test(0,NULL);
  }
}
