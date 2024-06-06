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
MCP23017 mcp = MCP23017(0x27);

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


void setup() {
  // LED setup
#ifdef ALTERNATIVE_PINS
  pinPeripheral(I2C_2_SDA_PIN, PIO_SERCOM_ALT);  //Assign SDA function
  pinPeripheral(I2C_2_SCL_PIN, PIO_SERCOM_ALT);  //Assign SCL function
#endif
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Hello world!");
  lcd.setCursor(0, 1);
  lcd.print("Hi back!");

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
  mcp.init();
  mcp.writeRegister(MCP23017Register::IODIR_A, (unsigned char )0x00); // set IO direction to output
  mcp.writeRegister(MCP23017Register::IODIR_B, (unsigned char )0x00);
  mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
  mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
}

void loop() {

  LED_exp();

  switch_loop_exp();
  // signal_loop_exp();
}

void signal_loop_exp()
{
  uint16_t idx;
  uint16_t signals;
  for (idx=0; idx<16; idx++) {
  switch(idx) {
  case 0:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111110);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 1:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111101);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 2:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111011);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 3:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11110111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 4:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11101111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 5:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11011111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 6:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b10111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 7:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b01111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
  break;
  case 8:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111110);
  break;
  case 9:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111101);
  break;
  case 10:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111011);
  break;
  case 11:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11110111);
  break;
  case 12:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11101111);
  break;
  case 13:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11011111);
  break;
  case 14:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b10111111);
  break;
  case 15:
    mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
    mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b01111111);
  break;
  default:
  break;
  }
  delay(250);
  }
  mcp.writeRegister(MCP23017Register::OLAT_A, (unsigned char )0b11111111);
  mcp.writeRegister(MCP23017Register::OLAT_B, (unsigned char )0b11111111);
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
  DCC_send_command(DCCaddress_train1, trainInstruction(true, true, 9), 100);
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
   int sensor_data = mcp.readRegister(MCP23017Register::INTCAP_A);
   // The only thing we do with the interrupt signal is printing it
   Serial.println("int0:");
   Serial.print(sensor_data, BIN);
   Serial.println();
}

void on_int1_change() {
   // In your code, you might want to move this logic out of the interrupt routine
   // (for instance, by setting a flag and checking this flag in your main-loop)
   // This will prevent overhead.
   delayMicroseconds(2000);
   int sensor_data = mcp.readRegister(MCP23017Register::INTCAP_B);
   // The only thing we do with the interrupt signal is printing it
   Serial.println("int1:");
   Serial.print(sensor_data, BIN);
   Serial.println();
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