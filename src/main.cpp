#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <Adafruit_AW9523.h>
#include "ili9488_driver.hpp"

// Backlight via AW9523
Adafruit_AW9523 aw;
constexpr uint8_t BL_PINS[] = {0, 1, 2, 3, 4, 5};

static lv_display_t* disp = nullptr;

// ---------------- BACKLIGHT ----------------
void backlight_init_and_on() {
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

// ---------------- LVGL KOPPELING ----------------
void lvgl_port_init() {
  uint16_t hor_res = 320;
  uint16_t ver_res = 480;

  disp = lv_display_create(hor_res, ver_res);

  // flush_cb blijft hetzelfde
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

  // HEEL BELANGRIJK: GEEN rotatie meer
  // lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);   // blijft uit-gemerkt


  // <<< hier rotatie instellen >>>
  // lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);


  static const uint16_t DRAW_BUF_LINES = 10;
  static lv_color_t buf1[320 * DRAW_BUF_LINES];
  static lv_color_t buf2[320 * DRAW_BUF_LINES];

  lv_display_set_buffers(disp,
                         buf1,
                         buf2,
                         sizeof(buf1),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

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

//---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("LVGL + ILI9488 baseline test");

  backlight_init_and_on();

  // Display init en harde clear in native oriëntatie
  ili9488_init();
  ili9488_fill_screen(0x0000);  // alles zwart maken

  // LVGL init
  lv_init();
  lvgl_port_init();

  // LVGL scherm opruimen (oude objecten weg, voor de zekerheid)
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  lv_obj_t* label = lv_label_create(scr);
  lv_label_set_text(label, "Hallo LVGL!");
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_center(label);

}



//---------------- LOOP ----------------
void loop() {
  // tijd doorgeven aan LVGL
  lv_tick_inc(5);
  lv_timer_handler();
  delay(5);
}
