//This is the code for the master control unit which sets the control signals based on the instruction and the stage of the instruction ,this has 8 pins which it takes in to decode the instruction received by the cpu.
//The data memory address is 4 bits wide(16 available locations) which will be set onto the bus by the control unit when the address should be received by the memory.
#include <Arduino.h>
#include <Wire.h>

#define SR_CLK 11
#define PC_CLK_PIN 12
#define CLK_PIN 13
#define SERIAL 10


uint8_t pc_write = 1, mar_read = 1, mar_write = 1, ir_read = 1, ir_write = 1,
        a_read = 1, a_write = 1, b_read = 1, jcz = 1, jsz = 1, uj = 1,
        mem_read_write = 1, mem_work = 0, out_enable = 1, alu_op1 = 0,
        alu_op0 = 0, alu_write = 1, cu_read_write = 1, cu_work = 1, inst_or_mem = 0;  //if 0 inst ,if 1 mem
void clear_cont() {
  pc_write = 1;
  mar_read = 1;
  mar_write = 1;
  ir_read = 1;
  ir_write = 1;
  a_read = 1;
  a_write = 1;
  b_read = 1;
  jcz = 1;
  jsz = 1;
  uj = 1;
  mem_read_write = 1;
  mem_work = 0;
  out_enable = 1;
  alu_op1 = 0;
  alu_op0 = 0;
  alu_write = 1;
  cu_read_write = 1;
  cu_work = 1;
  inst_or_mem = 0;
}

const uint8_t pins[] = { 2, 3, 4, 5, 6, 7, 8, 9 };  // 8 pins for cu

uint8_t *controls[] = {
  &pc_write, &mar_read, &mar_write, &ir_read, &ir_write,
  &a_read, &a_write, &b_read, &jcz, &jsz, &uj,
  &mem_read_write, &mem_work, &out_enable, &alu_op1,
  &alu_op0, &alu_write, &cu_read_write, &cu_work, &inst_or_mem
};


// -------------------- setup / loop --------------------
void setup() {
  Wire.begin();
 
  Serial.begin(9600);
  pinMode(PC_CLK_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  digitalWrite(PC_CLK_PIN, LOW);
  digitalWrite(CLK_PIN, LOW);

  for (int i = 0; i < 8; i++) {
    pinMode(pins[i], INPUT);
  }
  pinMode(SR_CLK,OUTPUT);
  digitalWrite(SR_CLK,LOW);
  pinMode(SERIAL,OUTPUT);
}

void debug( int cycle) {
  digitalWrite(CLK_PIN, HIGH);
  delay(1000);
  digitalWrite(CLK_PIN, LOW);
   delay(1000);
  clear_cont();
 

  // for (uint8_t i = 0; i < 10; ++i) Serial.print(bits[i]);
  // Serial.print("   S2 bits: ");
  // for (uint8_t i = 0; i < 10; ++i) Serial.print(bits2[i]);
  // Serial.println();
}

// Helper: Write address to CU pins
void writeCUAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  cu_work = 0;
  cu_read_write = 0;
  for (int i = 0; i < 8; i++) pinMode(pins[i], OUTPUT);
  digitalWrite(pins[0], a);
  digitalWrite(pins[1], b);
  digitalWrite(pins[2], c);
  digitalWrite(pins[3], d);
  for (int i = 4; i < 8; i++) digitalWrite(pins[i], HIGH);
}
void load_controls(uint8_t *controls[]) {

  uint16_t slave1 = 0;
  uint16_t slave2 = 0;

  // pack first 10 control bits for slave 1
  for (int i = 0; i < 10; i++) {
    slave1 |= ((*controls[i] & 1) << i);
  }

  // pack next 10 control bits for slave 2
  for (int i = 10; i < 20; i++) {
    slave2 |= ((*controls[i] & 1) << (i - 10));
  }

  // send to slave 1 (address 8)
  Wire.beginTransmission(8);
  Wire.write((uint8_t)(slave1 & 0xFF));      // low byte
  Wire.write((uint8_t)((slave1 >> 8) & 0xFF)); // high byte
  Wire.endTransmission();

  // send to slave 2 (address 9)
  Wire.beginTransmission(9);
  Wire.write((uint8_t)(slave2 & 0xFF));
  Wire.write((uint8_t)((slave2 >> 8) & 0xFF));
  Wire.endTransmission();
}
// Helper: Execute ALU instruction (ADD, SUB, AND, OR)
void executeALU(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t op1, uint8_t op0) {
  // 5th cycle
  writeCUAddress(a, b, c, d);
  mar_read = 0;
   load_controls(controls);
  debug( 5);

  // 6th cycle
  mar_write = 0;
  mem_read_write = 0;
  mem_work = 1;
  inst_or_mem = 1;
   load_controls(controls);
  debug( 6);

  // 7th cycle
  mem_read_write = 1;
  inst_or_mem = 1;
  mem_work = 1;
  b_read = 0;
  alu_op1 = op1;
  alu_op0 = op0;
   load_controls(controls);
  debug( 7);

  // 8th cycle
  alu_write = 0;
  a_read = 0;
   load_controls(controls);
  debug( 8);
}

void loop() {



  // Common 4 cycles for all instructions
  // 1st cycle
  pc_write = 0;
  mar_read = 0;
  load_controls(controls);
  debug( 1);

  // 2nd cycle
  mar_write = 0;
  mem_work = 1;
  mem_read_write = 0;
  inst_or_mem = 0;
  load_controls(controls);
  debug( 2);

  // 3rd cycle
  mem_read_write = 1;
  inst_or_mem = 0;
  ir_read = 0;
   load_controls(controls);
  debug( 3);

  // 4th cycle - Read opcode from IR
  ir_write = 0;
  cu_work = 0;
  cu_read_write = 1;
  load_controls(controls);

  for (int i = 0; i < 8; i++)
    pinMode(pins[i], INPUT);

  delay(1000);
  digitalWrite(CLK_PIN,HIGH);
  
  uint8_t x = digitalRead(pins[0]);
  uint8_t y = digitalRead(pins[1]);
  uint8_t z = digitalRead(pins[2]);
  uint8_t r = digitalRead(pins[3]);

  uint8_t opcode = (x << 3) | (y << 2) | (z << 1) | r;
  uint8_t a = digitalRead(pins[4]);
  uint8_t b = digitalRead(pins[5]);
  uint8_t c = digitalRead(pins[6]);
  uint8_t d = digitalRead(pins[7]);

  delay(1000);
  digitalWrite(CLK_PIN,LOW);
  delay(1);//the clock styling only for this is different cycle 4

  // Execute instruction based on opcode
  if (opcode == 0b0000) {  // LDA
    // 5th cycle
    writeCUAddress(a, b, c, d);
    mar_read = 0;
    load_controls(controls);
    debug( 5);
    // 6th cycle
    mar_write = 0;
    mem_read_write = 0;
    mem_work = 1;
    inst_or_mem = 1;
    load_controls(controls);
    debug( 6);
    // 7th cycle
    mem_read_write = 1;
    inst_or_mem = 1;
    a_read = 0;
     load_controls(controls);
    debug( 7);
    delay(2000);
  }

  else if (opcode == 0b0100) {  // LDI
    // 5th cycle
    writeCUAddress(a, b, c, d);
    a_read = 0;
     load_controls(controls);
    debug( 5);
    delay(6000);
  }

  else if (opcode == 0b0011) {  // STA
    // 5th cycle
    writeCUAddress(a, b, c, d);
    mar_read = 0;
     load_controls(controls);
    debug( 5);
    // 6th cycle
    mar_write = 0;
    mem_read_write = 0;
    mem_work = 1;
    inst_or_mem = 1;
     load_controls(controls);
    debug( 6);
    // 7th cycle
    a_write = 0;
    mem_read_write = 0;
    mem_work = 1;
    inst_or_mem = 1;
     load_controls(controls);
    debug( 7);
    delay(2000);
  }

  else if (opcode == 0b0001) {  // ADD
    executeALU(a, b, c, d, 1, 0);
  }

  else if (opcode == 0b0010) {  // SUB
    executeALU(a, b, c, d, 1, 1);
  }

  else if (opcode == 0b1010) {  // AND
    executeALU(a, b, c, d, 0, 0);
  }

  else if (opcode == 0b1011) {  // OR
    executeALU(a, b, c, d, 0, 1);
  }

  else if (opcode == 0b0101) {  // JMP
    // 5th cycle
    writeCUAddress(a, b, c, d);
    mar_read = 0;
     load_controls(controls);
    debug( 5);
    // 6th cycle
    uj = 0;
     load_controls(controls);
    debug( 6);
    delay(4000);
  }

  else if (opcode == 0b0110) {  // JC
    // 5th cycle
    writeCUAddress(a, b, c, d);
    mar_read = 0;
     load_controls(controls);
    debug( 5);
    // 6th cycle
    jcz = 0;
     load_controls(controls);
    debug( 6);
    delay(4000);
  }

  else if (opcode == 0b0111) {  // JZ
    // 5th cycle
    writeCUAddress(a, b, c, d);
    mar_read = 0;
     load_controls(controls);
    debug( 5);
    // 6th cycle
    jsz = 0;
     load_controls(controls);
    debug( 6);
    delay(4000);
  }

  else if (opcode == 0b1000) {  // OUT
    // 5th cycle
    out_enable = 0;
    a_write = 0;
     load_controls(controls);
    debug( 5);
    delay(6000);
  }

  else if (opcode == 0b1001) {  // HLT
    while(1);//runs infintely
  }

  else {
    Serial.print("Unknown opcode: 0b");
    Serial.println(opcode, BIN);
    delay(14000);
  }

  digitalWrite(PC_CLK_PIN, LOW);
  digitalWrite(CLK_PIN,LOW);
  delay(1);
}

// master
