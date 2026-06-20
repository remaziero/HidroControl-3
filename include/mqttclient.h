#pragma once

void mqttclient_init();

bool mqttclient_is_connected();

void mqttclient_publish_telemetry(const char* modo,
                                  float fluxo_frio,
                                  float fluxo_quente,
                                  bool rele_frio,
                                  bool rele_quente,
                                  bool wifi_connected);