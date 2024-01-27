#pragma once

#include <cinttypes>
#include <vector>
#include <iostream>

typedef enum {
    primary,
    backup,
} server_type_t;

typedef struct {
    uint32_t ip;
    uint16_t port;
    server_type_t server_type;
} server_t;

typedef struct {
    std::vector<server_t> servers;
} metadata_t;

void initiateElection();

server_t getNextServer(const server_t currentServer);

void sendElectionMessage(server_t targetServer, server_t winningServer);

void sendElectedMessage(server_t targetServer, server_t electedServer);

void handle_async_election_message(server_t winningServer);

void handle_async_elected_message(server_t electedServer);

void setElected();

bool updateElected(server_t electedServer);

void printServer(std::ostream &stream, const server_t server);

bool el_start_thread();

inline std::ostream &operator<<(std::ostream &stream, const server_t &server){
    return stream << std::hex << server.ip << std::dec << ':' << server.port << "[" << (server.server_type == primary ? "p" : "b") << "]";
}

