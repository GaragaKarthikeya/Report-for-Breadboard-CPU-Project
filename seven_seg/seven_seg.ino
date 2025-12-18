/* Two's complement bus -> 4x7-seg CC with leading minus.
   Same functionality; higher refresh to eliminate visible flicker.
   Timing: DIGIT_ON_US=500us, BLANK_US=10us, REFRESH_PASSES=40
*/

const int OE245_PIN = 10;  // 74LS245 OE̅ (LOW=read, HIGH=tri-state)

// Segments a..g mapped to D2..D8
const int SEG_A = 2, SEG_B = 3, SEG_C = 4, SEG_D = 5, SEG_E = 6, SEG_F = 7, SEG_G = 8;

// Digit selects (active-LOW common-cathode drivers)
const int DIGIT_ONES     = 11;
const int DIGIT_TENS     = 12;
const int DIGIT_HUNDREDS = 13;
const int DIGIT_SIGN     = 9;   // front-most minus-only digit

// ---- Multiplex timing (tuned for minimal flicker) ----
const uint16_t DIGIT_ON_US = 500;   // per-digit ON time (~0.5 ms)
const uint16_t BLANK_US    = 10;    // tiny dead-time to avoid ghosting
const uint8_t  REFRESH_PASSES = 40; // full 4-digit scans per bus read

// ---- Serial throttling ----
const unsigned long PRINT_PERIOD_MS = 100;
unsigned long lastPrintMs = 0;

// CC patterns: a,b,c,d,e,f,g (1=ON)
const uint8_t segmentPatterns[10][7] = {
  {1,1,1,1,1,1,0}, //0
  {0,1,1,0,0,0,0}, //1
  {1,1,0,1,1,0,1}, //2
  {1,1,1,1,0,0,1}, //3
  {0,1,1,0,0,1,1}, //4
  {1,0,1,1,0,1,1}, //5
  {1,0,1,1,1,1,1}, //6
  {1,1,1,0,0,0,0}, //7
  {1,1,1,1,1,1,1}, //8
  {1,1,1,1,0,1,1}  //9
};

inline void digitOn(int pin)  { digitalWrite(pin, LOW); }
inline void digitOff(int pin) { digitalWrite(pin, HIGH); }
inline void allDigitsOff()    { digitOff(DIGIT_ONES); digitOff(DIGIT_TENS); digitOff(DIGIT_HUNDREDS); digitOff(DIGIT_SIGN); }
inline void blankSegments()   { for (int s = SEG_A; s <= SEG_G; s++) digitalWrite(s, LOW); }

inline void driveNumberCC(int d) {
  if (d < 0 || d > 9) { blankSegments(); return; }
  for (int s = 0; s < 7; s++) digitalWrite(SEG_A + s, segmentPatterns[d][s] ? HIGH : LOW);
}

inline void driveMinusCC() { // only segment g
  for (int s = 0; s < 6; s++) digitalWrite(SEG_A + s, LOW);
  digitalWrite(SEG_G, HIGH);
}

void setup() {
  Serial.begin(115200);  // faster prints reduce blocking

  // Bus pins D2..D9 as INPUT (D2=MSB, D9=LSB)
  for (int p = 2; p <= 9; p++) pinMode(p, INPUT);

  // Segments start as inputs; digits as outputs (OFF)
  for (int s = SEG_A; s <= SEG_G; s++) pinMode(s, INPUT);
  pinMode(DIGIT_ONES, OUTPUT);
  pinMode(DIGIT_TENS, OUTPUT);
  pinMode(DIGIT_HUNDREDS, OUTPUT);
  pinMode(DIGIT_SIGN, OUTPUT);
  allDigitsOff();

  pinMode(OE245_PIN, OUTPUT);
  digitalWrite(OE245_PIN, HIGH); // tri-state 74LS245 (safe)
}

void loop() {
  // ---------- READ BUS ----------
  allDigitsOff();
  for (int s = SEG_A; s <= SEG_G; s++) pinMode(s, INPUT); // D2..D8 as inputs
  pinMode(DIGIT_SIGN, INPUT); // D9 as input (LSB) while reading

  digitalWrite(OE245_PIN, LOW);      // enable 74LS245
  delayMicroseconds(2);

  int byte_value = 0; // 0..255
  for (int p = 2; p <= 9; p++) {
    int bitVal = digitalRead(p) ? 1 : 0;
    int bitPos = 9 - p; // p=2->7 ... p=9->0
    byte_value |= (bitVal << bitPos);
  }

  digitalWrite(OE245_PIN, HIGH);     // tri-state 74LS245

  // ---------- SIGNED INTERPRETATION ----------
  int8_t signed_value = (int8_t)byte_value;          // −128..+127
  bool isNegative = (signed_value < 0);
  // abs for display; handle -128 explicitly
  int abs_value = isNegative ? (signed_value == -128 ? 128 : -signed_value) : signed_value;

  // zero-padded 3 digits from abs_value
  int hundreds = (abs_value / 100) % 10;
  int tens     = (abs_value / 10)  % 10;
  int ones     =  abs_value        % 10;

  // ---------- THROTTLED SERIAL PRINT ----------
  unsigned long now = millis();
  if (now - lastPrintMs >= PRINT_PERIOD_MS) {
    lastPrintMs = now;
    Serial.print("Raw (0–255): ");
    Serial.println(byte_value);
    Serial.print("Signed (two's complement): ");
    Serial.println(signed_value);
    Serial.print("Sign: ");
    Serial.println(isNegative ? "NEGATIVE" : "POSITIVE");
    Serial.print("Abs value: ");
    Serial.println(abs_value);
    Serial.print("Digits (H T O): ");
    Serial.print(hundreds); Serial.print(' ');
    Serial.print(tens);     Serial.print(' ');
    Serial.println(ones);
    Serial.println("--------------------------");
  }

  // ---------- DISPLAY MODE ----------
  for (int s = SEG_A; s <= SEG_G; s++) { pinMode(s, OUTPUT); digitalWrite(s, LOW); }
  pinMode(DIGIT_SIGN, OUTPUT);
  allDigitsOff();

  // Perform several fast refresh passes before next read
  for (uint8_t pass = 0; pass < REFRESH_PASSES; ++pass) {
    // Sign (only if negative)
    if (isNegative) {
      driveMinusCC();
      digitOn(DIGIT_SIGN);
      delayMicroseconds(DIGIT_ON_US);
      digitOff(DIGIT_SIGN);
      blankSegments();
      delayMicroseconds(BLANK_US);
    }

    // Hundreds
    driveNumberCC(hundreds);
    digitOn(DIGIT_HUNDREDS);
    delayMicroseconds(DIGIT_ON_US);
    digitOff(DIGIT_HUNDREDS);
    blankSegments();
    delayMicroseconds(BLANK_US);

    // Tens
    driveNumberCC(tens);
    digitOn(DIGIT_TENS);
    delayMicroseconds(DIGIT_ON_US);
    digitOff(DIGIT_TENS);
    blankSegments();
    delayMicroseconds(BLANK_US);

    // Ones
    driveNumberCC(ones);
    digitOn(DIGIT_ONES);
    delayMicroseconds(DIGIT_ON_US);
    digitOff(DIGIT_ONES);
    blankSegments();
    delayMicroseconds(BLANK_US);
  }

  // Restore pins for next read
  for (int s = SEG_A; s <= SEG_G; s++) pinMode(s, INPUT);
  pinMode(DIGIT_SIGN, INPUT);
}
