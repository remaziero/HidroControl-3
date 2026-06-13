#pragma once

#include "driver/gpio.h"

// HidroControl-3 V0.1 - AC220 / ESP32-WROOM-32E

#define PIN_RELE_FRIO      GPIO_NUM_16
#define PIN_RELE_QUENTE    GPIO_NUM_17

#define PIN_FLUXO_FRIO     GPIO_NUM_25
#define PIN_FLUXO_QUENTE   GPIO_NUM_26

#define RELAY_ACTIVE_HIGH  true

#define DELAY_OFF_MS       5000
#define STARTUP_INHIBIT_MS 3000

#define FLOW_K_FACTOR      6.6f   // F = 6,6 x Q  => Q = F / 6,6