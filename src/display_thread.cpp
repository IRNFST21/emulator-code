#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AW9523.h>
#include <lvgl.h>

#include "ili9488_driver.hpp"
#include "display_thread.hpp"
#include "ui_screens.hpp"

// ---------------- BACKLIGHT ----------------
Adafruit_AW9523 aw;
constexpr uint8_t BL_PINS[] = {0, 1, 2, 3, 4, 5};

static lv_display_t* disp = nullptr;

// UI switch interval in ms
constexpr uint32_t UI_SWITCH_INTERVAL_MS = 10000; // 10 seconds for demo

enum class ActiveUI : uint8_t {
  UI1 = 0,
  UI2 = 1,
  UI3 = 2
};

static ActiveUI current_ui = ActiveUI::UI1;

// ---------------- MODEL (single source of truth) ----------------
static DisplayModel g_model;

// Demo-only direction helpers (niet in model, puur animatie)
static int   ui1_progress_dir = 1;  // +1 naar rechts, -1 naar links
static float ui2_dir = 1.0f;
static float ui3_dir = 1.0f;

static void model_init(DisplayModel& m)
{
  // UI1: curve + startwaarden
  m.ui1.curve_len = 32;
  int16_t init_curve[32] = {
      98, 95, 93, 92, 91, 90, 89, 88,
      87, 86, 84, 82, 80, 78, 75, 72,
      70, 67, 63, 58, 52, 45, 38, 30,
      25, 20, 15, 10, 7, 5, 3, 0
  };
  for (int i = 0; i < 32; ++i) m.ui1.curve[i] = init_curve[i];

  m.ui1.voltage_val      = 0.0f;
  m.ui1.current_val      = 0.0f;
  m.ui1.capacity_val     = 0.0f;
  m.ui1.runtime_sec      = 0;
  m.ui1.state_load       = true;
  m.ui1.nominal_v_val    = 0.0f;
  m.ui1.btn_capacity_val = 0.0f;
  m.ui1.progress_index   = 0;

  ui1_progress_dir = 1;

  // UI2
  m.ui2.set_voltage = 0.0f;
  m.ui2.meas_ampere = 0.0f;
  m.ui2.vmax        = 20.0f;
  ui2_dir = 1.0f;

  // UI3
  m.ui3.set_ampere   = 0.0f;
  m.ui3.meas_voltage = 0.0f;
  m.ui3.imax         = 5.0f;
  ui3_dir = 1.0f;
}

static void model_tick_1s(DisplayModel& m)
{
  // ---------------- UI1 ----------------
  m.ui1.voltage_val += 0.05f;
  if (m.ui1.voltage_val > 5.0f) m.ui1.voltage_val = 0.0f;

  m.ui1.current_val += 0.02f;
  if (m.ui1.current_val > 2.0f) m.ui1.current_val = 0.0f;

  m.ui1.runtime_sec++;

  m.ui1.capacity_val += 0.10f;
  if (m.ui1.capacity_val > 10.0f) m.ui1.capacity_val = 0.0f;

  m.ui1.state_load = !m.ui1.state_load;

  // Cursor heen en weer over curve
  m.ui1.progress_index += ui1_progress_dir;
  if (m.ui1.progress_index >= (m.ui1.curve_len - 1)) { m.ui1.progress_index = m.ui1.curve_len - 1; ui1_progress_dir = -1; }
  if (m.ui1.progress_index <= 0)                      { m.ui1.progress_index = 0;                      ui1_progress_dir =  1; }

  // Buttons: nominal voltage & capacity laten lopen
  m.ui1.nominal_v_val += 0.05f;
  if (m.ui1.nominal_v_val > 5.0f) m.ui1.nominal_v_val = 0.0f;

  m.ui1.btn_capacity_val += 0.10f;
  if (m.ui1.btn_capacity_val > 10.0f) m.ui1.btn_capacity_val = 0.0f;

  // ---------------- UI2 ----------------
  m.ui2.set_voltage += 0.4f * ui2_dir;
  if (m.ui2.set_voltage >= m.ui2.vmax) { m.ui2.set_voltage = m.ui2.vmax; ui2_dir = -1.0f; }
  if (m.ui2.set_voltage <= 0.0f)       { m.ui2.set_voltage = 0.0f;       ui2_dir =  1.0f; }

  m.ui2.meas_ampere = 0.2f + (m.ui2.set_voltage / m.ui2.vmax) * 1.8f;

  // ---------------- UI3 ----------------
  m.ui3.set_ampere += 0.25f * ui3_dir;
  if (m.ui3.set_ampere >= m.ui3.imax) { m.ui3.set_ampere = m.ui3.imax; ui3_dir = -1.0f; }
  if (m.ui3.set_ampere <= 0.0f)       { m.ui3.set_ampere = 0.0f;       ui3_dir =  1.0f; }

  m.ui3.meas_voltage = 12.0f - (m.ui3.set_ampere / m.ui3.imax) * 6.0f;
}

// ---------------- BACKLIGHT INIT ----------------
static void backlight_init_and_on() {
  Wire.begin(21, 19);
  if (!aw.begin(0x58)) {
    Serial.println("AW9523 niet gevonden!");
    return;
  }
  Serial.println("AW9523 OK, backlight aan");
  for (auto pin : BL_PINS) {
    aw.pinMode(pin, AW9523_LED_MODE);
    aw.analogWrite(pin, 255);
  }
}

// ---------------- LVGL DISPLAY PORT ----------------
static void my_flush_cb(lv_display_t* disp_drv, const lv_area_t* area, uint8_t* px_map) {
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;

  // Driver verwacht bytes (RGB565 little-endian)
  ili9488_push_pixels(area->x1, area->y1, w, h, (const uint8_t*)px_map);

  lv_display_flush_ready(disp_drv);
}

static void lvgl_port_init() {
  const uint16_t hor_res = 480;
  const uint16_t ver_res = 320;

  disp = lv_display_create(hor_res, ver_res);

  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, my_flush_cb);

  static const uint16_t DRAW_BUF_LINES = 10;
  static lv_color_t buf1[480 * DRAW_BUF_LINES];
  static lv_color_t buf2[480 * DRAW_BUF_LINES];
  lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void display_task(void* pvParameters) {
  Serial.println("Display task gestart");

  backlight_init_and_on();
  ili9488_init();

  lv_init();
  lvgl_port_init();

  model_init(g_model);

  // Start met UI1
  current_ui = ActiveUI::UI1;
  ui1_create();
  ui1_update(g_model);

  uint32_t last_update = millis();
  uint32_t last_switch = millis();

  while (true) {
    // LVGL tick + timers
    lv_tick_inc(5);
    lv_timer_handler();

    const uint32_t now = millis();

    // Elke seconde: model + UI updaten
    if (now - last_update >= 1000) {
      last_update = now;

      model_tick_1s(g_model);

      switch (current_ui) {
        case ActiveUI::UI1: ui1_update(g_model); break;
        case ActiveUI::UI2: ui2_update(g_model); break;
        case ActiveUI::UI3: ui3_update(g_model); break;
      }
    }

    // Elke UI_SWITCH_INTERVAL_MS ms: UI wisselen
    if (now - last_switch >= UI_SWITCH_INTERVAL_MS) {
      last_switch = now;

      current_ui = static_cast<ActiveUI>((static_cast<uint8_t>(current_ui) + 1) % 3);

      switch (current_ui) {
        case ActiveUI::UI1: ui1_create(); ui1_update(g_model); break;
        case ActiveUI::UI2: ui2_create(); ui2_update(g_model); break;
        case ActiveUI::UI3: ui3_create(); ui3_update(g_model); break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
