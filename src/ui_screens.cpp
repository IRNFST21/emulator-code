// ui_screens.cpp
#include "ui_screens.hpp"
#include <Arduino.h>
#include <lvgl.h>

// --- UI color palette ---
#define UI_COL_BG              0x000000   // global background
#define UI_COL_TEXT            0xEDBE0E   // main text (yellow)
#define UI_COL_CHART_BG        0x000000   // chart background
#define UI_COL_CHART_BORDER    0xEDBE0E   // chart border
#define UI_COL_CHART_SERIES    0xEDBE0E   // discharge curve
#define UI_COL_CHART_LINE      0xEDBE0E   // helper line in chart
#define UI_COL_AXIS_TEXT       0xEDBE0E   // axis labels
#define UI_COL_MEAS_TEXT       0xEDBE0E   // measurement & curve info text
#define UI_COL_SIDEBAR_BG      0xEDBE0E   // sidebar background
#define UI_COL_SIDEBAR_BORDER  0x000000   // sidebar border lines
#define UI_COL_BUTTON_BG       0xEDBE0E   // button background (yellow)
#define UI_COL_BUTTON_BORDER   0x000000   // button border
#define UI_COL_BUTTON_TEXT     0x000000   // button text
#define UI_COL_UI2_BG          0x000000   // background for UI2
#define UI_COL_UI2_TEXT        0xEDBE0E   // UI2 text

// ---------- UI1: Emulate / laadcurve-scherm ----------

// pointers bewaren voor later gebruik / updates
static lv_obj_t* ui1_chart             = nullptr;
static lv_chart_series_t* ui1_series   = nullptr;

// beneden labels
static lv_obj_t* ui1_label_meas_title  = nullptr;
static lv_obj_t* ui1_label_v_meas      = nullptr;
static lv_obj_t* ui1_label_i_meas      = nullptr;

static lv_obj_t* ui1_label_curve_title = nullptr;
static lv_obj_t* ui1_label_runtime     = nullptr;
static lv_obj_t* ui1_label_capacity    = nullptr;
static lv_obj_t* ui1_label_state       = nullptr;

// rechter kolom “knoppen”
static lv_obj_t* ui1_btn_choose_curve  = nullptr;
static lv_obj_t* ui1_btn_choose_setp   = nullptr;
static lv_obj_t* ui1_btn_nominal_v     = nullptr;
static lv_obj_t* ui1_btn_capacity      = nullptr;
static lv_obj_t* ui1_btn_reset         = nullptr;

// labels ín de knoppen (voor dynamische tekst)
static lv_obj_t* ui1_lbl_btn_nominal_v = nullptr;
static lv_obj_t* ui1_lbl_btn_capacity  = nullptr;

// lijn in de grafiek
static lv_obj_t* ui1_progress_line     = nullptr;
static lv_point_precise_t ui1_progress_pts[2];

// dummy curve data (zelfde vorm als je eerder had)
static const int16_t ui1_curve_vals[32] = {
    98, 95, 93, 92, 91, 90, 89, 88,
    87, 86, 84, 82, 80, 78, 75, 72,
    70, 67, 63, 58, 52, 45, 38, 30,
    25, 20, 15, 10, 7, 5, 3, 0
};

// dynamische state voor dummy-animatie
static float    ui1_voltage_val        = 0.0f;
static float    ui1_current_val        = 0.0f;
static float    ui1_capacity_val       = 0.0f;
static uint32_t ui1_runtime_sec        = 0;
static bool     ui1_state_load         = true;

static float    ui1_nominal_v_val      = 0.0f;
static float    ui1_btn_capacity_val   = 0.0f;

static int      ui1_progress_index     = 0;
static int      ui1_progress_dir       = 1;   // +1 naar rechts, -1 naar links

// helper: verticale lijnpositie updaten op basis van ui1_progress_index
static void ui1_update_progress_line()
{
    if (!ui1_chart || !ui1_progress_line) return;

    const int point_count = 32;
    int idx = ui1_progress_index;
    if (idx < 0) idx = 0;
    if (idx > point_count - 1) idx = point_count - 1;

    int graph_width  = lv_obj_get_width(ui1_chart);
    int graph_height = lv_obj_get_height(ui1_chart);
    if (graph_width <= 1 || graph_height <= 1) return;

    // X-positie over de breedte van de chart
    int x = (graph_width - 1) * idx / (point_count - 1);

    // Waarde (0..100) uit de curve
    int16_t v = ui1_curve_vals[idx];

    // Map waarde naar pixel (0 = boven, graph_height-1 = onder)
    int y_curve = (graph_height - 1) - (graph_height - 1) * v / 100;

    // Lijn van onderkant chart tot aan de curve
    ui1_progress_pts[0].x = x;
    ui1_progress_pts[0].y = graph_height - 2;   // net boven onderste rand
    ui1_progress_pts[1].x = x;
    ui1_progress_pts[1].y = y_curve;

    lv_line_set_points(ui1_progress_line, ui1_progress_pts, 2);
}

void ui1_create() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  // baseline voor dynamische waardes
  ui1_voltage_val      = 0.0f;
  ui1_current_val      = 0.0f;
  ui1_capacity_val     = 0.0f;
  ui1_runtime_sec      = 0;
  ui1_state_load       = true;
  ui1_nominal_v_val    = 0.0f;
  ui1_btn_capacity_val = 0.0f;
  ui1_progress_index   = 0;
  ui1_progress_dir     = 1;

  // -------- achtergrond / hoofdvlak --------
  lv_obj_set_style_bg_color(scr, lv_color_hex(UI_COL_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  // Titel bovenaan
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Emulate");
  lv_obj_set_style_text_color(title, lv_color_hex(UI_COL_TEXT), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

  // -------- linker inhoudsgebied (grafiek + info) --------
  const int left_margin   = 30;
  const int top_margin    = 25;
  const int graph_width   = 310;
  const int graph_height  = 180;

  ui1_chart = lv_chart_create(scr);
  lv_obj_set_size(ui1_chart, graph_width, graph_height);
  lv_obj_align(ui1_chart, LV_ALIGN_TOP_LEFT, left_margin, top_margin);

  lv_obj_clear_flag(ui1_chart, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(ui1_chart, LV_SCROLLBAR_MODE_OFF);

  lv_chart_set_type(ui1_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_range(ui1_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_point_count(ui1_chart, 32);

  lv_obj_set_style_bg_color(ui1_chart, lv_color_hex(UI_COL_CHART_BG), LV_PART_MAIN);
  lv_obj_set_style_border_color(ui1_chart, lv_color_hex(UI_COL_CHART_BORDER), LV_PART_MAIN);
  lv_obj_set_style_border_width(ui1_chart, 1, LV_PART_MAIN);

  // Discharge-curve dummy data (alleen voor look & feel)
  ui1_series = lv_chart_add_series(ui1_chart,
                                   lv_color_hex(UI_COL_CHART_SERIES),
                                   LV_CHART_AXIS_PRIMARY_Y);

  for (int i = 0; i < 32; ++i) {
    lv_chart_set_value_by_id(ui1_chart, ui1_series, i, ui1_curve_vals[i]);
  }
  lv_chart_refresh(ui1_chart);

  // Verticale “cursor”-lijn
  ui1_progress_line = lv_line_create(ui1_chart);
  lv_obj_set_style_line_color(ui1_progress_line, lv_color_hex(UI_COL_CHART_LINE), 0);
  lv_obj_set_style_line_width(ui1_progress_line, 2, 0);
  lv_obj_set_style_line_dash_width(ui1_progress_line, 6, 0);
  lv_obj_set_style_line_dash_gap(ui1_progress_line, 4, 0);
  ui1_update_progress_line(); // init

  // As-labels
  lv_obj_t* lbl_x = lv_label_create(scr);
  lv_label_set_text(lbl_x, "Capacity ->");
  lv_obj_set_style_text_color(lbl_x, lv_color_hex(UI_COL_AXIS_TEXT), 0);
  lv_obj_align(lbl_x, LV_ALIGN_TOP_LEFT, left_margin + 60, top_margin + graph_height + 5);

  lv_obj_t* lbl_y = lv_label_create(scr);
  lv_label_set_text(lbl_y, "Voltage ->");
  lv_obj_set_style_text_color(lbl_y, lv_color_hex(UI_COL_AXIS_TEXT), 0);
  lv_obj_align(lbl_y, LV_ALIGN_TOP_LEFT, left_margin - 20, top_margin + graph_height/2 + 30);
  lv_obj_set_style_transform_angle(lbl_y, 2700, 0); // 90 graden roteren

  // -------- onderbalk: meetwaarden en curve-info --------
  const int bottom_y = top_margin + graph_height + 35;

  // Measurements block (links)
  ui1_label_meas_title = lv_label_create(scr);
  lv_label_set_text(ui1_label_meas_title, "Measurements:");
  lv_obj_set_style_text_color(ui1_label_meas_title, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align(ui1_label_meas_title, LV_ALIGN_TOP_LEFT, left_margin, bottom_y - 8);

  ui1_label_v_meas = lv_label_create(scr);
  lv_label_set_text(ui1_label_v_meas, "Voltage = 0.00 V");
  lv_obj_set_style_text_color(ui1_label_v_meas, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align_to(ui1_label_v_meas, ui1_label_meas_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  ui1_label_i_meas = lv_label_create(scr);
  lv_label_set_text(ui1_label_i_meas, "Ampere = 0.00 A");
  lv_obj_set_style_text_color(ui1_label_i_meas, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align_to(ui1_label_i_meas, ui1_label_v_meas, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  // Curve-info block (rechts van measurements)
  ui1_label_curve_title = lv_label_create(scr);
  lv_label_set_text(ui1_label_curve_title, "Curve:");
  lv_obj_set_style_text_color(ui1_label_curve_title, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align(ui1_label_curve_title, LV_ALIGN_TOP_LEFT, left_margin + 150, bottom_y - 8);

  ui1_label_runtime = lv_label_create(scr);
  lv_label_set_text(ui1_label_runtime, "Run-time = 00:00");
  lv_obj_set_style_text_color(ui1_label_runtime, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align_to(ui1_label_runtime, ui1_label_curve_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  ui1_label_capacity = lv_label_create(scr);
  lv_label_set_text(ui1_label_capacity, "Capacity = 0.00 F");
  lv_obj_set_style_text_color(ui1_label_capacity, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align_to(ui1_label_capacity, ui1_label_runtime, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  ui1_label_state = lv_label_create(scr);
  lv_label_set_text(ui1_label_state, "Current state = load/unload");
  lv_obj_set_style_text_color(ui1_label_state, lv_color_hex(UI_COL_MEAS_TEXT), 0);
  lv_obj_align_to(ui1_label_state, ui1_label_capacity, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

  // -------- rechter kolom: 5 “knop”-blokken --------
  lv_obj_t* sidebar = lv_obj_create(scr);
  lv_obj_set_size(sidebar, 120, 300);
  lv_obj_align(sidebar, LV_ALIGN_RIGHT_MID, -5, 5);
  lv_obj_set_style_bg_color(sidebar, lv_color_hex(UI_COL_SIDEBAR_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(sidebar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(sidebar, lv_color_hex(UI_COL_SIDEBAR_BORDER), LV_PART_MAIN);
  lv_obj_set_style_border_width(sidebar, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_all(sidebar, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(sidebar, 4, LV_PART_MAIN);
  lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(sidebar,
                        LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_START);

  lv_obj_clear_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(sidebar, LV_SCROLLBAR_MODE_OFF);

  auto make_btn = [&](const char* txt) -> lv_obj_t* {
    lv_obj_t* btn = lv_btn_create(sidebar);

    // Breedte 100%, hoogte door flex/content bepalen
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_height(btn, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1); // verdeel hoogte gelijk

    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COL_BUTTON_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(UI_COL_BUTTON_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);

    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(btn, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_center(l);

    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(l, lv_color_hex(UI_COL_BUTTON_TEXT), 0);

    return btn;
  };

  ui1_btn_choose_curve = make_btn("Choose Curve");
  ui1_btn_choose_setp  = make_btn("Choose Setpoint");
  ui1_btn_nominal_v    = make_btn("Nominal voltage:\n0.00 V");
  ui1_btn_capacity     = make_btn("Capacity\n0.00 F");
  ui1_btn_reset        = make_btn("Reset");

  // labels uit de knoppen trekken zodat we ze kunnen updaten
  if (ui1_btn_nominal_v) {
    ui1_lbl_btn_nominal_v = lv_obj_get_child(ui1_btn_nominal_v, 0);
  }
  if (ui1_btn_capacity) {
    ui1_lbl_btn_capacity = lv_obj_get_child(ui1_btn_capacity, 0);
  }
}

void ui1_update() {
  char buf[64];

  // ---- Measurements: voltage & ampere optellen ----
  ui1_voltage_val += 0.05f;
  if (ui1_voltage_val > 5.0f) ui1_voltage_val = 0.0f;

  ui1_current_val += 0.02f;
  if (ui1_current_val > 2.0f) ui1_current_val = 0.0f;

  snprintf(buf, sizeof(buf), "Voltage = %.2f V", ui1_voltage_val);
  lv_label_set_text(ui1_label_v_meas, buf);

  snprintf(buf, sizeof(buf), "Ampere = %.2f A", ui1_current_val);
  lv_label_set_text(ui1_label_i_meas, buf);

  // ---- Curve-info: runtime, capacity, state ----
  ui1_runtime_sec++;
  uint32_t minutes = ui1_runtime_sec / 60;
  uint32_t seconds = ui1_runtime_sec % 60;

  snprintf(buf, sizeof(buf), "Run-time = %02u:%02u", minutes, seconds);
  lv_label_set_text(ui1_label_runtime, buf);

  ui1_capacity_val += 0.10f;
  if (ui1_capacity_val > 10.0f) ui1_capacity_val = 0.0f;

  snprintf(buf, sizeof(buf), "Capacity = %.2f F", ui1_capacity_val);
  lv_label_set_text(ui1_label_capacity, buf);

  ui1_state_load = !ui1_state_load;
  lv_label_set_text(ui1_label_state,
                    ui1_state_load ? "Current state = load"
                                   : "Current state = unload");

  // ---- Verticale lijn heen en weer over de curve ----
  ui1_progress_index += ui1_progress_dir;
  if (ui1_progress_index >= 31) {
    ui1_progress_index = 31;
    ui1_progress_dir   = -1;
  } else if (ui1_progress_index <= 0) {
    ui1_progress_index = 0;
    ui1_progress_dir   = 1;
  }
  ui1_update_progress_line();

  // ---- Buttons: nominal voltage & capacity laten lopen ----
  ui1_nominal_v_val += 0.05f;
  if (ui1_nominal_v_val > 5.0f) ui1_nominal_v_val = 0.0f;

  if (ui1_lbl_btn_nominal_v) {
    snprintf(buf, sizeof(buf), "Nominal voltage:\n%.2f V", ui1_nominal_v_val);
    lv_label_set_text(ui1_lbl_btn_nominal_v, buf);
  }

  ui1_btn_capacity_val += 0.10f;
  if (ui1_btn_capacity_val > 10.0f) ui1_btn_capacity_val = 0.0f;

  if (ui1_lbl_btn_capacity) {
    snprintf(buf, sizeof(buf), "Capacity\n%.2f F", ui1_btn_capacity_val);
    lv_label_set_text(ui1_lbl_btn_capacity, buf);
  }
}

// ================= UI 2: Simpel dashboard / testscreen =================

static lv_obj_t* ui2_label_title   = nullptr;
static lv_obj_t* ui2_label_counter = nullptr;
static int       ui2_counter       = 0;

void ui2_create() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  // Donkere achtergrond
  lv_obj_set_style_bg_color(scr, lv_color_hex(UI_COL_UI2_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  ui2_label_title = lv_label_create(scr);
  lv_label_set_text(ui2_label_title, "UI 2 - Dashboard Test");
  lv_obj_align(ui2_label_title, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_text_color(ui2_label_title, lv_color_hex(UI_COL_UI2_TEXT), 0);

  ui2_label_counter = lv_label_create(scr);
  lv_label_set_text(ui2_label_counter, "Counter: 0");
  lv_obj_align(ui2_label_counter, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(ui2_label_counter, lv_color_hex(UI_COL_UI2_TEXT), 0);

  ui2_counter = 0;
}

void ui2_update() {
  ui2_counter++;

  char buf[32];
  snprintf(buf, sizeof(buf), "Counter: %d", ui2_counter);
  lv_label_set_text(ui2_label_counter, buf);
}
