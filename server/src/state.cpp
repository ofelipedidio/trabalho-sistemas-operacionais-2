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

    state.metadata.servers.push_back(state.current_server);

    state.should_stop = false;

    sem_init(&state.metadata_mutex, 0, 1);
    sem_init(&state.logging_mutex, 0, 1);
    sem_init(&state.heartbeatSocket_mutex, 0, 1);
    sem_init(&state.heartbeatvector_mutex, 0, 1);
}

/*************\
* Should stop *
\*************/
bool should_stop() {
    return state.should_stop;
}

/****************\
* Current server *
\****************/
server_t *get_current_server() {
    return &state.current_server;
}

server_t *get_primary_server() {
    metadata_t *metadata = acquire_metadata();
    for (server_t server : metadata->servers) {
        state.primary_server = server;
        release_metadata();
        return &state.primary_server;
    }
    release_metadata();
    return NULL;
}

/**********\
* Metadata *
\**********/
metadata_t *acquire_metadata() {
    std::cerr << "[DEBUG] [State] Acquiring metadata" << std::endl;
    sem_wait(&state.metadata_mutex);
    std::cerr << "[DEBUG] [State] Metadata acquired" << std::endl;
    return &state.metadata;
}

void release_metadata() {
    std::cerr << "(metadata) [ ";
    for (auto server : state.metadata.servers) {
        printServer(std::cerr, server);
        std::cerr << ", ";
    }
    std::cerr << "]" << std::endl;

    sem_post(&state.metadata_mutex);
}


void acquire_logging_mutex() {
    sem_wait(&state.logging_mutex);
}

void release_logging_mutex() {
    sem_post(&state.logging_mutex);
}

/***********\
* Heartbeat *
\***********/

connection_t** get_heartbeat_socket(){
    sem_wait(&state.heartbeatSocket_mutex);
    return &state.heartbeatSocket;
}

void set_heartbeat_socket(connection_t *heartbeatconnection){
    connection_t **new_socket = get_heartbeat_socket();
    (*new_socket) = heartbeatconnection;
    release_heartbeat_socket();
}

void release_heartbeat_socket(){
    sem_post(&state.heartbeatSocket_mutex);
}

std::vector <connection_t*>* get_heartbeat_connections(){
    sem_wait(&state.heartbeatvector_mutex);
    return &state.heartbeatconnections;
}

void release_heartbeat_connections(){
    sem_post(&state.heartbeatvector_mutex);
}