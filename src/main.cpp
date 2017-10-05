
#include <Arduino.h>
#include <OneWire.h>

// DS1821 commands
#define READ_CFG 0xAC   // read configuration register
#define WRITE_CFG 0x0C  // write configuration register
#define READ_TEMP 0xAA  // read temperature register
#define START_CNV 0xEE  // start temperature conversion
#define STOP_CNV 0x22   // stop temperature conversion
#define READ_TH 0xA1    // read high temperature register
#define READ_TL 0xA2    // read low temperature register
#define WRITE_TH 0x01   // write high temperature register
#define WRITE_TL 0x02   // write low temperature register
#define READ_CNTR 0xA0  // read counter register
#define READ_SLOPE 0xA9 // read slope register

// CFG Configuration register anatomy
#define DONE B10000000   // temperature conversion is done
#define NVB B00100000    // non-volatile memory is busy
#define THF B00010000    // high temp flag
#define TLF B00001000    // low temp flag
#define TRMODE B00000100 // Power-up operating mode; 0 - OneWire; 1 - thermostat
#define POL B00000010    // DQ Polarity. Active high when POL = 1
#define ONESHOT B00000001 // 1 = one conversion; 0 = continuous conversion

#define pDQ 4
#define pVDD A5
OneWire ds(pDQ);

boolean chipReady(boolean print = true) {
  // send the reset pulse command and fail fast if no sensor
  int resp = ds.reset();
  if (resp == 0) {
    if (print) Serial.println("Can't perform reset cycle. No answer from chip. there is no device or the bus is shorted or otherwise held low for more than 250uS");
    return false;
  }
  return true;
}

uint8_t readCFG() {
  if (!chipReady())
    return 0;
  ds.write(READ_CFG);
  uint8_t cfg = ds.read();
  Serial.println("DS1821 CFG Map: ");
  Serial.println("D      1");
  Serial.println("O NTT PS");
  Serial.println("N VHLTOH");
  Serial.println("E BFFRLT");
  for (int i = 0; i < 8; i++)
    if (cfg < pow(2, i))
      Serial.print(B0);
  Serial.println(cfg, BIN);
  return cfg;
}

int8_t readTHTL(uint8_t cmd) {
  if (!chipReady())
    return 0;
  ds.write(cmd);
  uint8_t v = ds.read();
  if (cmd == READ_TH) Serial.print("TH: ");
  else Serial.print("TL: ");
  Serial.println((int8_t)v);
  return (int8_t)v;
}

uint8_t readTEMP() {
  if (!chipReady())
    return 0;
  ds.write(START_CNV);
  delay(1000);
  if (!chipReady())
    return 0;
  ds.write(READ_TEMP);
  uint8_t v = ds.read();
  Serial.print("TEMP: ");
  Serial.print(v, BIN);
  Serial.print(" ");
  Serial.println(v);
  return v;
}

void writeGisteresis(uint8_t reg) {
  Serial.println("Enter temperature and press enter");
  String inputText = "";
  char readedValue = 0;
  while(readedValue != '\n'){
    if (Serial.available()){
      readedValue = (char)Serial.read();
      Serial.print(readedValue);
      inputText += readedValue;
    }
  }
  int celsius = inputText.toInt();
  if (!chipReady())
    return;
  // make sure the alarm temperature is within the device's range
  uint8_t t = (uint8_t)constrain(celsius, -55, 125);
  ds.write(reg);
  ds.write(t);
  if (reg == WRITE_TH) Serial.print("TH");
  else Serial.print("TL");
  Serial.print(" Temperature writed: ");
  Serial.println(celsius);
}
void writeCFG(uint8_t data) {
  if (!chipReady())
    return;
  ds.write(WRITE_CFG);
  ds.write(data);
  Serial.println("CFG writed. Now you can detach DS1821 from pins and use it");
}

void exitFromThermostatMode() {
  pinMode(pDQ, OUTPUT);
  digitalWrite(pVDD, LOW);
  delayMicroseconds(1);
  for (uint8_t i = 0; i < 32; i++) {
    if (i % 2 == 0) {
      digitalWrite(pDQ, LOW);
    } else {
      digitalWrite(pDQ, HIGH);
    }
    delayMicroseconds(1);
  }
  digitalWrite(pVDD, HIGH);
  delay(1);
}

void onewireStart() {
  String startInstructions = "\nChip in OneWire mode and ready to program\n\nType one of the following commands to continue:\n'c' to show chip config and TH/TL registers\n't' to show temperarure\n'h' to enter TH register\n'l' to enter TL register\n'0' to switch chip to thermostat mode with pol=0\n'1' to switch chip to thermostat mode with pol=1\n'o' to switch chip to OneWire mode";
  Serial.print("Trying communicate in OneWire mode...");
  if (chipReady(false)) {
    Serial.println(startInstructions);
    return;
  }
  Serial.print("Chip in thermostat mode\nTrying to switch in OneWire mode...");
  exitFromThermostatMode();
  if (chipReady(false)) {
    Serial.print("Switched to OneWire mode.");
    Serial.println(startInstructions);
    return;
  }
  Serial.println("Error ocurred. Exit from thermostat Mode failed. OneWire mode not available. Check chip and connections.");
}

void setup() {
  Serial.begin(9600);
  Serial.print("DS1821 Programmator started\nConnect DS1821 GND DQ VDD pins to programmator GND ");
  Serial.print(pDQ);
  Serial.print(" ");
  Serial.print(pVDD);
  Serial.println(" pins and\ntype 's' to start");
  pinMode(pVDD, OUTPUT);
  digitalWrite(pVDD, HIGH);
}

void loop() {
  if (Serial.available()) {
    char v = (char)Serial.read();
    switch (v) {
    case 115: // s
      onewireStart();
      break;
    case 99: // c - read config info
      readCFG();
      readTHTL(READ_TH);
      readTHTL(READ_TL);
      break;
    case 116: //t read temperature
      readTEMP();
      break;
    case 104: // h
      writeGisteresis(WRITE_TH);
      break;
    case 108: // l
      writeGisteresis(WRITE_TL);
      break;
    case 48: // 0 thermostat mode POL = 0
      writeCFG(B00000101);
      break;
    case 49: // 1 thermostat mode POL = 1
      writeCFG(B00000111);
      break;
    case 111: // o OneWire mode
      writeCFG(B00000011);
      break;
    }
  }
}
