#include "mqttclient.h"
#include "secrets.h"
#include "deviceid.h"
#include "timekeeper.h"
#include "modos.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t s_client = nullptr;
static bool s_connected = false;

static constexpr size_t TOPIC_BUFFER_SIZE = 128;
static constexpr size_t PAYLOAD_BUFFER_SIZE = 64;

// ---------------------------------------------------------------------
// Montagem dos topicos individuais por deviceId
// ---------------------------------------------------------------------

static void build_telemetry_topic(char *buffer, size_t len)
{
    if (buffer == nullptr || len == 0) {
        return;
    }

    snprintf(
        buffer,
        len,
        "hidrocontrol/%s/telemetry",
        deviceid_get()
    );
}

static void build_modo_topic(char *buffer, size_t len)
{
    if (buffer == nullptr || len == 0) {
        return;
    }

    snprintf(
        buffer,
        len,
        "hidrocontrol/%s/modo",
        deviceid_get()
    );
}

// ---------------------------------------------------------------------
// Processamento do comando de modo recebido pelo MQTT
// ---------------------------------------------------------------------

static void processar_comando_modo(const char *payload)
{
    if (payload == nullptr) {
        ESP_LOGW(TAG, "Payload de modo nulo");
        return;
    }

    if (strcasecmp(payload, "NORMAL") == 0 ||
        strcmp(payload, "0") == 0) {

        modos_set(ModoOperacao::NORMAL);
        ESP_LOGI(TAG, "Modo alterado remotamente para NORMAL");
    }
    else if (strcasecmp(payload, "DUO") == 0 ||
             strcmp(payload, "1") == 0) {

        modos_set(ModoOperacao::DUO);
        ESP_LOGI(TAG, "Modo alterado remotamente para DUO");
    }
    else if (strcasecmp(payload, "MIX") == 0 ||
             strcmp(payload, "2") == 0) {

        modos_set(ModoOperacao::MIX);
        ESP_LOGI(TAG, "Modo alterado remotamente para MIX");
    }
    else {
        ESP_LOGW(
            TAG,
            "Modo MQTT invalido recebido: '%s'",
            payload
        );
    }
}

// ---------------------------------------------------------------------
// Processamento de mensagem MQTT recebida
// ---------------------------------------------------------------------

static void processar_mensagem_recebida(
    esp_mqtt_event_handle_t event)
{
    if (event == nullptr) {
        ESP_LOGW(TAG, "Evento MQTT DATA nulo");
        return;
    }

    if (event->topic == nullptr ||
        event->data == nullptr) {

        ESP_LOGW(TAG, "Mensagem MQTT sem topico ou payload");
        return;
    }

    char topic[TOPIC_BUFFER_SIZE] = {0};
    char payload[PAYLOAD_BUFFER_SIZE] = {0};

    size_t topic_len =
        static_cast<size_t>(event->topic_len);

    size_t payload_len =
        static_cast<size_t>(event->data_len);

    if (topic_len >= sizeof(topic)) {
        ESP_LOGW(
            TAG,
            "Topico recebido excede o limite de %u bytes",
            static_cast<unsigned>(sizeof(topic) - 1)
        );

        topic_len = sizeof(topic) - 1;
    }

    if (payload_len >= sizeof(payload)) {
        ESP_LOGW(
            TAG,
            "Payload recebido excede o limite de %u bytes",
            static_cast<unsigned>(sizeof(payload) - 1)
        );

        payload_len = sizeof(payload) - 1;
    }

    memcpy(topic, event->topic, topic_len);
    topic[topic_len] = '\0';

    memcpy(payload, event->data, payload_len);
    payload[payload_len] = '\0';

    ESP_LOGI(
        TAG,
        "Mensagem recebida: topico='%s' payload='%s'",
        topic,
        payload
    );

    char topic_modo[TOPIC_BUFFER_SIZE] = {0};
    build_modo_topic(topic_modo, sizeof(topic_modo));

    if (strcmp(topic, topic_modo) == 0) {
        processar_comando_modo(payload);
    }
    else {
        ESP_LOGW(
            TAG,
            "Mensagem recebida em topico nao tratado: %s",
            topic
        );
    }
}

// ---------------------------------------------------------------------
// Handler central de eventos do ESP-MQTT
// ---------------------------------------------------------------------

static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    (void)handler_args;
    (void)base;

    auto event =
        static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (
        static_cast<esp_mqtt_event_id_t>(event_id)
    ) {
        case MQTT_EVENT_CONNECTED: {
            s_connected = true;

            ESP_LOGI(TAG, "MQTT conectado ao broker");

            char topic_modo[TOPIC_BUFFER_SIZE] = {0};

            build_modo_topic(
                topic_modo,
                sizeof(topic_modo)
            );

            int msg_id = esp_mqtt_client_subscribe(
                s_client,
                topic_modo,
                1
            );

            if (msg_id < 0) {
                ESP_LOGE(
                    TAG,
                    "Falha ao solicitar subscribe em: %s",
                    topic_modo
                );
            }
            else {
                ESP_LOGI(
                    TAG,
                    "Subscribe solicitado: topico=%s msg_id=%d",
                    topic_modo,
                    msg_id
                );
            }

            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;

            ESP_LOGW(TAG, "MQTT desconectado do broker");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(
                TAG,
                "Subscribe confirmado pelo broker msg_id=%d",
                event != nullptr ? event->msg_id : -1
            );
            break;

        case MQTT_EVENT_DATA:
            processar_mensagem_recebida(event);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(
                TAG,
                "MQTT publicado com sucesso msg_id=%d",
                event != nullptr ? event->msg_id : -1
            );
            break;

        case MQTT_EVENT_ERROR:
            s_connected = false;

            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");

            if (event != nullptr &&
                event->error_handle != nullptr) {

                ESP_LOGE(
                    TAG,
                    "error_type=%d",
                    event->error_handle->error_type
                );

                ESP_LOGE(
                    TAG,
                    "esp_tls_last_esp_err=0x%x",
                    event->error_handle->esp_tls_last_esp_err
                );

                ESP_LOGE(
                    TAG,
                    "esp_tls_stack_err=0x%x",
                    event->error_handle->esp_tls_stack_err
                );

                int socket_errno =
                    event->error_handle->esp_transport_sock_errno;

                ESP_LOGE(
                    TAG,
                    "esp_transport_sock_errno=%d (%s)",
                    socket_errno,
                    strerror(socket_errno)
                );
            }

            break;

        default:
            ESP_LOGD(
                TAG,
                "Evento MQTT recebido: %ld",
                static_cast<long>(event_id)
            );
            break;
    }
}

// ---------------------------------------------------------------------
// Inicializacao do cliente MQTT
// ---------------------------------------------------------------------

void mqttclient_init()
{
    ESP_LOGI(TAG, "Inicializando MQTT...");

    esp_mqtt_client_config_t mqtt_cfg = {};

    mqtt_cfg.broker.address.uri =
        MQTT_BROKER_URI;

    mqtt_cfg.broker.verification.crt_bundle_attach =
        esp_crt_bundle_attach;

    mqtt_cfg.credentials.client_id =
        deviceid_get();

    mqtt_cfg.credentials.username =
        MQTT_USERNAME;

    mqtt_cfg.credentials.authentication.password =
        MQTT_PASSWORD;

    mqtt_cfg.session.keepalive = 60;

    mqtt_cfg.network.reconnect_timeout_ms =
        5000;

    mqtt_cfg.network.timeout_ms =
        10000;

    ESP_LOGI(TAG, "Broker MQTT: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "Usuario MQTT: %s", MQTT_USERNAME);
    ESP_LOGI(TAG, "ClientId MQTT: %s", deviceid_get());

    s_client =
        esp_mqtt_client_init(&mqtt_cfg);

    if (s_client == nullptr) {
        ESP_LOGE(TAG, "Falha ao criar cliente MQTT");
        return;
    }

    esp_err_t err =
        esp_mqtt_client_register_event(
            s_client,
            MQTT_EVENT_ANY,
            mqtt_event_handler,
            nullptr
        );

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Falha ao registrar eventos MQTT: %s",
            esp_err_to_name(err)
        );

        esp_mqtt_client_destroy(s_client);
        s_client = nullptr;
        return;
    }

    err = esp_mqtt_client_start(s_client);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Falha ao iniciar cliente MQTT: %s",
            esp_err_to_name(err)
        );

        esp_mqtt_client_destroy(s_client);
        s_client = nullptr;
        return;
    }

    ESP_LOGI(
        TAG,
        "Cliente MQTT iniciado com TLS para broker: %s",
        MQTT_BROKER_URI
    );
}

// ---------------------------------------------------------------------
// Consulta do estado da conexao
// ---------------------------------------------------------------------

bool mqttclient_is_connected()
{
    return s_connected;
}

// ---------------------------------------------------------------------
// Publicacao periodica da telemetria
// ---------------------------------------------------------------------

void mqttclient_publish_telemetry(
    const char *modo,
    float fluxo_frio,
    float fluxo_quente,
    bool rele_frio,
    bool rele_quente,
    bool wifi_connected)
{
    if (s_client == nullptr || !s_connected) {
        ESP_LOGW(
            TAG,
            "MQTT nao conectado. Publicacao ignorada."
        );
        return;
    }

    char timestamp[40] = {0};

    timekeeper_get_timestamp(
        timestamp,
        sizeof(timestamp)
    );

    int64_t epoch =
        timekeeper_get_epoch();

    char payload[384] = {0};

    int written = snprintf(
        payload,
        sizeof(payload),
        "{"
        "\"deviceId\":\"%s\","
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
        timestamp,
        static_cast<long long>(epoch),
        modo,
        fluxo_frio,
        fluxo_quente,
        rele_frio ? 1 : 0,
        rele_quente ? 1 : 0,
        wifi_connected ? 1 : 0,
        s_connected ? 1 : 0
    );

    if (written < 0) {
        ESP_LOGE(TAG, "Erro ao montar payload MQTT");
        return;
    }

    if (
        static_cast<size_t>(written) >=
        sizeof(payload)
    ) {
        ESP_LOGE(
            TAG,
            "Payload MQTT excedeu o buffer de %u bytes",
            static_cast<unsigned>(sizeof(payload))
        );
        return;
    }

    char topic_telemetry[TOPIC_BUFFER_SIZE] = {0};

    build_telemetry_topic(
        topic_telemetry,
        sizeof(topic_telemetry)
    );

    int msg_id = esp_mqtt_client_publish(
        s_client,
        topic_telemetry,
        payload,
        0,
        1,
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(
            TAG,
            "Falha ao publicar MQTT no topico: %s",
            topic_telemetry
        );
    }
    else {
        ESP_LOGI(
            TAG,
            "Publicacao MQTT solicitada: topico=%s msg_id=%d payload=%s",
            topic_telemetry,
            msg_id,
            payload
        );
    }
}