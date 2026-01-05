#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AW9523.h>
#include <lvgl.h>
#include "ili9488_driver.hpp"
#include "display_thread.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Main setup: start display task");

  // Display-task starten op core 1 (kan ook 0 zijn)
  xTaskCreatePinnedToCore(
    display_task,          // task-functie
    "DisplayTask",         // naam
    8192,                  // stack depth (words, 8192 is ruim voor LVGL)
    nullptr,               // geen parameter
    1,                     // prioriteit (iets boven idle, lager dan zware taken)
    nullptr,               // geen task-handle nodig
    1                      // core-ID (0 of 1, S3 is dual-core)
  );
}

void loop() {
  // Hoofd-loop doet hier niks voor de display.
  // Later kun je hier andere dingen doen, of hem gewoon laten idlen.
  delay(1000);
}

// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_MCP23X17.h>

// // I2C pins ESP32-S3
// static constexpr int I2C_SDA = 21;
// static constexpr int I2C_SCL = 19;

// // MCP23017 address
// static constexpr uint8_t MCP_ADDR = 0x20;

// Adafruit_MCP23X17 mcp;

// // GPB0..3 = 8..11
// static constexpr uint8_t BTN_PINS[4] = { 8, 9, 10, 11 };
// // GPA1..4 = 1..4
// static constexpr uint8_t LED_PINS[4] = { 1, 2, 3, 4 };

// // LED toggle states
// bool ledState[4] = { false, false, false, false };

// // Debounce
// bool lastStableBtn[4] = { HIGH, HIGH, HIGH, HIGH };
// bool lastReading[4]   = { HIGH, HIGH, HIGH, HIGH };
// unsigned long lastChange[4] = { 0, 0, 0, 0 };
// static constexpr unsigned long DEBOUNCE_MS = 40;

// void setup()
// {
//   Serial.begin(115200);
//   delay(200);

//   Wire.begin(I2C_SDA, I2C_SCL);

//   if (!mcp.begin_I2C(MCP_ADDR, &Wire)) {
//     Serial.println("FOUT: MCP23017 niet gevonden");
//     while (true) { delay(1000); }
//   }

//   // Zet ALLES eerst veilig als input
//   for (int pin = 0; pin < 16; pin++) {
//     mcp.pinMode(pin, INPUT);
//   }

//   // LED outputs
//   for (int i = 0; i < 4; i++) {
//     mcp.pinMode(LED_PINS[i], OUTPUT);
//     mcp.digitalWrite(LED_PINS[i], LOW);
//   }

//   // Button inputs met pull-up
//   for (int i = 0; i < 4; i++) {
//     mcp.pinMode(BTN_PINS[i], INPUT_PULLUP);
//   }

//   Serial.println("GPB0-3 toggelen GPA1-4. Overige pinnen = input.");
// }

// void loop()
// {
//   const unsigned long now = millis();

//   for (int i = 0; i < 4; i++) {
//     bool reading = mcp.digitalRead(BTN_PINS[i]);

//     // Detecteer verandering
//     if (reading != lastReading[i]) {
//       lastChange[i] = now;
//       lastReading[i] = reading;
//     }

//     // Na debounce-tijd → stabiele status
//     if ((now - lastChange[i]) > DEBOUNCE_MS) {
//       if (reading != lastStableBtn[i]) {
//         lastStableBtn[i] = reading;

//         // Alleen bij indrukken (HIGH → LOW)
//         if (reading == LOW) {
//           ledState[i] = !ledState[i];
//           mcp.digitalWrite(LED_PINS[i], ledState[i] ? HIGH : LOW);
//         }
//       }
//     }
//   }
// }


// #include <Arduino.h>
// #include <Wire.h>

// // ================== I2C PINS ==================
// static constexpr int PIN_I2C_SDA = 21;   // pas aan
// static constexpr int PIN_I2C_SCL = 19;   // pas aan

// // ================== AD5274 SETTINGS ==================
// static constexpr float RPOT_MAX_OHM = 100000.0f;  // 100k variant

// // AD5274 mogelijke adressen o.b.v. ADDR-pin toestand (datasheet: VDD/NC/GND)
// static constexpr uint8_t AD5274_ADDR_CANDIDATES[] = { 0x2C, 0x2E, 0x2F };

// // Command nibble (C3..C0) in 16-bit frame (2 dummy bits + 4 cmd + 10 data)
// static constexpr uint16_t CMD_WRITE_RDAC = 0x1;
// static constexpr uint16_t CMD_READ_RDAC  = 0x2;
// static constexpr uint16_t CMD_WRITE_CTRL = 0x7;

// // Control register: C1=1 => RDAC updates via interface toegestaan
// static constexpr uint16_t CTRL_C1_ALLOW_RDAC_UPDATE = 0x0002;

// // ================== HELPERS ==================
// static inline uint16_t makeFrame(uint16_t cmd, uint16_t data10)
// {
//   return (uint16_t)(((cmd & 0x0F) << 10) | (data10 & 0x03FF));
// }

// static bool i2cPing(uint8_t addr)
// {
//   Wire.beginTransmission(addr);
//   return (Wire.endTransmission() == 0);
// }

// static bool ad5274Write16(uint8_t addr, uint16_t word)
// {
//   Wire.beginTransmission(addr);
//   Wire.write((uint8_t)((word >> 8) & 0xFF)); // MSB
//   Wire.write((uint8_t)(word & 0xFF));        // LSB
//   return (Wire.endTransmission() == 0);
// }

// // Veel ADI digipots: read = stuur "read command frame" en daarna 2 bytes ophalen.
// // Dit werkt op de AD5274 voor RDAC readback.
// static bool ad5274Read16(uint8_t addr, uint16_t commandFrame, uint16_t &outWord)
// {
//   // command sturen
//   if (!ad5274Write16(addr, commandFrame)) {
//     return false;
//   }

//   // 2 bytes teruglezen
//   const int n = Wire.requestFrom((int)addr, 2);
//   if (n != 2) {
//     return false;
//   }

//   const uint8_t msb = Wire.read();
//   const uint8_t lsb = Wire.read();
//   outWord = ((uint16_t)msb << 8) | lsb;
//   return true;
// }

// static uint8_t codeFromRohm(float r_ohm)
// {
//   if (r_ohm < 0.0f) r_ohm = 0.0f;
//   if (r_ohm > RPOT_MAX_OHM) r_ohm = RPOT_MAX_OHM;

//   int code = (int)lroundf((r_ohm / RPOT_MAX_OHM) * 255.0f);
//   if (code < 0) code = 0;
//   if (code > 255) code = 255;
//   return (uint8_t)code;
// }

// static float rohmsFromCode(uint8_t code)
// {
//   return (code / 255.0f) * RPOT_MAX_OHM;
// }

// static bool ad5274EnableRdacUpdate(uint8_t addr)
// {
//   const uint16_t frame = makeFrame(CMD_WRITE_CTRL, CTRL_C1_ALLOW_RDAC_UPDATE);
//   return ad5274Write16(addr, frame);
// }

// static bool ad5274WriteCode(uint8_t addr, uint8_t code8)
// {
//   // AD5274: effectief 8-bit; in 10-bit veld gaat code in D9..D2, D1..D0 don't care
//   const uint16_t data10 = ((uint16_t)code8) << 2;
//   const uint16_t frame  = makeFrame(CMD_WRITE_RDAC, data10);
//   return ad5274Write16(addr, frame);
// }

// static bool ad5274ReadCode(uint8_t addr, uint8_t &codeOut)
// {
//   uint16_t rdWord = 0;
//   const uint16_t cmdFrame = makeFrame(CMD_READ_RDAC, 0);

//   if (!ad5274Read16(addr, cmdFrame, rdWord)) {
//     return false;
//   }

//   // data zit in 10 LSB's; voor AD5274 -> terug naar 0..255 via >>2
//   const uint16_t data10 = rdWord & 0x03FF;
//   codeOut = (uint8_t)(data10 >> 2);
//   return true;
// }

// static uint8_t findAd5274Address()
// {
//   for (uint8_t a : AD5274_ADDR_CANDIDATES) {
//     if (i2cPing(a)) return a;
//   }
//   return 0; // 0 = niet gevonden
// }

// // ================== GLOBALS ==================
// static uint8_t g_addr = 0;

// // Kies hier je vaste testcodes (0..255). Als je "0-250 verandert weinig" ervaart,
// // neem juist extreme punten mee (0, 1, 2, 128, 254, 255) om meetfouten snel te spotten.
// static const uint8_t TEST_CODES[] = {
//   0, 1, 2, 8, 32, 64, 128, 192, 254, 255
// };
// static constexpr uint32_t STEP_DELAY_MS = 3000;

// // ================== SETUP ==================
// void setup()
// {
//   Serial.begin(115200);
//   delay(200);
//   Serial.println();
//   Serial.println("AD5274 I2C test (write + readback)");

//   Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
//   Wire.setClock(400000);

//   // Zoek adres
//   g_addr = findAd5274Address();
//   if (g_addr == 0) {
//     Serial.println("FOUT: geen AD5274 gevonden op 0x2C/0x2E/0x2F.");
//     Serial.println("Check: SDA/SCL, pull-ups, GND/VDD, ADDR-pin, voeding.");
//     while (true) { delay(1000); }
//   }

//   Serial.printf("AD5274 gevonden op adres: 0x%02X\n", g_addr);

//   // Enable RDAC update (write-protect openzetten)
//   Serial.println("Control register: RDAC update enable...");
//   if (!ad5274EnableRdacUpdate(g_addr)) {
//     Serial.println("FOUT: control write mislukt. RDAC writes kunnen geblokkeerd blijven.");
//   } else {
//     Serial.println("OK: RDAC update enabled.");
//   }

//   // Startwaarde
//   if (!ad5274WriteCode(g_addr, 0)) {
//     Serial.println("FOUT: init RDAC write mislukt.");
//   } else {
//     uint8_t rb = 0xFF;
//     bool ok = ad5274ReadCode(g_addr, rb);
//     Serial.printf("Init set code=0, readback=%s (%u)\n", ok ? "OK" : "FAIL", ok ? rb : 0);
//   }

//   Serial.println();
//   Serial.println("Meet tip: meet weerstand tussen W en A of W en B (niet A-B).");
//   Serial.println("Start testreeks...");
// }

// // ================== LOOP ==================
// void loop()
// {
//   static size_t idx = 0;

//   const uint8_t code = TEST_CODES[idx];

//   const bool wOK = ad5274WriteCode(g_addr, code);

//   uint8_t rb = 0;
//   const bool rOK = ad5274ReadCode(g_addr, rb);

//   const float r_set = rohmsFromCode(code);
//   const float r_rb  = rohmsFromCode(rb);

//   Serial.printf(
//     "STEP[%u] write=%s code=%3u (R≈%7.0fΩ) | read=%s rb=%3u (R≈%7.0fΩ)\n",
//     (unsigned)idx,
//     wOK ? "OK " : "FAIL",
//     code, r_set,
//     rOK ? "OK " : "FAIL",
//     rOK ? rb : 0,
//     rOK ? r_rb : 0.0f
//   );

//   idx = (idx + 1) % (sizeof(TEST_CODES) / sizeof(TEST_CODES[0]));
//   delay(STEP_DELAY_MS);
// }

