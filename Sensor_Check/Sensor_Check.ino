/*
  Sensor_Check.ino — SZ LABS Tiny OLED Keychain, Stage D verification sketch
  ------------------------------------------------------------------------
  Purpose : Prove the I2C bus and the LSM6DS3 sensor are wired correctly
            BEFORE the battery is connected.
  Method  : 1) Scan the I2C bus and list every device that answers.
            2) Read the LSM6DS3 WHO_AM_I register (0x0F) — must be 0x69.
            3) Stream live accelerometer X/Y/Z so a tilt is visible.
  Wiring  : Uses the XIAO ESP32-S3 default I2C pins — SDA = D4, SCL = D5.
            Expected devices: OLED at 0x3C, LSM6DS3 at 0x6A.
  Board   : Seeed XIAO ESP32S3 (Arduino IDE), USB-C, Serial Monitor 115200.
*/

#include <Wire.h>

#define LSM6DS3_ADDR   0x6A
#define REG_WHO_AM_I   0x0F
#define REG_CTRL1_XL   0x10
#define REG_OUTX_L_XL  0x28
#define WHO_AM_I_VALUE 0x69

void writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LSM6DS3_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(LSM6DS3_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(LSM6DS3_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

void scanBus() {
  Serial.println("Scanning I2C bus...");
  int found = 0;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  device found at 0x");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) Serial.println("  NO devices found — check SDA/SCL and power.");
  Serial.println("Expected: OLED 0x3C, LSM6DS3 0x6A.");
}

void setup() {
  Serial.begin(115200);
  delay(1500);  // allow the Serial Monitor to connect
  Wire.begin(); // default pins on XIAO ESP32-S3: SDA = D4, SCL = D5

  Serial.println();
  Serial.println("=== SZ LABS sensor check ===");
  scanBus();

  uint8_t who = readReg(REG_WHO_AM_I);
  Serial.print("LSM6DS3 WHO_AM_I = 0x");
  Serial.print(who, HEX);
  if (who == WHO_AM_I_VALUE) {
    Serial.println("  -> sensor OK");
    // Accelerometer on: 104 Hz, +/-2 g full scale (CTRL1_XL = 0x40)
    writeReg(REG_CTRL1_XL, 0x40);
    Serial.println("Streaming acceleration in g — tilt the board:");
  } else {
    Serial.println("  -> WRONG VALUE. Check the sensor wiring (VIN, GND, SDA, SCL).");
  }
}

void loop() {
  uint8_t buf[6];
  Wire.beginTransmission(LSM6DS3_ADDR);
  Wire.write(REG_OUTX_L_XL);
  Wire.endTransmission(false);
  Wire.requestFrom(LSM6DS3_ADDR, (uint8_t)6);
  for (int i = 0; i < 6 && Wire.available(); i++) buf[i] = Wire.read();

  int16_t ax = (int16_t)(buf[1] << 8 | buf[0]);
  int16_t ay = (int16_t)(buf[3] << 8 | buf[2]);
  int16_t az = (int16_t)(buf[5] << 8 | buf[4]);

  // +/-2 g range: 16384 LSB per g
  Serial.print("X: "); Serial.print(ax / 16384.0, 2);
  Serial.print(" g   Y: "); Serial.print(ay / 16384.0, 2);
  Serial.print(" g   Z: "); Serial.print(az / 16384.0, 2);
  Serial.println(" g");
  delay(250);
}
