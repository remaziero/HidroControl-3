#include "fluxo.h"
#include "hardware.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "FLUXO";

#include <atomic>

static std::atomic<uint32_t> s_pulsos_frio{0};
static std::atomic<uint32_t> s_pulsos_quente{0};

static uint32_t s_last_pulsos_frio = 0;
static uint32_t s_last_pulsos_quente = 0;

static float s_frio_lmin = 0.0f;
static float s_quente_lmin = 0.0f;

static bool s_frio_ativo = false;
static bool s_quente_ativo = false;

static uint32_t s_last_stop_frio_ms = 0;
static uint32_t s_last_stop_quente_ms = 0;

static int64_t s_last_update_us = 0;

static uint32_t millis32() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void IRAM_ATTR isr_fluxo_frio(void *arg) {
    (void)arg;
    s_pulsos_frio.fetch_add(1, std::memory_order_relaxed);
}

static void IRAM_ATTR isr_fluxo_quente(void *arg) {
    (void)arg;
    s_pulsos_quente.fetch_add(1, std::memory_order_relaxed);
}

void fluxo_init() {
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask =
        (1ULL << PIN_FLUXO_FRIO) |
        (1ULL << PIN_FLUXO_QUENTE);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;

    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_FLUXO_FRIO, isr_fluxo_frio, nullptr);
    gpio_isr_handler_add(PIN_FLUXO_QUENTE, isr_fluxo_quente, nullptr);

    s_last_update_us = esp_timer_get_time();

    ESP_LOGI(TAG, "Sensores de fluxo inicializados");
}

void fluxo_update() {
    int64_t now_us = esp_timer_get_time();
    int64_t dt_us = now_us - s_last_update_us;

    if (dt_us < 500000) {
        return;
    }

    s_last_update_us = now_us;

    uint32_t p_frio = s_pulsos_frio.load(std::memory_order_relaxed);
    uint32_t p_quente = s_pulsos_quente.load(std::memory_order_relaxed);

    uint32_t d_frio = p_frio - s_last_pulsos_frio;
    uint32_t d_quente = p_quente - s_last_pulsos_quente;

    s_last_pulsos_frio = p_frio;
    s_last_pulsos_quente = p_quente;

    float dt_s = (float)dt_us / 1000000.0f;

    float hz_frio = (float)d_frio / dt_s;
    float hz_quente = (float)d_quente / dt_s;

    s_frio_lmin = hz_frio / FLOW_K_FACTOR;
    s_quente_lmin = hz_quente / FLOW_K_FACTOR;

    bool novo_frio_ativo = s_frio_lmin > 0.10f;
    bool novo_quente_ativo = s_quente_lmin > 0.10f;

    uint32_t now_ms = millis32();

    if (s_frio_ativo && !novo_frio_ativo) {
        s_last_stop_frio_ms = now_ms;
    }

    if (s_quente_ativo && !novo_quente_ativo) {
        s_last_stop_quente_ms = now_ms;
    }

    s_frio_ativo = novo_frio_ativo;
    s_quente_ativo = novo_quente_ativo;
}

float fluxo_frio_lmin() {
    return s_frio_lmin;
}

float fluxo_quente_lmin() {
    return s_quente_lmin;
}

bool fluxo_frio_ativo() {
    return s_frio_ativo;
}

bool fluxo_quente_ativo() {
    return s_quente_ativo;
}

uint32_t fluxo_pulsos_frio() {
    return s_pulsos_frio.load(std::memory_order_relaxed);
}

uint32_t fluxo_pulsos_quente() {
    return s_pulsos_quente.load(std::memory_order_relaxed);
}

uint32_t fluxo_last_stop_frio_ms() {
    return s_last_stop_frio_ms;
}

uint32_t fluxo_last_stop_quente_ms() {
    return s_last_stop_quente_ms;
}