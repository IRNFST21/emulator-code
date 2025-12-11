#include <lvgl.h>
#include <Arduino.h>
// ================= UI 1: Laadcurve + 3 som-labels =================

static lv_obj_t* ui1_chart = nullptr;
static lv_chart_series_t* ui1_series = nullptr;
static lv_obj_t* ui1_label_a = nullptr;
static lv_obj_t* ui1_label_b = nullptr;
static lv_obj_t* ui1_label_total = nullptr;

static float ui1_sum_a = 0.0f;
static float ui1_sum_b = 0.0f;
static int   ui1_chart_x = 0;

void ui1_create() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  // Achtergrond zwart
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  // Chart
  ui1_chart = lv_chart_create(scr);
  lv_obj_set_size(ui1_chart, 440, 180);
  lv_obj_align(ui1_chart, LV_ALIGN_TOP_MID, 0, 10);

  lv_chart_set_type(ui1_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_range(ui1_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_point_count(ui1_chart, 50);

  lv_obj_set_style_bg_color(ui1_chart, lv_color_hex(0x101010), LV_PART_MAIN);
  lv_obj_set_style_border_color(ui1_chart, lv_color_hex(0x404040), LV_PART_MAIN);
  lv_obj_set_style_border_width(ui1_chart, 1, LV_PART_MAIN);

  ui1_series = lv_chart_add_series(ui1_chart,
                                   lv_color_hex(0x00FF00),
                                   LV_CHART_AXIS_PRIMARY_Y);

  for (int i = 0; i < 50; i++) {
    lv_chart_set_value_by_id(ui1_chart, ui1_series, i, 0);
  }

  // Labels onderaan
  ui1_label_a = lv_label_create(scr);
  ui1_label_b = lv_label_create(scr);
  ui1_label_total = lv_label_create(scr);

  lv_obj_align(ui1_label_a, LV_ALIGN_BOTTOM_LEFT, 20, -60);
  lv_obj_align(ui1_label_b, LV_ALIGN_BOTTOM_LEFT, 20, -35);
  lv_obj_align(ui1_label_total, LV_ALIGN_BOTTOM_LEFT, 20, -10);

  lv_obj_set_style_text_color(ui1_label_a, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(ui1_label_b, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_color(ui1_label_total, lv_color_hex(0xFFFFFF), 0);

  lv_label_set_text(ui1_label_a, "Som A : 0.0");
  lv_label_set_text(ui1_label_b, "Som B : 0.0");
  lv_label_set_text(ui1_label_total, "Totaal: 0.0");

  // Startwaardes resetten
  ui1_sum_a = 0.0f;
  ui1_sum_b = 0.0f;
  ui1_chart_x = 0;
}

void ui1_update() {
  ui1_sum_a += 1.2f;
  ui1_sum_b += 0.8f;
  float total = ui1_sum_a + ui1_sum_b;

  char buf[32];

  snprintf(buf, sizeof(buf), "Som A : %.1f", ui1_sum_a);
  lv_label_set_text(ui1_label_a, buf);

  snprintf(buf, sizeof(buf), "Som B : %.1f", ui1_sum_b);
  lv_label_set_text(ui1_label_b, buf);

  snprintf(buf, sizeof(buf), "Totaal: %.1f", total);
  lv_label_set_text(ui1_label_total, buf);

  lv_chart_set_value_by_id(ui1_chart, ui1_series, ui1_chart_x, (int)(total));
  ui1_chart_x++;
  if (ui1_chart_x >= 50) {
    ui1_chart_x = 0;
  }

  lv_chart_refresh(ui1_chart);
}


// ================= UI 2: Simpel dashboard / testscreen =================

static lv_obj_t* ui2_label_title = nullptr;
static lv_obj_t* ui2_label_counter = nullptr;
static int       ui2_counter = 0;

void ui2_create() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  // Donkere achtergrond
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x001020), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  ui2_label_title = lv_label_create(scr);
  lv_label_set_text(ui2_label_title, "UI 2 - Dashboard Test");
  lv_obj_align(ui2_label_title, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_color(ui2_label_title, lv_color_hex(0xFFFFFF), 0);

  ui2_label_counter = lv_label_create(scr);
  lv_label_set_text(ui2_label_counter, "Counter: 0");
  lv_obj_align(ui2_label_counter, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(ui2_label_counter, lv_color_hex(0xFFFF00), 0);

  ui2_counter = 0;
}

void ui2_update() {
  ui2_counter++;

  char buf[32];
  snprintf(buf, sizeof(buf), "Counter: %d", ui2_counter);
  lv_label_set_text(ui2_label_counter, buf);
}
