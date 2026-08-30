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

#if defined(DEVICE_AC220_7DD764)

#define MQTT_USERNAME   "ac220-7DD764"
#define MQTT_PASSWORD   "Camila12@"

#elif defined(DEVICE_AC220_888334)

#define MQTT_USERNAME   "ac220-888334"
#define MQTT_PASSWORD   "Camila12@"

#elif defined(DEVICE_AC220_306CF0)

#define MQTT_USERNAME   "ac220-306CF0"
#define MQTT_PASSWORD   "Camila12@"

#else

#error "Dispositivo AC220 nao definido"

#endif

#else

#error "Ambiente nao definido"

#endif
