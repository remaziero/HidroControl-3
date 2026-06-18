#include "modos.h"
#include "hardware.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "MODOS";

static ModoOperacao s_modo = ModoOperacao::NORMAL;

static bool s_btnStable = true;   // HIGH solto
static bool s_btnLastRaw = true;  // HIGH solto
static uint32_t s_lastDebounceMs = 0;

static uint32_t millis32() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void modos_init() {
    s_modo = ModoOperacao::NORMAL;

    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_MODE_BUTTON);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);

    s_btnStable = true;
    s_btnLastRaw = true;
    s_lastDebounceMs = millis32();

    ESP_LOGI(TAG, "Modo inicial: NORMAL");
    ESP_LOGI(TAG, "Botao de modo inicializado em GPIO%d", PIN_MODE_BUTTON);
}

void modos_set(ModoOperacao modo) {
    if (modo == s_modo) return;

    s_modo = modo;
    ESP_LOGI(TAG, "Modo alterado para: %s", modos_nome(s_modo));
}

void modos_next() {
    switch (s_modo) {
        case ModoOperacao::NORMAL:
            modos_set(ModoOperacao::DUO);
            break;

        case ModoOperacao::DUO:
            modos_set(ModoOperacao::MIX);
            break;

        case ModoOperacao::MIX:
        default:
            modos_set(ModoOperacao::NORMAL);
            break;
    }
}

void modos_update_button() {
    uint32_t now = millis32();

    bool btnRaw = gpio_get_level(PIN_MODE_BUTTON); // HIGH solto, LOW pressionado

    if (btnRaw != s_btnLastRaw) {
        s_btnLastRaw = btnRaw;
        s_lastDebounceMs = now;
    }

    if ((uint32_t)(now - s_lastDebounceMs) > MODE_BUTTON_DEBOUNCE_MS) {
        if (s_btnStable != s_btnLastRaw) {
            bool oldStable = s_btnStable;
            s_btnStable = s_btnLastRaw;

            // Borda de descida: HIGH -> LOW
            if (oldStable == true && s_btnStable == false) {
                ESP_LOGI(TAG, "Botao pressionado: alternando modo");
                modos_next();
            }
        }
    }
}

ModoOperacao modos_get() {
    return s_modo;
}

const char* modos_nome(ModoOperacao modo) {
    switch (modo) {
        case ModoOperacao::NORMAL: return "NORMAL";
        case ModoOperacao::DUO:    return "DUO";
        case ModoOperacao::MIX:    return "MIX";
        default:                   return "DESCONHECIDO";
    }
}