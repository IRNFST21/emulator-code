#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel;
  lgfx::Bus_Parallel8 _bus;

public:
  LGFX() {
    { // --- Bus config ---
      auto b = _bus.config();

      // Gebruik de gewone GPIO-parallel bus
      b.port       = 0;              // BELANGRIJK op ESP32-S3
      b.freq_write = 12000000;       // 12 MHz om veilig te beginnen

      // Neem hier precies dezelfde waarden als in je bit-bang test:
      b.pin_wr  = 20;                // WR (pas aan als je in je test een andere gebruikte)
      b.pin_rd  = -1;                // RD niet gebruikt (op FPC aan 3V3)
      b.pin_rs  = 45;                // RS/DC

      // Databus D0..D7
      b.pin_d0  = 10;
      b.pin_d1  = 11;
      b.pin_d2  = 12;
      b.pin_d3  = 13;
      b.pin_d4  = 14;
      b.pin_d5  = 15;
      b.pin_d6  = 16;
      b.pin_d7  = 17;

      _bus.config(b);
      _panel.setBus(&_bus);
    }

    { // --- Panel config ---
      auto p = _panel.config();

      p.pin_cs   = 2;                // CS LCD
      p.pin_rst  = -1;               // als RESET op 3V3 zit; anders hier je RESET-GPIO
      p.pin_busy = -1;               // TE niet gebruikt

      p.panel_width   = 320;
      p.panel_height  = 480;
      p.memory_width  = 320;
      p.memory_height = 480;

      p.offset_x = 0;
      p.offset_y = 0;

      p.readable   = false;          // RD = -1, dus niet lezen
      p.invert     = false;
      p.rgb_order  = false;
      p.dlen_16bit = true;           // 16-bit RGB565 (zoals in je test met 0x3A / 0x55)
      p.bus_shared = false;

      _panel.config(p);
    }

    setPanel(&_panel);
  }
};

extern LGFX gfx;
