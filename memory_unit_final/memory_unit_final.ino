const uint8_t pins[8] = {2,3,4,5,6,7,8,9}; // 8-bit data bus (2 = LSB, 9 = MSB)

uint8_t inst_mem[256] = {
  0b00001010,
  0b00011100,
  0b00101111,
  0b00111101,
  0b01001111,
  0b01011001,
  0b01100000,
  0b01111000,
  0b10001111,
  0b10011111,
  0b10101111,
  0b10111011,
};  // 256-location instruction memory

uint8_t data_mem[16]  = {1,2,3,4,7,6,7,8,9,10,11,12,13,14,15,16};  // 16-location data memory (4-bit address)

int prevCLK = LOW;
uint8_t addr = 4;
uint8_t stage = 0;   // 0 = address phase, 1 = data phase

bool rising(int prev, int curr) {
  return (prev == LOW && curr == HIGH);
}

uint8_t readBus() {
  uint8_t v = 0;
  for (int i = 0; i < 8; i++)
    v |= (digitalRead(pins[i]) << i);
  return v;
}

void writeBus(uint8_t v) {
  for (int i = 0; i < 8; i++)
    digitalWrite(pins[i], ((v >> i) & 1) ? HIGH : LOW);
}

void setup() {
  pinMode(13, INPUT); // CLK
  pinMode(12, INPUT); // INST(0) / DATA(1)
  pinMode(11, INPUT); // IN(1) / OUT(0)
  pinMode(10, INPUT); // MEM_WORK
  Serial.begin(9600);
  for (int i = 0; i < 8; i++)
    pinMode(pins[i], INPUT_PULLUP);   // idle pulled up; change to INPUT if you want true high-Z
}

void loop() {
  int clk     = digitalRead(13);

  // <<< MINIMAL CHANGE HERE >>>
  // Treat MEM_WORK as active-low: external pulls LOW when transaction active
  bool memActive = (digitalRead(10) == LOW);
  // <<< end minimal change >>>

  int instSel = digitalRead(12);
  int dir     = digitalRead(11);

  if (rising(prevCLK, clk) && memActive) {

    // ---------- STAGE 0 (ADDRESS) ----------
    if (stage == 0) {
      for (int i = 0; i < 8; i++) pinMode(pins[i], INPUT); // tri-state while sampling
      addr = readBus();       // capture address
      Serial.print("Captured ADDR = ");
      Serial.println((int)addr);
      stage = 0;              // next rising clock = data phase
    }

    // ---------- STAGE 1 (DATA) ----------
    else {
      if (dir == HIGH) {
        // bus -> memory (external drives bus)
        for (int i = 0; i < 8; i++) pinMode(pins[i], INPUT);
        uint8_t data = readBus();
        Serial.print("Read DATA from bus DEC=");
        Serial.println((int)data);

        if (instSel == LOW) {
          inst_mem[addr] = data;
          Serial.print("Wrote to inst_mem[");
          Serial.print((int)addr);
          Serial.println("]");
        } else {
          data_mem[addr & 0x0F] = data;
          Serial.print("Wrote to data_mem[");
          Serial.print((int)(addr & 0x0F));
          Serial.println("]");
        }
      }
      else {
        // memory -> bus (Arduino drives bus)
        uint8_t data;
        if (instSel == LOW)
          data = inst_mem[addr];
        else
          data = data_mem[addr & 0x0F];
        Serial.println("Driving bus with:");
        Serial.println(data);
        for (int i = 0; i < 8; i++) pinMode(pins[i], OUTPUT);
        writeBus(data);
        // leave outputs briefly so external scope or device can sample
        delay(50);
        // tri-state afterwards to avoid holding the bus forever
        // for (int i = 0; i < 8; i++) pinMode(pins[i], INPUT);
      }

      stage = 1; // go back to address phase
    }

   
  }

  else if (!memActive) {
    // Ensure bus is tri-stated when no transaction is active
    for (int i = 0; i < 8; i++) {
      pinMode(pins[i], INPUT_PULLUP);
    }
  }

  prevCLK = clk;

  // FINAL DEBUG PRINT
Serial.print("ADDR = ");
  Serial.println(addr);
  Serial.println("Value to bus");
  Serial.println(data_mem[addr]);

  // Serial.println(inst_mem[addr]);
  

  // small loop pause (tune as required)
  delay(1000);
}
