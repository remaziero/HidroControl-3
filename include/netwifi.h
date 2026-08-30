#pragma once

void netwifi_init();

bool netwifi_is_connected();
bool netwifi_connection_failed();
int netwifi_attempt();
int netwifi_max_attempts();
const char* netwifi_ip();
