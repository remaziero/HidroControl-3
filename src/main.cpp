#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "hardware.h"
#include "reles.h"
#include "fluxo.h"

static const char *TAG = "MAIN";

static uint32_t millis32() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static bool hold_off_active(uint32_t last_stop_ms, uint32_t now_ms) {
    if (last_stop_ms == 0) return false;
    return (uint32_t)(now_ms - last_stop_ms) < DELAY_OFF_MS;
}

static void task_hidrocontrol(void *pv) {
    (void)pv;

    ESP_LOGI(TAG, "Task HidroControl iniciada");

    while (true) {
        uint32_t now = millis32();

        fluxo_update();

        bool frio_ativo = fluxo_frio_ativo();
        bool quente_ativo = fluxo_quente_ativo();

        bool rele_frio = false;
        bool rele_quente = false;

        if (now > STARTUP_INHIBIT_MS) {
            rele_frio =
                frio_ativo ||
                hold_off_active(fluxo_last_stop_frio_ms(), now);

            rele_quente =
                quente_ativo ||
                hold_off_active(fluxo_last_stop_quente_ms(), now);
        }

        reles_set_frio(rele_frio);
        reles_set_quente(rele_quente);

        static uint32_t last_log = 0;
        if ((uint32_t)(now - last_log) >= 1000) {
            last_log = now;

            ESP_LOGI(TAG,
                     "FRIO: pulsos=%lu vazao=%.2f L/min ativo=%d rele=%d | QUENTE: pulsos=%lu vazao=%.2f L/min ativo=%d rele=%d",
                     (unsigned long)fluxo_pulsos_frio(),
                     fluxo_frio_lmin(),
                     frio_ativo,
                     reles_frio_ligado(),
                     (unsigned long)fluxo_pulsos_quente(),
                     fluxo_quente_lmin(),
                     quente_ativo,
                     reles_quente_ligado());
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void task_blink(void *pv)
{
    bool led = false;
    gpio_reset_pin(PIN_LED_STATUS);
    gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);

    while (true)
    {
        led = !led;

        gpio_set_level(PIN_LED_STATUS, led);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "HidroControl-3 V0.1 - ESP-IDF C++");
    ESP_LOGI(TAG, "AC220 / NodeMCU-32S board profile");
    ESP_LOGI(TAG, "====================================");

    reles_init();
    fluxo_init();

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