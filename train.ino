#include <MCP23017.h>


#define INT0  2
#define INT1  3


MCP23017 mcp = MCP23017(0x26);


void setup() {
 pinMode(INT0, INPUT_PULLUP);
 pinMode(INT1, INPUT_PULLUP);
 pinMode(LED_BUILTIN, OUTPUT);


 init_mcp();
 init_interrupts();


}


void loop() {
 // show the program is still running by blinking the onboard LED
 digitalWrite(LED_BUILTIN, LOW);
 delay(500);
 digitalWrite(LED_BUILTIN, HIGH);
 delay(500);
}




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
