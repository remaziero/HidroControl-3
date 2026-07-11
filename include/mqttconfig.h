#pragma once

struct MqttConfig {
    const char *uri;
    const char *username;
    const char *password;
    int keepalive_seconds;
    int reconnect_timeout_ms;
    int network_timeout_ms;
};

const MqttConfig& mqttconfig_get();