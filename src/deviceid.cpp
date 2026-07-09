#include "deviceid.h"

#include <stdio.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "DEVICEID";

static char s_device_id[32] = "ac220-unknown";
static char s_chip_id[16]   = "unknown";

void deviceid_init()
{
    uint8_t mac[6] = {0};

    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf(s_chip_id, sizeof(s_chip_id),
             "%02X%02X%02X",
             mac[3], mac[4], mac[5]);

    snprintf(s_device_id, sizeof(s_device_id),
             "ac220-%s", s_chip_id);

    ESP_LOGI(TAG, "DeviceID: %s", s_device_id);
    ESP_LOGI(TAG, "ChipID: %s", s_chip_id);
}

const char* deviceid_get()
{
    return s_device_id;
}

const char* deviceid_chip()
{
    return s_chip_id;
}