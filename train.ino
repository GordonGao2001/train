// A short program that displays "Hello world!" on the top line of the lcd screen
// and "Hi back!" on the lower line
// Written by ?
// Adjusted by Geoffrey Frankhuizen,
// Uses code from Jeroen Tas to use alternative pins for I2C

// Wire connections:
//
// Default
// LCD connector to board connector
//
// Alternative
// D5 -> yellow (LCD connector)
// D6 -> green (LCD connector)
// 3.3v -> red (LCD connector)
// GND -> black (LCD connector)

// Uncomment to use alternative pins for the LCD

// LED init
#define ALTERNATIVE_PINS

#include "Adafruit_LiquidCrystal.h"

#ifdef ALTERNATIVE_PINS

#include "Arduino.h"
#include "wiring_private.h"

#define I2C_2_SDA_PIN 6
#define I2C_2_SCL_PIN 5

TwoWire secondWire(&sercom0, I2C_2_SDA_PIN, I2C_2_SCL_PIN);
Adafruit_LiquidCrystal lcd(0, &secondWire);

#else

Adafruit_LiquidCrystal lcd(0);

#endif

// HALL-EFFECT init
#include <MCP23017.h>


#define INT0  2
#define INT1  3


MCP23017 mcp = MCP23017(0x26);
MCP23017 mcp2 = MCP23017(0x27);

 // TRAIN init
#define DATAPIN 16
#define ENABLEPIN 17

const unsigned int DCCaddress_train1 = 0x01;   //dark red train
const unsigned int DCCaddress_train2 = 0x02;   //light red train
const unsigned int DCCaddress_train3 = 0x03;   //new train

const unsigned int DCCaddress_decoder = 0x06;
const unsigned int DCCswitchidle = 0b10000000;
const unsigned int DCCswitch1 = 0b10000001;
const unsigned int DCCswitch2 = 0b10000010;
const unsigned int DCCswitch3 = 0b10000100;
const unsigned int DCCswitch4 = 0b10001000;

const int switchwait = 1500;

// system variable
bool init_check = false;
bool init_station_fr = false;
bool init_station_neu = false;
bool train_dir_counter_clock=true;

struct _sens{
  bool int_rup0 ;
  int detector ;
};
//init train pos, 1 past d12 & next d13
_sens train1_past_pos = {false, 0b11101111};
_sens train1_next_pos = {false, 0b11011111};
//init train pos, 3 past d2 & next d21
_sens train3_past_pos = {true, 0b11111011};
_sens train3_next_pos = {false, 0b10111111};

bool switch_ready = false;//initial switch repostion

bool sw1_inward = false;
bool sw2_inward = false;
bool sw3_inward = false;
bool sw4_inward = false;


bool switch1_hot = false;
bool switch2_hot = false;
bool sw14_lock_true = false;
bool sw13_lock_true = false;
bool sw23_lock_true = false;
bool sw24_lock_true = false;

bool loop_occ = false;

unsigned long switch_delay_start = 0;
unsigned long switch_delay_end = 0;

void setup() {
  // LED setup
#ifdef ALTERNATIVE_PINS
  pinPeripheral(I2C_2_SDA_PIN, PIO_SERCOM_ALT);  //Assign SDA function
  pinPeripheral(I2C_2_SCL_PIN, PIO_SERCOM_ALT);  //Assign SCL function
#endif
  Serial.begin(115200);

  // HALL-EFFECT setup
 pinMode(INT0, INPUT_PULLUP);
 pinMode(INT1, INPUT_PULLUP);
 pinMode(LED_BUILTIN, OUTPUT);


 init_mcp();
 init_interrupts();

 // TRAIN setup
  pinMode(DATAPIN, OUTPUT);
  digitalWrite(DATAPIN, LOW);
  pinMode(ENABLEPIN, OUTPUT);
  digitalWrite(ENABLEPIN, HIGH);
  // Serial.begin(9600);

// switch setup 
  pinMode(DATAPIN, OUTPUT);
  digitalWrite(DATAPIN, LOW);
  pinMode(ENABLEPIN, OUTPUT);
  digitalWrite(ENABLEPIN, HIGH);


//sign signal setup
  Wire.begin();
  mcp2.init();
  mcp2.writeRegister(MCP23017Register::IODIR_A, (unsigned char )0x00); // set IO direction to output
  mcp2.writeRegister(MCP23017Register::IODIR_B, (unsigned char )0x00);
  mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
  mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);

}

void loop() {
  if(!init_check){
    Serial.println("init check not passed!");
    starting_check();
    Serial.print("\t neu status:");
    Serial.print(init_station_neu);
    Serial.print("fred status:");
    Serial.print(init_station_fr);
    Serial.println();
  }
  else{
    /*
    Add a manual switch here?
    */
    Serial.println("init check passed!");
    // if(!switch_ready){
    //   Switches_reset();
    //   switch_ready = true;
    // }
    Decision_table();
  }
}
void Switches_reset()
{
  delay(switchwait);
  changeSwitch1(false);
  delay(switchwait);
  changeSwitch2(false);
  delay(switchwait);
  changeSwitch3(false);
  delay(switchwait);
  changeSwitch4(false);
  switch_ready=true;
}
void starting_check()
{
    lcd.setBacklight(HIGH);
    DCC_send_command(DCCaddress_train1, trainInstruction_stop(true, true, 0), 100);
    delay(20);
    DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 0), 100);
    delay(20);
    
    if(!init_station_fr)
    {
      lcd.begin(16, 2);
      lcd.clear();
      //Serial.print("no train on Fred!\n");
      lcd.print("FRED: No\n");
    }else{
      lcd.begin(16, 2);
      lcd.clear();
      lcd.print("FRED: Ready\n");}
      //Serial.print("Fred Train checked!\n");
    if(!init_station_neu)
    {
      lcd.setCursor(0, 1);
      lcd.print("NEU: No\n");
      //Serial.print("No train on Neu!\n");
    }else{
      lcd.setCursor(0, 1);
      lcd.print("NEU: Ready\n");
      //Serial.print("Neu Train checked!\n");
      }
    init_check = (init_station_fr)&(init_station_neu);
    lcd.setBacklight(LOW);
}
void Decision_table()
{
  
  int train1_pos = where_is_train1(train1_past_pos, train1_next_pos);
  int train3_pos = where_is_train3(train3_past_pos, train3_next_pos);
  Serial.print("\n train1_pos:");
  Serial.print(train1_pos);
  Serial.print("\t train3_pos:");
  Serial.print(train3_pos);
  int table_case = train1_pos*100+train3_pos;
  // DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 6), 100);
  // delay(25);
  // DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
  // delay(25);
  switch(table_case)
  {
    // cases of impossible
    // system halt, restartable
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
    case 113:
    case 114:
    case 101:
    case 202:
    case 303:
    case 404:
    case 505:
    case 606:
    case 707:
    case 801:
    case 901:
    case 1001:
    case 1101:
    case 1201:
    case 808:
    case 809:
    case 810:
    case 811:
    case 812:
    case 813:
    case 814:
    case 908:
    case 909:
    case 910:
    case 911:
    case 912:
    case 913:
    case 914:
    case 1008:
    case 1009:
    case 1010:
    case 1011:
    case 1012:
    case 1013:
    case 1014:
    case 1108:
    case 1109:
    case 1110:
    case 1111:
    case 1112:
    case 1113:
    case 1114:
    case 1208:
    case 1209:
    case 1210:
    case 1211:
    case 1212:
    case 1213:
    case 1214:
      digitalWrite(ENABLEPIN,LOW);
      Serial.println("\t System Halted");
      delay(1000);
      // todo: implement restart
      break;
    //case of consecutive
      // train 1 at front
    case 201:
    case 302:
    case 403:
    case 504:
    case 605:
    case 807:
      Serial.println("\n case 201~807\t");
      DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 8), 100);
      delay(20);
      DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 0), 100);
      delay(20);
      break;
    case 706:
      Serial.println("\n case 706 \t");
      if(sw23_lock_true)
      {
        DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 8), 100);
        delay(20);
        DCC_send_command(DCCaddress_train3,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
      }else
      {
        changeSwitch2(false);
        changeSwitch3(false);
        sw23_lock_true = true;
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
        DCC_send_command(DCCaddress_train3,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
      }
      break;
    //train 3 at front
    case 102:
    case 203:
    case 304:
    case 405:
    case 506:
    case 708:
      Serial.println("\t case 102~203\t");
      DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
      delay(20);
      DCC_send_command(DCCaddress_train1, trainInstruction_stop(true, true, 0), 100);
      delay(20);
      break;
    case 607:
      Serial.println("\t case 607 \t");
      //todo same as 706
      if(sw24_lock_true)
      {
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
      }else
      {
        changeSwitch2(false);
        changeSwitch4(false);
        sw24_lock_true = true;
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
        DCC_send_command(DCCaddress_train3,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
      }
      break;
    //case of WFS: COL 7th
    case 701:
      Serial.println("\t case 701 \t");
      DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
      delay(20);
      DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
      delay(20);
      break;
    case 702:
      sw13_lock_true = false;
    case 703:
    case 704:
    case 705:
      Serial.println("\t case 702~705 \t");
      if(sw23_lock_true){
        DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 8), 100);
        delay(20);
        DCC_send_command(DCCaddress_train3,  trainInstruction(true, true, 0), 100);
        delay(20);
      }else{
        changeSwitch2(true);
        changeSwitch3(true);
        sw23_lock_true = true;
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1, trainInstruction_stop(true, true, 5), 100);
        delay(20);
      }
      break;
    case 709:
    case 710:
    case 711:
    case 712:
    case 713:
      Serial.println("\t case 708~713\t");
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
        break;
    case 714:
        Serial.println("\t case 714\t");
        sw13_lock_true = false;
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
        break;
    case 1202:
    case 1203:
    case 1204:
    case 1205:
    case 1206:
    Serial.println("\t case 1202~1206\t");
      if(sw14_lock_true)
      {
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 8), 100);
        delay(20);
      }else{
        changeSwitch1(false);
        changeSwitch4(false);
        sw14_lock_true = true;
        sw24_lock_true = true;
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
        delay(20);
      }
      break;
    case 107:
      Serial.println("\t case 107\t");
        DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction(true, true, 5), 100);
        delay(20);
      break;
    case 207:
    case 307:
    case 407:
    case 507:
        Serial.println("\t case 207~507\t");
        if(sw23_lock_true){
          DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
          delay(20);
          DCC_send_command(DCCaddress_train1,  trainInstruction(true, true, 5), 100);
          delay(20);
        }else{
          changeSwitch2(false);
          changeSwitch3(false);
          sw23_lock_true = true;
          DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 5), 100);
          delay(20);
          DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 0), 100);
          delay(20);
        }
        break;
    case 907:
    case 1007:
    case 1107:
      sw23_lock_true = false;
      Serial.println("\t case 807~1107 \t");
      DCC_send_command(DCCaddress_train1, trainInstruction_stop(true, true, 5), 100);
      delay(20);
      DCC_send_command(DCCaddress_train3,  trainInstruction_stop(true, true, 0), 100);
      delay(20);
      break;
    case 1207:
      Serial.println("\t case 1207 \t");
      if(sw14_lock_true){
        DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction(true, true, 5), 100);
        delay(20);
      }else{
        changeSwitch1(false);
        changeSwitch4(false);
        sw14_lock_true = true;
        DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction_stop(true, true, 5), 100);
        delay(20);
      }
      break;
    case 213:
    case 313:
    case 413:
    case 513:
    case 613:
      Serial.println("\t case 213~613\t");
      if(sw13_lock_true)
      {
        DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1, trainInstruction_stop(true, true, 5), 100);
        delay(20);
      }else{
        changeSwitch1(true);
        changeSwitch3(true);
        sw13_lock_true = true;
        sw14_lock_true = true;
        DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 5), 100);
        delay(20);
        DCC_send_command(DCCaddress_train1,  trainInstruction(true, true, 5), 100);
        delay(20);
      }
      break;
    //1 cases
    case 103:
    case 104:
    case 105:
    case 106:
    case 204:
    case 205:
    case 206:
    case 208:
    case 209:
    case 210:
    case 211:
    case 212:
    case 214:
    case 301:
    case 305:
    case 306:
    case 308:
    case 309:
    case 310:
    case 311:
    case 312:
    case 314:
    case 401:
    case 402:
    case 406:
    case 408:
    case 409:
    case 410:
    case 411:
    case 412:
    case 414:
    case 501:
    case 502:
    case 503:
    case 508:
    case 509:
    case 510:
    case 511:
    case 512:
    case 514:
    case 601:
    case 602:
    case 603:
    case 604:
    case 608:
    case 609:
    case 610:
    case 611:
    case 612:
    case 614:
    case 802:
    case 803:
    case 804:
    case 805:
    case 806:
    case 902:
    case 903:
    case 904:
    case 905:
    case 906:
    case 1102:
    case 1103:
    case 1104:
    case 1105:
    case 1106:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
    case 1006:
      sw13_lock_true = false;
      sw14_lock_true = false;
      sw23_lock_true = false;
      sw24_lock_true = false;
      Serial.print("\t case 1.\n");
      DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 8), 100);
      delay(20);
      DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
      delay(20);
      break;
    // Abnormal cases
    default:
      digitalWrite(ENABLEPIN,LOW);
      Serial.println("\t System Halted");
      break;
  }
}


//map sensdata to sensor location
int detector_mapper(_sens sens_data)
{
  switch(sens_data.detector)
  {
    case 0b11111110:
      return sens_data.int_rup0?0:8;
      break;
    case 0b11111101:
      return sens_data.int_rup0?1:9;
      break;
    case 0b11111011:
      return sens_data.int_rup0?2:10;
      break;
    case 0b11110111:
      return sens_data.int_rup0?3:11;
      break;
    case 0b11101111:
      return sens_data.int_rup0?4:12;
      break;
    case 0b11011111:
      return sens_data.int_rup0?5:13;
      break;
    case 0b10111111:
      return sens_data.int_rup0?6:21;
      break;
    case 0b1111111:
      return sens_data.int_rup0?7:22;
      break;
    default: return 101;
  }
}
void Trains_delay(int milisec)
{
  int repeats=milisec%40+1;
  while(repeats>0)
  {
    DCC_send_command(DCCaddress_train1, trainInstruction_stop(true, true, 0), 100);
    delay(20);
    DCC_send_command(DCCaddress_train3, trainInstruction_stop(true, true, 0), 100);
    delay(20);
    repeats = repeats-1;
  }
}
// e.g. triain is between 10 and 12, so it is on part 1
  // 10 is past, 12 is next
  // 10*100+12 = 1012
  // for revers maybe like case calculated with another formula
int where_is_train1(_sens past, _sens next)
{
  bool reverse = false;
  int my_case = reverse? 
    detector_mapper(past)+detector_mapper(next)*1000:detector_mapper(past)*100+detector_mapper(next);
  switch(my_case)
  {
    case 1012:
      return 1;
      break;
    case 1213:
      return 2;
      break;
    case 1300:
      return 3;
      break;
    case 1:
      return 4;
      break;
    case 122:
      return 5;
      break;
    case 2202:
      return 6;
      break;
    case 221:
      return 7;
      break;
    case 2104:
      return 8;
    case 406:
      return 9;
      break;
    case 607:
      return 10;
      break;
    case 708:
      return 11;
      break;
    case 810:
      return 12;
      break;
    default: 
      return 101;
      break;
  }
}

int where_is_train3(_sens past, _sens next)
{
  bool reverse = false;
  int my_case = reverse? 
    detector_mapper(past)+detector_mapper(next)*1000:detector_mapper(past)*100+detector_mapper(next);
  switch(my_case)
  {
    case 1112:
      return 1;
      break;
    case 1213:
      return 2;
      break;
    case 1300:
      return 3;
      break;
    case 1:
      return 4;
      break;
    case 122:
      return 5;
      break;
    case 2202:
      return 6;
      break;
    case 221:
      return 7;
      break;
    case 2103:
      return 8;
      break;
    case 309:
      return 9;
      break;
    case 908:
      return 10;
      break;
    case 807:
      return 11;
      break;
    case 706:
      return 12;
      break;
    case 605:
      return 13;
      break;
    case 511:
      return 14;
      break;
    default: 
      return 101;
      break;
  }
}
void signal_loop_exp()
{
  uint16_t idx;
  uint16_t signals;
  for (idx=0; idx<16; idx++) {
  switch(idx) {
  case 0:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111110);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 1:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111101);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 2:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111011);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 3:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11110111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 4:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11101111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 5:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11011111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 6:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b10111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 7:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b01111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 8:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111110);
  break;
  case 9:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111101);
  break;
  case 10:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111011);
  break;
  case 11:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11110111);
  break;
  case 12:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11101111);
  break;
  case 13:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11011111);
  break;
  case 14:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b10111111);
  break;
  case 15:
    mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b01111111);
  break;
  default:
  break;
  }
  delay(250);
  }
  mcp2.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
  mcp2.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
}

void LED_exp()
{
 // LED loop
  lcd.setBacklight(HIGH);
  delay(1000);
  lcd.setBacklight(LOW);
  delay(1000);
}

void hall_effect_loop_exp()
{
   // HALL-EFFECT loop
 // show the program is still running by blinking the onboard LED
 digitalWrite(LED_BUILTIN, LOW);
 delay(500);
 digitalWrite(LED_BUILTIN, HIGH);
 delay(500);
}

void train_loop_exp()
{
   // TRAIN loop
  DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 5), 100);
  delay(25);
  DCC_send_command(DCCaddress_train2, trainInstruction(true, true, 7), 100);
  delay(25);
  DCC_send_command(DCCaddress_train3, trainInstruction(true, true, 5), 100);
  delay(25);
}

void switch_loop_exp()
{
  delay(switchwait);
  changeSwitch1(true);
  delay(switchwait);
  changeSwitch2(true);
  delay(switchwait);
  changeSwitch3(true);
  delay(switchwait);
  changeSwitch4(true);
  delay(switchwait);
  changeSwitch1(false);
  delay(switchwait);
  changeSwitch2(false);
  delay(switchwait);
  changeSwitch3(false);
  delay(switchwait);
  changeSwitch4(false);
}

//HALL-EFFECT functions
void init_mcp() {
   Wire.begin();
   mcp.init();
   // Initialisation of MCP registers, documentation on registers is available at Niels/Abel/Robert/Natalia
   mcp.writeRegister(MCP23017Register::IODIR_A, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::IODIR_B, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::IPOL_A, (unsigned char )0x00);
   mcp.writeRegister(MCP23017Register::IPOL_B, (unsigned char )0x00);
   mcp.writeRegister(MCP23017Register::DEFVAL_A, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::DEFVAL_B, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::INTCON_A, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::INTCON_B, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::IOCON, (unsigned char )0x2);
   mcp.writeRegister(MCP23017Register::GPPU_A, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::GPPU_B, (unsigned char )0xff);
}

void init_interrupts() {
   // Clear current interrupts
   mcp.readRegister(MCP23017Register::GPIO_A);
   mcp.readRegister(MCP23017Register::GPIO_B);
   // Register callbacks
   attachInterrupt(digitalPinToInterrupt(INT0), on_int0_change, FALLING);
   attachInterrupt(digitalPinToInterrupt(INT1), on_int1_change, FALLING);
   // Enable interrupts on the MCP
   mcp.writeRegister(MCP23017Register::GPINTEN_A, (unsigned char )0xff);
   mcp.writeRegister(MCP23017Register::GPINTEN_B, (unsigned char )0xff);
   // Ready to go!
}

void on_int0_change() {
    // In your code, you might want to move this logic out of the interrupt routine
    // (for instance, by setting a flag and checking this flag in your main-loop)
    // This will prevent overhead.
    delayMicroseconds(2000);
    int  int0_sensor_data = mcp.readRegister(MCP23017Register::INTCAP_A);
    // The only thing we do with the interrupt signal is printing it
    //init station check
    if(int0_sensor_data == 0b11111011)
    {
      init_station_neu = true;
    }
    if(init_check)
    { 
    UpdatePos(int0_sensor_data,true);
    Serial.print("\t train1 next pos=\t");
    Serial.print(train1_next_pos.detector, BIN);
    Serial.println();
    }
    Serial.println("int0:");
    Serial.print(int0_sensor_data, BIN);
    Serial.println();
}

void on_int1_change() {
    // In your code, you might want to move this logic out of the interrupt routine
    // (for instance, by setting a flag and checking this flag in your main-loop)
    // This will prevent overhead.
    delayMicroseconds(2000);
    int int1_sensor_data = mcp.readRegister(MCP23017Register::INTCAP_B);
    //init staion check
    if(int1_sensor_data == 0b11101111)
    {
      init_station_fr = true;
    }
    if(init_check)
    {  UpdatePos(int1_sensor_data,false);
      Serial.print("\t train1 next pos=\t");
      Serial.print(train1_next_pos.detector, BIN);
      Serial.println();
    }
    // The only thing we do with the interrupt signal is printing it
    Serial.print("\n int1: \t");
    Serial.print(int1_sensor_data, BIN);
    Serial.println();
}
void UpdatePos(int sens_data,bool int_rup0)
{
  if( !(int_rup0 ^ train1_next_pos.int_rup0) && sens_data==train1_next_pos.detector)
  {
    Update_train1();
  }
  else if(!(int_rup0 ^ train3_next_pos.int_rup0)&&sens_data==train3_next_pos.detector)
  {Update_train3();}
  else{
    Serial.print("Undefined sensor input detected, quitting!\n");
    // TODO test this one
    // digitalWrite(ENABLEPIN,LOW);
    // Serial.println("\t System Halted");
  }
}

void Update_train1()
{
  train1_past_pos = train1_next_pos;
  _sens temp1;
  _sens temp2;
  switch(train1_past_pos.detector)
  {
    //d10
    case 0b11111011:
      if(!train1_past_pos.int_rup0)
      {  temp1 ={false, 0b11101111};
        temp2={false, 0b11111110};
        train1_next_pos = train_dir_counter_clock?temp1:temp2;

      }else
      {
        //d2
        temp1 = {false,0b10111111};
        temp2 = {false, 0b1111111};
        train1_next_pos = train_dir_counter_clock?temp1:temp2;
      }
    break;
    //d12
    case 0b11101111:
    if(!train1_past_pos.int_rup0)
      {temp1={false, 0b11011111};
      temp2={false, 0b11111011};
      train1_next_pos = train_dir_counter_clock?temp1:temp2;}
      else{
        temp1={true, 0b10111111};
        temp2={true, 0b11111011};
        train1_next_pos = train_dir_counter_clock?temp1:temp2;
      }
    break;
    //d13
    case 0b11011111:
      temp1={true, 0b11111110};
      temp2={false, 0b11101111};
      train1_next_pos = train_dir_counter_clock?temp1:temp2;
    break;
    //d0
    case 0b11111110:
    if(train1_past_pos.int_rup0){
      
      temp1={true, 0b11111101};
      temp2={false, 0b11011111};
      train1_next_pos = train_dir_counter_clock?temp1:temp2;
    }
    else{
      temp1={false, 0b11111011};
      temp2={true, 0b1111111};
      train1_next_pos = train_dir_counter_clock?temp1:temp2;
    }
    break;
    //d1
    case 0b11111101:
      temp1={false, 0b1111111};
      temp2={true, 0b11111110};
      train1_next_pos = train_dir_counter_clock?temp1:temp2;
    break;
    //d22
    case 0b1111111:
      if(!train1_past_pos.int_rup0)
      {
        temp1={true, 0b11111011};
        temp2={true, 0b11111101};
        train1_next_pos = train_dir_counter_clock?temp1:temp2;
      }
      else{
        temp1={false,0b11111110};
        temp2={true, 0b10111111};
        train1_next_pos = train_dir_counter_clock?temp1:temp2;
      }
    break;
    case 0b10111111:
    if(!train1_past_pos.int_rup0)
      {
      temp1={true, 0b11101111};
      temp2={true, 0b11111011};
      }else{
        temp1 ={true,0b1111111};
        temp2 = {true,0b11101111};
      }
      train1_next_pos = train_dir_counter_clock?temp1:temp2;
    break;
  }
}

void Update_train3()
{
  train3_past_pos = train3_next_pos;
  _sens temp;
  _sens temp1;
  switch(train3_past_pos.detector){
    case 0b11110111:
      //3
      if(train3_past_pos.int_rup0){
        temp = {false, 0b11111101};
        temp1 = {false, 0b10111111};
      }else{
        //d11
        temp = {false, 0b11101111};
        temp1 = {true, 0b11011111};
      }
      train3_next_pos = train_dir_counter_clock?temp:temp1;
      break;
  // d12
    case 0b11101111:
	      temp = {false, 0b11011111};
	      temp1 = {false, 0b11110111};
        train3_next_pos = train_dir_counter_clock?temp:temp1;
      break;
    case 0b11011111:
      if(train3_past_pos.int_rup0)
      //d5
      {
        temp = {false, 0b11110111};
        temp1 = {true, 0b10111111};
      }else{
        //d13
        temp = {true, 0b11111110};
        temp1 = {false, 0b11101111};
      }
      train3_next_pos = train_dir_counter_clock?temp:temp1;
    break;
    case 0b11111110:
    if(train3_past_pos.int_rup0){
      //d0
      temp = {true, 0b11111101};
      temp1 = {false, 0b11011111};
    }else{
      //d8
      temp = {true, 0b1111111};
      temp1 = {false, 0b11111101};
    }
    train3_next_pos = train_dir_counter_clock?temp:temp1;
    break;
	// d1
    case 0b11111101:
    if(train3_past_pos.int_rup0){
      temp = {false, 0b1111111};
      temp1 = {true, 0b11111110};
    }else{
      //d9
      temp = {false, 0b11111110};
      temp1 = {true, 0b11110111};
    }
    train3_next_pos = train_dir_counter_clock?temp:temp1;
    break;
	// d22
    case 0b1111111:
    if(!train3_past_pos.int_rup0){
      temp = {true, 0b11111011};
      temp1 = {true, 0b11111101};
    }else{
      //d7
      temp = {true, 0b10111111};
      temp1 = {false, 0b11111110};
    }
    train3_next_pos = train_dir_counter_clock?temp:temp1;
    break;
	// d2
    case 0b11111011:
      temp1 = {false, 0b1111111};
      temp = {false, 0b10111111};
      train3_next_pos = train_dir_counter_clock?temp:temp1;
    break;
  //d6
  case 0b10111111:
    if(train3_past_pos.int_rup0)
    {
      temp = {true, 0b11011111};
      temp1 = {true, 0b1111111};
    }else{
      //d21
      temp = {true, 0b11110111};
      temp1 = {true, 0b11111011};
    }
    train3_next_pos = train_dir_counter_clock?temp:temp1;
  break;
  default: train3_next_pos = {false,101};
  }
}
// TRAIN functions
unsigned int trainInstruction(bool direction, bool light, int speed) {
  unsigned int result = 0x40;  //0100 0000
  if (direction) {
    result += 0x20;  //0010 0000
  }
  if (light) {
    result += 0x10;  //0001 0000
  }
  if (speed > 0 && speed < 15) {
    result += speed + 1;
  }
  return result;
}

unsigned int trainInstruction_stop(bool estop, bool light, int speed) {
  unsigned int result = 0x40;  //0100 0000
  
  if (light) {
    result += 0x10;  //0001 0000
  }
  if(estop){
    result += 1;
  }
  return result;
}

void DCC_send_command(uint address, uint inst, uint repeat_count) {
  uint64_t command = 0x0000000000000000;
  uint64_t temp_command = 0x0000000000000000;
  uint64_t prefix = 0x3FFF;
  uint error = 0x00;
  error = address ^ inst;
  command = (prefix << 28) | (address << 19) | (inst << 10) | ((error) << 1) | 0x01;
  int i = 0;
  while (i < repeat_count) {
    temp_command = command;
    for (int j = 0; j < 64; j++) {
      if ((temp_command & 0x8000000000000000) == 0) { 
        digitalWrite(DATAPIN, LOW);
        delayMicroseconds(100);
        digitalWrite(DATAPIN, HIGH);
        delayMicroseconds(100);
      } else {
        digitalWrite(DATAPIN, LOW);
        delayMicroseconds(58);
        digitalWrite(DATAPIN, HIGH);
        delayMicroseconds(58);
      }
      temp_command = temp_command << 1;
    }
    i++;
  }
}

void changeSwitch1(bool inwards) {
  if (inwards) {
    DCC_send_command(DCCaddress_decoder, DCCswitch1, 1);
    DCC_send_command(DCCaddress_decoder, DCCswitchidle, 1);
    Serial.println("STATUS: Switch 1 changed inward");
  } else {
    DCC_send_command(DCCaddress_decoder, DCCswitch1, 1);
    Serial.println("STATUS: Switch 1 changed outward");
  }
}

void changeSwitch2(bool inwards) {
  if (inwards) {
    DCC_send_command(DCCaddress_decoder, DCCswitch2, 1);
    Serial.println("STATUS: Switch 2 changed inward");
  } else {
    DCC_send_command(DCCaddress_decoder, DCCswitch2, 1);
    DCC_send_command(DCCaddress_decoder, DCCswitchidle, 1);
    Serial.println("STATUS: Switch 2 changed outward");
  }
}

void changeSwitch3(bool inwards) {
  if (inwards) {
    DCC_send_command(DCCaddress_decoder, DCCswitch3, 1);
    DCC_send_command(DCCaddress_decoder, DCCswitchidle, 1);
    Serial.println("STATUS: Switch 3 changed inward");
  } else {
    DCC_send_command(DCCaddress_decoder, DCCswitch3, 1);
    Serial.println("STATUS: Switch 3 changed outward");
  }
}

void changeSwitch4(bool inwards) {
  if (inwards) {
    DCC_send_command(DCCaddress_decoder, DCCswitch4, 1);
    DCC_send_command(DCCaddress_decoder, DCCswitchidle, 1);
    Serial.println("STATUS: Switch 4 changed inward");
  } else {
    DCC_send_command(DCCaddress_decoder, DCCswitch4, 1);
    Serial.println("STATUS: Switch 4 changed outward");
  }
}
