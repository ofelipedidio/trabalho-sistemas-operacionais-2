#include "../include/state.h"
#include <semaphore.h>

program_state_t state;

void state_init(uint32_t ip, uint16_t port) {
    state.primary_ip = ip;
    state.primary_port = port;
    sem_init(&state.logging_mutex, 0, 1);
}

void acquire_logging_mutex() {
    sem_wait(&state.logging_mutex);
}

void release_logging_mutex() {
    sem_post(&state.logging_mutex);
}

void get_primary_server(uint32_t *ip, uint16_t *port) {
    *ip = state.primary_ip;
    *port = state.primary_port;
}

void set_primary_server(uint32_t ip, uint16_t port) {
    state.primary_ip = ip;
    state.primary_port = port;
}

