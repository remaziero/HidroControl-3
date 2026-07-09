#include "timekeeper.h"

#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "esp_log.h"
#include "esp_sntp.h"

static const char *TAG = "TIME";

static const char *DEFAULT_TZ = "BRT3";          // São Paulo: UTC-3
static const char *NTP_SERVER = "pool.ntp.org";

static bool s_synced = false;

static void time_sync_cb(struct timeval *tv)
{
    (void)tv;
    s_synced = true;
    ESP_LOGI(TAG, "Horario sincronizado via SNTP");
}

void timekeeper_init()
{
    setenv("TZ", DEFAULT_TZ, 1);
    tzset();

    ESP_LOGI(TAG, "Timezone configurado: %s", DEFAULT_TZ);
    ESP_LOGI(TAG, "Inicializando SNTP: %s", NTP_SERVER);

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    esp_sntp_set_time_sync_notification_cb(time_sync_cb);
    esp_sntp_init();
}

bool timekeeper_is_synced()
{
    time_t now = time(nullptr);

    // Qualquer data posterior a 2023 indica que o relógio já saiu do epoch inicial.
    if (now > 1700000000) {
        s_synced = true;
    }

    return s_synced;
}

void timekeeper_get_timestamp(char *buffer, size_t len)
{
    if (!buffer || len == 0) return;

    if (!timekeeper_is_synced()) {
        snprintf(buffer, len, "unsynced");
        return;
    }

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Formato ISO 8601 sem milissegundos.
    // Para São Paulo/BRT3, o sufixo padrão fica -03:00.
    strftime(buffer, len, "%Y-%m-%dT%H:%M:%S-03:00", &timeinfo);
}

int64_t timekeeper_get_epoch()
{
    if (!timekeeper_is_synced()) {
        return 0;
    }

    return (int64_t)time(nullptr);
}