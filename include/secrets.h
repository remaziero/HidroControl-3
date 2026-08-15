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
#define WIFI_STA_SSID   "VIVOFIBRA-B8D1"
#define WIFI_STA_PASS   "jjBPu4aZNL"

#define MQTT_BROKER_URI \
    "mqtts://3f64e3d334e045448a9277d405a8f0da.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME   "ac220-888334"
#define MQTT_PASSWORD   "Camila12@"

#else

#error "Ambiente não definido"

#endif
