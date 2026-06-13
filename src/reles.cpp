#include "reles.h"
#include "hardware.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "RELES";

static bool s_frio = false;
static bool s_quente = false;

static int level_on() {
    return RELAY_ACTIVE_HIGH ? 1 : 0;
}

static int level_off() {
    return RELAY_ACTIVE_HIGH ? 0 : 1;
}

void reles_init() {
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
        (1ULL << PIN_RELE_FRIO) |
        (1ULL << PIN_RELE_QUENTE);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);

    gpio_set_level(PIN_RELE_FRIO, level_off());
    gpio_set_level(PIN_RELE_QUENTE, level_off());

    s_frio = false;
    s_quente = false;

    ESP_LOGI(TAG, "Reles inicializados DESLIGADOS");
}

void reles_set_frio(bool ligar) {
    if (ligar == s_frio) return;

    s_frio = ligar;
    gpio_set_level(PIN_RELE_FRIO, ligar ? level_on() : level_off());

    ESP_LOGI(TAG, "RELE FRIO: %s", ligar ? "LIGADO" : "DESLIGADO");
}

void reles_set_quente(bool ligar) {
    if (ligar == s_quente) return;

    s_quente = ligar;
    gpio_set_level(PIN_RELE_QUENTE, ligar ? level_on() : level_off());

    ESP_LOGI(TAG, "RELE QUENTE: %s", ligar ? "LIGADO" : "DESLIGADO");
}

bool reles_frio_ligado() {
    return s_frio;
}

bool reles_quente_ligado() {
    return s_quente;
}