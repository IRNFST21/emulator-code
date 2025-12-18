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