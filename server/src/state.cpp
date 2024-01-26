#include "../include/state.h"
#include <semaphore.h>
#include <vector>

program_state_t state;

void state_init(uint32_t ip, uint16_t port, server_type_t type) {
    state.current_server = {
        .ip = ip,
        .port = port,
        .server_type = type
    };

    sem_init(&state.coms_server_mutex, 0, 1);
}

coms_server_t *acquire_coms_server() {
    sem_wait(&state.coms_server_mutex);
    return &state.coms_server;
}

void release_coms_server(coms_server_t *coms_server) {
    sem_post(&state.coms_server_mutex);
}

void set_coms_server(coms_server_t *coms_server) {
    state.coms_server = *coms_server;
}

server_t *get_current_server() {
    return &state.current_server;
}

