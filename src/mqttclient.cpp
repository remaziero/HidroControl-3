#include "mqttclient.h"
#include "secrets.h"
#include "deviceid.h"
#include "timekeeper.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "esp_log.h"
#include "esp_event.h"
//#include "esp_app_desc.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t s_client = nullptr;
static bool s_connected = false;

static const char *TOPIC_TELEMETRY = "hidrocontrol/ac220/telemetry";

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            s_connected = true;
            ESP_LOGI(TAG, "MQTT conectado ao broker");
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "MQTT desconectado do broker");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT publicado com sucesso msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_ERROR:
            s_connected = false;
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");

            if (event->error_handle) {
                ESP_LOGE(TAG, "error_type=%d", event->error_handle->error_type);
                ESP_LOGE(TAG, "esp_tls_last_esp_err=0x%x",
                         event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "esp_tls_stack_err=0x%x",
                         event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "esp_transport_sock_errno=%d (%s)",
                         event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;

        default:
            ESP_LOGD(TAG, "Evento MQTT recebido: %ld", (long)event_id);
            break;
    }
}

void mqttclient_init()
{
    ESP_LOGI(TAG, "Inicializando MQTT...");

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_BROKER_URI;
    mqtt_cfg.session.keepalive = 60;
    mqtt_cfg.network.reconnect_timeout_ms = 5000;
    mqtt_cfg.network.timeout_ms = 10000;

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == nullptr) {
        ESP_LOGE(TAG, "Falha ao criar cliente MQTT");
        return;
    }

    esp_mqtt_client_register_event(
        s_client,
        MQTT_EVENT_ANY,
        mqtt_event_handler,
        nullptr
    );

    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar cliente MQTT: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "MQTT configurado para broker: %s", MQTT_BROKER_URI);
}

bool mqttclient_is_connected()
{
    return s_connected;
}

void mqttclient_publish_telemetry(const char* modo,
                                  float fluxo_frio,
                                  float fluxo_quente,
                                  bool rele_frio,
                                  bool rele_quente,
                                  bool wifi_connected)
{
    if (!s_client || !s_connected) {
        ESP_LOGW(TAG, "MQTT nao conectado. Publicacao ignorada.");
        return;
    }

    char timestamp[40] = {0};
    timekeeper_get_timestamp(timestamp, sizeof(timestamp));

    int64_t epoch = timekeeper_get_epoch();

    //const esp_app_desc_t* app = esp_app_get_description();
    //const char* fw = app ? app->version : "unknown";

    char payload[384];

    snprintf(payload, sizeof(payload),
             "{"
             "\"deviceId\":\"%s\","
             //"\"fw\":\"%s\","
             "\"timestamp\":\"%s\","
             "\"epoch\":%lld,"
             "\"modo\":\"%s\","
             "\"flowFrio\":%.2f,"
             "\"flowQuente\":%.2f,"
             "\"releFrio\":%d,"
             "\"releQuente\":%d,"
             "\"wifi\":%d,"
             "\"mqtt\":%d"
             "}",
             deviceid_get(),
             //fw,
             timestamp,
             (long long)epoch,
             modo,
             fluxo_frio,
             fluxo_quente,
             rele_frio ? 1 : 0,
             rele_quente ? 1 : 0,
             wifi_connected ? 1 : 0,
             s_connected ? 1 : 0);

    int msg_id = esp_mqtt_client_publish(
        s_client,
        TOPIC_TELEMETRY,
        payload,
        0,
        1,
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Falha ao publicar MQTT");
    } else {
        ESP_LOGI(TAG, "Publicacao MQTT solicitada msg_id=%d payload=%s",
                 msg_id,
                 payload);
    }
}