#pragma once

#include <stddef.h>

bool wificreds_exists();

bool wificreds_load(
    char *ssid,
    size_t ssid_len,
    char *password,
    size_t password_len
);

bool wificreds_save(
    const char *ssid,
    const char *password
);

bool wificreds_clear();
