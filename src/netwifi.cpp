#include "netwifi.h"
#include "secrets.h"
#include "wificreds.h"
#include "deviceid.h"
#include "provisioning.h"

#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"

static const char *TAG = "NETWIFI";

static bool s_connected = false;
static char s_ip[16] = "0.0.0.0";

static constexpr int WIFI_MAX_ATTEMPTS = 10;
static int s_wifi_attempt = 0;
static bool s_connection_failed = false;

#ifndef WOKWI_SIM
static char s_wifi_ssid[33] = {0};
static char s_wifi_password[65] = {0};
static char s_ap_ssid[33] = {0};
#endif

static void event_handler(void *arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void *event_data) {
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        s_wifi_attempt = 1;
        s_connection_failed = false;

        ESP_LOGI(
            TAG,
            "Tentativa WiFi %d/%d",
            s_wifi_attempt,
            WIFI_MAX_ATTEMPTS
        );

        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        strcpy(s_ip, "0.0.0.0");

        if (s_wifi_attempt < WIFI_MAX_ATTEMPTS) {
            s_wifi_attempt++;

            ESP_LOGW(
                TAG,
                "Conexao WiFi falhou. Tentativa %d/%d",
                s_wifi_attempt,
                WIFI_MAX_ATTEMPTS
            );

            esp_wifi_connect();
        }
        else {
            s_connection_failed = true;

            ESP_LOGE(
                TAG,
                "Nao foi possivel conectar ao WiFi apos %d tentativas",
                WIFI_MAX_ATTEMPTS
            );
        }
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = (ip_event_got_ip_t *)event_data;

        snprintf(s_ip, sizeof(s_ip),
                 IPSTR,
                 IP2STR(&event->ip_info.ip));

        s_connected = true;
        s_connection_failed = false;
        s_wifi_attempt = 0;

        ESP_LOGI(TAG, "WiFi conectado. IP=%s", s_ip);
    }
}

void netwifi_init() {
    ESP_LOGI(TAG, "Inicializando WiFi...");

#ifndef WOKWI_SIM
    snprintf(
        s_ap_ssid,
        sizeof(s_ap_ssid),
        "HIDROCONTROL-%s",
        deviceid_chip()
    );

    ESP_LOGI(
        TAG,
        "SSID de provisionamento preparado: %s",
        s_ap_ssid
    );

    bool has_credentials = wificreds_load(
        s_wifi_ssid,
        sizeof(s_wifi_ssid),
        s_wifi_password,
        sizeof(s_wifi_password)
    );
#endif

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifndef WOKWI_SIM
    if (has_credentials) {
        esp_netif_create_default_wifi_sta();
        esp_netif_create_default_wifi_ap();
    }
    else {
        esp_netif_create_default_wifi_ap();
    }
#else
    esp_netif_create_default_wifi_sta();
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            nullptr,
            nullptr
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            nullptr,
            nullptr
        )
    );

#ifndef WOKWI_SIM

    if (has_credentials) {
        ESP_LOGI(TAG, "Credenciais WiFi carregadas do NVS");

        wifi_config_t wifi_config = {};

        strncpy(
            (char *)wifi_config.sta.ssid,
            s_wifi_ssid,
            sizeof(wifi_config.sta.ssid) - 1
        );

        strncpy(
            (char *)wifi_config.sta.password,
            s_wifi_password,
            sizeof(wifi_config.sta.password) - 1
        );

        wifi_config.sta.threshold.authmode =
            WIFI_AUTH_WPA2_PSK;

        wifi_config_t ap_config = {};

        strncpy(
            (char *)ap_config.ap.ssid,
            s_ap_ssid,
            sizeof(ap_config.ap.ssid) - 1
        );

        ap_config.ap.ssid_len =
            strlen(s_ap_ssid);

        ap_config.ap.channel = 1;
        ap_config.ap.max_connection = 4;
        ap_config.ap.authmode = WIFI_AUTH_OPEN;

        ESP_ERROR_CHECK(
            esp_wifi_set_mode(WIFI_MODE_APSTA)
        );

        ESP_ERROR_CHECK(
            esp_wifi_set_config(
                WIFI_IF_STA,
                &wifi_config
            )
        );

        ESP_ERROR_CHECK(
            esp_wifi_set_config(
                WIFI_IF_AP,
                &ap_config
            )
        );

        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_ERROR_CHECK(
            esp_wifi_set_ps(WIFI_PS_NONE)
        );

        ESP_LOGI(
            TAG,
            "WiFi STA configurado para SSID: %s",
            s_wifi_ssid
        );

        ESP_LOGI(
            TAG,
            "AP de manutencao iniciado: %s",
            s_ap_ssid
        );

        provisioning_start();
    }
    else {
        ESP_LOGW(
            TAG,
            "NVS sem credenciais. Iniciando modo de provisionamento"
        );

        wifi_config_t ap_config = {};

        strncpy(
            (char *)ap_config.ap.ssid,
            s_ap_ssid,
            sizeof(ap_config.ap.ssid) - 1
        );

        ap_config.ap.ssid_len =
            strlen(s_ap_ssid);

        ap_config.ap.channel = 1;
        ap_config.ap.max_connection = 4;
        ap_config.ap.authmode = WIFI_AUTH_OPEN;

        ESP_ERROR_CHECK(
            esp_wifi_set_mode(WIFI_MODE_AP)
        );

        ESP_ERROR_CHECK(
            esp_wifi_set_config(
                WIFI_IF_AP,
                &ap_config
            )
        );

        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(
            TAG,
            "AP de provisionamento iniciado: %s",
            s_ap_ssid
        );

        provisioning_start();
    }

#else

    wifi_config_t wifi_config = {};

    strncpy(
        (char *)wifi_config.sta.ssid,
        WIFI_STA_SSID,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        WIFI_STA_PASS,
        sizeof(wifi_config.sta.password) - 1
    );

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(
        TAG,
        "WiFi STA configurado para SSID: %s",
        WIFI_STA_SSID
    );

#endif
}

bool netwifi_is_connected() {
    return s_connected;
}

bool netwifi_connection_failed() {
    return s_connection_failed;
}

int netwifi_attempt() {
    return s_wifi_attempt;
}

int netwifi_max_attempts() {
    return WIFI_MAX_ATTEMPTS;
}

const char* netwifi_ip() {
    return s_ip;
}