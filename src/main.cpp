#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "hardware.h"
#include "reles.h"
#include "fluxo.h"
#include "modos.h"
#include "oled.h"
#include "deviceid.h"

#include "netwifi.h"
#include "mqttclient.h"

#include "timekeeper.h"

static const char *TAG = "MAIN";

static uint32_t millis32() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static bool hold_off_active(uint32_t last_stop_ms, uint32_t now_ms) {
    if (last_stop_ms == 0) return false;
    return (uint32_t)(now_ms - last_stop_ms) < DELAY_OFF_MS;
}
/*
static void task_hidrocontrol(void *pv) {
    (void)pv;

    ESP_LOGI(TAG, "Task HidroControl iniciada");
    uint32_t last_mqtt_pub = 0;

    while (true) {
        uint32_t now = millis32();

        // Atualiza botão físico de modo: NORMAL -> DUO -> MIX -> NORMAL
        modos_update_button();

        // Atualiza leitura dos sensores de fluxo
        fluxo_update();

        bool frio_ativo   = fluxo_frio_ativo();
        bool quente_ativo = fluxo_quente_ativo();

        bool demanda_frio   = false;
        bool demanda_quente = false;

        bool rele_frio   = false;
        bool rele_quente = false;

        ModoOperacao modo = modos_get();

        // Proteção inicial: durante STARTUP_INHIBIT_MS os relés permanecem desligados
        if (now > STARTUP_INHIBIT_MS) {
            demanda_frio =
                frio_ativo ||
                hold_off_active(fluxo_last_stop_frio_ms(), now);

            demanda_quente =
                quente_ativo ||
                hold_off_active(fluxo_last_stop_quente_ms(), now);

            switch (modo) {
                case ModoOperacao::NORMAL:
                    // NORMAL:
                    // Fluxo frio   -> relé frio
                    // Fluxo quente -> relé quente
                    rele_frio   = demanda_frio;
                    rele_quente = demanda_quente;
                    break;

                case ModoOperacao::DUO:
                    // DUO:
                    // Qualquer fluxo aciona os dois relés
                    rele_frio   = demanda_frio || demanda_quente;
                    rele_quente = demanda_frio || demanda_quente;
                    break;

                case ModoOperacao::MIX:
                    // MIX:
                    // Frio aciona só frio
                    // Quente aciona frio + quente
                    rele_frio   = demanda_frio || demanda_quente;
                    rele_quente = demanda_quente;
                    break;
            }
        }

        reles_set_frio(rele_frio);
        reles_set_quente(rele_quente);

        static uint32_t last_log = 0;
        if ((uint32_t)(now - last_log) >= 1000) {
            last_log = now;

            ESP_LOGI(TAG,
                     "MODO=%s | FRIO: pulsos=%lu vazao=%.2f L/min ativo=%d rele=%d | QUENTE: pulsos=%lu vazao=%.2f L/min ativo=%d rele=%d",
                     modos_nome(modo),
                     (unsigned long)fluxo_pulsos_frio(),
                     fluxo_frio_lmin(),
                     frio_ativo,
                     reles_frio_ligado(),
                     (unsigned long)fluxo_pulsos_quente(),
                     fluxo_quente_lmin(),
                     quente_ativo,
                     reles_quente_ligado());

            // Estados provisórios:
            // WiFi e MQTT ainda não foram implementados nesta etapa.
            // Por enquanto aparecem como desconectados no OLED.
            oled_show_status(
                netwifi_is_connected(),
                mqttclient_is_connected(),
                modos_nome(modo),
                fluxo_frio_lmin(),
                fluxo_quente_lmin()
            );
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
*/
static void task_hidrocontrol(void *pv) {
    (void)pv;

    ESP_LOGI(TAG, "Task HidroControl iniciada");

    uint32_t last_mqtt_pub = 0;

    while (true) {
        uint32_t now = millis32();

        modos_update_button();
        fluxo_update();

        bool frio_ativo   = fluxo_frio_ativo();
        bool quente_ativo = fluxo_quente_ativo();

        bool demanda_frio   = false;
        bool demanda_quente = false;

        bool rele_frio   = false;
        bool rele_quente = false;

        ModoOperacao modo = modos_get();

        if (now > STARTUP_INHIBIT_MS) {
            demanda_frio =
                frio_ativo ||
                hold_off_active(fluxo_last_stop_frio_ms(), now);

            demanda_quente =
                quente_ativo ||
                hold_off_active(fluxo_last_stop_quente_ms(), now);

            switch (modo) {
                case ModoOperacao::NORMAL:
                    rele_frio   = demanda_frio;
                    rele_quente = demanda_quente;
                    break;

                case ModoOperacao::DUO:
                    rele_frio   = demanda_frio || demanda_quente;
                    rele_quente = demanda_frio || demanda_quente;
                    break;

                case ModoOperacao::MIX:
                    rele_frio   = demanda_frio || demanda_quente;
                    rele_quente = demanda_quente;
                    break;
            }
        }

        reles_set_frio(rele_frio);
        reles_set_quente(rele_quente);

        static uint32_t last_log = 0;
        if ((uint32_t)(now - last_log) >= 1000) {
            last_log = now;

            ESP_LOGI(TAG,
                     "MODO=%s | FRIO: pulsos=%lu vazao=%.2f L/min ativo=%d rele=%d | QUENTE: pulsos=%lu vazao=%.2f L/min ativo=%d rele=%d",
                     modos_nome(modo),
                     (unsigned long)fluxo_pulsos_frio(),
                     fluxo_frio_lmin(),
                     frio_ativo,
                     reles_frio_ligado(),
                     (unsigned long)fluxo_pulsos_quente(),
                     fluxo_quente_lmin(),
                     quente_ativo,
                     reles_quente_ligado());

            oled_show_status(
                netwifi_is_connected(),
                mqttclient_is_connected(),
                modos_nome(modo),
                fluxo_frio_lmin(),
                fluxo_quente_lmin()
            );
        }

        if ((uint32_t)(now - last_mqtt_pub) >= 5000) {
            last_mqtt_pub = now;

            mqttclient_publish_telemetry(
                modos_nome(modo),
                fluxo_frio_lmin(),
                fluxo_quente_lmin(),
                reles_frio_ligado(),
                reles_quente_ligado(),
                netwifi_is_connected()
            );
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


static void task_blink(void *pv) {
    (void)pv;

    bool led = false;

    gpio_reset_pin(PIN_LED_STATUS);
    gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);

    while (true) {
        led = !led;
        gpio_set_level(PIN_LED_STATUS, led);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
/*
extern "C" void app_main(void) {
        esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "HidroControl-3 V0.4 - OLED de status");
    ESP_LOGI(TAG, "AC220 / NodeMCU-32S board profile");
    ESP_LOGI(TAG, "====================================");

    // Inicialização de hardware e módulos
    nvs_flash_init();
    reles_init();
    fluxo_init();
    modos_init();
    oled_init();
    netwifi_init();
    mqttclient_init();

    xTaskCreate(
        task_hidrocontrol,
        "task_hidrocontrol",
        4096,
        nullptr,
        5,
        nullptr
    );

    xTaskCreate(
        task_blink,
        "task_blink",
        2048,
        nullptr,
        1,
        nullptr
    );
}
    */
   extern "C" void app_main(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "HidroControl-3 V1.0.3 - OLED com WiFi/MQTT reais");
    ESP_LOGI(TAG, "AC220 / NodeMCU-32S board profile");
    ESP_LOGI(TAG, "====================================");
/*
    reles_init();
    fluxo_init();
    modos_init();
    oled_init();
    netwifi_init();
    mqttclient_init();
*/
reles_init();
fluxo_init();
modos_init();
deviceid_init();
oled_init();
netwifi_init();

ESP_LOGI(TAG, "Aguardando WiFi conectar antes de iniciar MQTT...");

while (!netwifi_is_connected()) {
    vTaskDelay(pdMS_TO_TICKS(500));
}

ESP_LOGI(TAG, "WiFi conectado. Iniciando timekeeper e MQTT.");

timekeeper_init();
mqttclient_init();

    xTaskCreate(
        task_hidrocontrol,
        "task_hidrocontrol",
        4096,
        nullptr,
        5,
        nullptr
    );

    xTaskCreate(
        task_blink,
        "task_blink",
        2048,
        nullptr,
        1,
        nullptr
    );
}