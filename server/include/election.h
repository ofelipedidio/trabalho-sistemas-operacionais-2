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

void initiateElection(metadata_t& metadata);

void receiveElectionMessage(metadata_t& metadata, const server_t& senderServer);

void receiveElectedMessage(metadata_t& metadata, const server_t& electedServer);

server_t getNextServer(const metadata_t& metadata, const server_t& currentServer);

void sendElectionMessage(const server_t& targetServer, const server_t& winningServer);

void sendElectedMessage(const server_t& targetServer, const server_t& electedServer);