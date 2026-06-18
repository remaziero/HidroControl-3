#pragma once

void oled_init();

void oled_show_status(bool wifi_connected,
                      bool mqtt_connected,
                      const char* modo,
                      float fluxo_frio,
                      float fluxo_quente);