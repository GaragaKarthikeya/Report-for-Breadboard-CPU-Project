// Robust slave: LSB-first, tolerant, tiny ISR, Enhanced Logging
#include <Wire.h>

#define SLAVE_ADDRESS 8   // change to 8 for first slave, 9 for second

const uint8_t outputPins[10] = {2,3,4,5,6,7,8,9,10,11};

/* MAPPING REFERENCE (Based on address):
   If Addr 8 (Slave 1): 
      pc_write, mar_read, mar_write, ir_read, ir_write,
      a_read, a_write, b_read, jcz, jsz
   
   If Addr 9 (Slave 2): 
      uj, mem_read_write, mem_work, out_enable, alu_op1,
      alu_op0, alu_write, cu_read_write, cu_work, inst_or_mem
*/

volatile uint16_t recvPacked = 0;
volatile bool newData = false;
volatile uint8_t lastLow = 0;
volatile uint8_t lastHigh = 0;

// Counter to track how many updates (cycles) we have received
unsigned long cycleCounter = 0;

void setup() {
  for (uint8_t i=0;i<10;i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(i2cReceive);
  Serial.begin(115200);
  
  Serial.println("--------------------------------------");
  Serial.print("Slave Initialized at Address: 0x");
  Serial.println(SLAVE_ADDRESS, HEX);
  Serial.println("Waiting for Control Signals...");
  Serial.println("--------------------------------------");
}

// ISR-like callback: VERY SMALL and deterministic
void i2cReceive(int howMany) {
  // Master sends LSB first: low byte then high byte
  uint8_t lowB = 0;
  uint8_t highB = 0;

  if (howMany >= 1 && Wire.available()) {
    lowB = Wire.read();
  }
  if (howMany >= 2 && Wire.available()) {
    highB = Wire.read();
  }

  // store last bytes for debug (volatile)
  lastLow = lowB;
  lastHigh = highB;

  uint16_t packed = ((uint16_t)highB << 8) | lowB;
  packed &= 0x03FF;       // mask to 10 bits
  recvPacked = packed;
  newData = true;
}

void applyPacked(uint16_t p) {
  for (uint8_t i=0;i<10;i++){
    digitalWrite(outputPins[i], ((p >> i) & 1) ? HIGH : LOW);
  }
}

void printBinary10(uint16_t v) {
  for (int i = 9; i >= 0; --i) {
    Serial.print( (v >> i) & 1 );
    // Optional: Add a separator for readability (e.g., 5 bits space 5 bits)
    // if (i == 5) Serial.print(" "); 
  }
}

void loop() {
  if (newData) {
    // 1. Copy data atomically (disable interrupts briefly)
    noInterrupts();
    uint16_t local = recvPacked;
    uint8_t low = lastLow;
    uint8_t high = lastHigh;
    newData = false;
    interrupts();

    // 2. Apply outputs to physical pins
    applyPacked(local);
    
    // 3. Increment Cycle Counter
    cycleCounter++;

    // 4. Enhanced Logging
    Serial.print("[Update #");
    Serial.print(cycleCounter);
    Serial.print("] ");
    
    Serial.print("Hex: 0x");
    if (local < 0x10) Serial.print("00"); // Padding for readability
    else if (local < 0x100) Serial.print("0");
    Serial.print(local, HEX);
    
    Serial.print(" | Bin: ");
    printBinary10(local);
    
    Serial.print(" | (Raw L:0x");
    Serial.print(low, HEX);
    Serial.print(" H:0x");
    Serial.print(high, HEX);
    Serial.println(")");
  }

  delay(1);
}