#pragma once

#include <stdint.h>
#include <stddef.h>

void timekeeper_init();

bool timekeeper_is_synced();

void timekeeper_get_timestamp(char *buffer, size_t len);

int64_t timekeeper_get_epoch();