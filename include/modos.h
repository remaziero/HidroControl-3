#pragma once

enum class ModoOperacao {
    NORMAL = 0,
    DUO    = 1,
    MIX    = 2
};

void modos_init();

void modos_set(ModoOperacao modo);
void modos_next();
void modos_update_button();

ModoOperacao modos_get();

const char* modos_nome(ModoOperacao modo);