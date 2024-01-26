#pragma once

#include <cinttypes>
#include <vector>

typedef enum {
    primary,
    secondary,
} server_type_t;

typedef struct {
    uint32_t ip;
    uint16_t port;
    server_type_t server_type;
} server_t;

typedef struct {
    std::vector<server_t> servers;
} metadata_t;
