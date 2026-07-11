#include "mqttconfig.h"
#include "secrets.h"

static const MqttConfig s_config = {
    .uri = MQTT_BROKER_URI,
    .username = MQTT_USERNAME,
    .password = MQTT_PASSWORD,
    .keepalive_seconds = 60,
    .reconnect_timeout_ms = 5000,
    .network_timeout_ms = 10000
};

const MqttConfig& mqttconfig_get()
{
    return s_config;
}