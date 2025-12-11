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

// UI-objecten voor de laadcurve + "tabel"
static lv_obj_t* chart = nullptr;
static lv_chart_series_t* series = nullptr;
static lv_obj_t* label_a = nullptr;
static lv_obj_t* label_b = nullptr;
static lv_obj_t* label_total = nullptr;

// Dummy waarden voor test
static float sum_a = 0.0f;
static float sum_b = 0.0f;
static int   chart_x = 0;

enum class ActiveUI : uint8_t {
  UI1 = 0,
  UI2 = 1,
};

static ActiveUI current_ui = ActiveUI::UI1;



// ---------------- BACKLIGHT INIT ----------------
static void backlight_init_and_on() {
  Wire.begin(21, 19);
  if (!aw.begin(0x58)) {
    Serial.println("AW9523 niet gevonden!");
    return;
  }
  Serial.println("AW9523 OK, backlight aan");
  for (auto p : BL_PINS) {
    aw.pinMode(p, AW9523_LED_MODE);
    aw.analogWrite(p, 255);
  }
}

// ---------------- LVGL / DISPLAY KOPPELING ----------------
static void lvgl_port_init() {
  // Logische resolutie in landscape
  uint16_t hor_res = 480;
  uint16_t ver_res = 320;

  disp = lv_display_create(hor_res, ver_res);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

  // Draw-buffers (double buffering, 10 lijnen hoog)
  static const uint16_t DRAW_BUF_LINES = 10;
  static lv_color_t buf1[320 * DRAW_BUF_LINES];
  static lv_color_t buf2[320 * DRAW_BUF_LINES];

  lv_display_set_buffers(disp,
                         buf1,
                         buf2,
                         sizeof(buf1),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  // Flush callback: LVGL -> ILI9488
  lv_display_set_flush_cb(disp,
    [](lv_display_t* d, const lv_area_t* area, uint8_t* px_map) {
      uint16_t x = area->x1;
      uint16_t y = area->y1;
      uint16_t w = lv_area_get_width(area);
      uint16_t h = lv_area_get_height(area);

      ili9488_push_pixels(x, y, w, h, px_map);

      lv_display_flush_ready(d);
    }
  );

  lv_display_set_default(disp);
}

// ---------------- UI OPBOUW ----------------
static void ui_create_initial_screen() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  // Achtergrond zwart
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  // --- CHART (laadcurve) ---
  chart = lv_chart_create(scr);
  lv_obj_set_size(chart, 440, 180);
  lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 10);

  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_point_count(chart, 50);

  // Styling
  lv_obj_set_style_bg_color(chart, lv_color_hex(0x101010), LV_PART_MAIN);
  lv_obj_set_style_border_color(chart, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_border_width(chart, 1, LV_PART_MAIN);

  // Serie (groene lijn)
  series = lv_chart_add_series(chart,
                               lv_color_hex(0x00FF00),
                               LV_CHART_AXIS_PRIMARY_Y);

  for (int i = 0; i < 50; i++) {
    lv_chart_set_value_by_id(chart, series, i, 0);
  }

  // --- LABELS ("tabel" met optellende waardes) ---
  label_a = lv_label_create(scr);
  label_b = lv_label_create(scr);
  label_total = lv_label_create(scr);

  lv_obj_align(label_a, LV_ALIGN_BOTTOM_LEFT, 20, -60);
  lv_obj_align(label_b, LV_ALIGN_BOTTOM_LEFT, 20, -35);
  lv_obj_align(label_total, LV_ALIGN_BOTTOM_LEFT, 20, -10);

  lv_obj_set_style_text_color(label_a, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(label_b, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(label_total, lv_color_hex(0xFFFFFF), 0);

  lv_label_set_text(label_a, "Som A : 0.0");
  lv_label_set_text(label_b, "Som B : 0.0");
  lv_label_set_text(label_total, "Totaal: 0.0");
}

// ---------------- PERIODIEKE UPDATE ----------------
static void ui_update_every_second() {
  // Dummy data laten oplopen
  sum_a += 1.2f;
  sum_b += 0.8f;
  float total = sum_a + sum_b;

  // Labels updaten
  char buf[32];

  snprintf(buf, sizeof(buf), "Som A : %.1f", sum_a);
  lv_label_set_text(label_a, buf);

  snprintf(buf, sizeof(buf), "Som B : %.1f", sum_b);
  lv_label_set_text(label_b, buf);

  snprintf(buf, sizeof(buf), "Totaal: %.1f", total);
  lv_label_set_text(label_total, buf);

  // Chart updaten: total als "laadcurve"
  lv_chart_set_value_by_id(chart, series, chart_x, (int)(total));
  chart_x++;
  if (chart_x >= 50) {
    chart_x = 0;
  }

  lv_chart_refresh(chart);
}

// ---------------- DISPLAY TASK ----------------
void display_task(void* pvParameters) {
  (void)pvParameters;

  Serial.println("Display task start");

  backlight_init_and_on();

  // Display init (met kleuren + landscape via MADCTL in ili9488_init)
  ili9488_init();

  // Eventueel eerst even zwart vullen
  ili9488_fill_screen(0x0000);

  // LVGL init
  lv_init();
  lvgl_port_init();

  // Start met UI1
  current_ui = ActiveUI::UI1;
  ui1_create();

  uint32_t last_update = millis();
  uint32_t last_switch = millis();

  while (true) {
    // LVGL tick + timers
    lv_tick_inc(5);
    lv_timer_handler();

    uint32_t now = millis();

    // Elke 1 seconde: actieve UI updaten
    if (now - last_update >= 1000) {
      last_update = now;

      switch (current_ui) {
        case ActiveUI::UI1:
          ui1_update();
          break;
        case ActiveUI::UI2:
          ui2_update();
          break;
      }
    }

    // Elke 100 seconden: UI wisselen
    if (now - last_switch >= 100000) {
      last_switch = now;

      // Wissel tussen UI1 en UI2
      if (current_ui == ActiveUI::UI1) {
        current_ui = ActiveUI::UI2;
        ui2_create();
      } else {
        current_ui = ActiveUI::UI1;
        ui1_create();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

