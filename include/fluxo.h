#pragma once

#include <stdint.h>

void fluxo_init();

void fluxo_update();

float fluxo_frio_lmin();
float fluxo_quente_lmin();

bool fluxo_frio_ativo();
bool fluxo_quente_ativo();

uint32_t fluxo_pulsos_frio();
uint32_t fluxo_pulsos_quente();

uint32_t fluxo_last_stop_frio_ms();
uint32_t fluxo_last_stop_quente_ms();