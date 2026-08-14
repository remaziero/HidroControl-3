#pragma once

#if defined(WOKWI_SIM)

#define WIFI_STA_SSID   "Wokwi-GUEST"
#define WIFI_STA_PASS   ""

#define MQTT_BROKER_URI \
    "mqtts://3f64e3d334e045448a9277d405a8f0da.s1.eu.hivemq.cloud:8883"

#define MQTT_USERNAME   "hidrocontrol-wokwi"
#define MQTT_PASSWORD   "Camila12@"

#elif defined(HIDROCONTROL_V3)

// AC220 real: manteremos para configurar depois.
#define WIFI_STA_SSID   "S24 FE de Renato"
#define WIFI_STA_PASS   "Camila@12"

#define MQTT_BROKER_URI ""
#define MQTT_USERNAME   ""
#define MQTT_PASSWORD   ""

#else

#error "Ambiente não definido"

#endif
