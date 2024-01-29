#pragma once

/******************************************\
* This module contains the code related to *
* the communication between servers.       *
\******************************************/
#include <cstdint>

void *dns_thread_init(void *args);

bool dns_thread_init(uint16_t *port);

