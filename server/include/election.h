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

metadata_t GetMetadata(); //TODO

void initiateElection(); //TODO

void setElected(metadata_t& metadata, server_t& electedServer);

void updateElected(server_t& currentServer, server_t& electedServer); //TODO

void receiveElectionMessage(metadata_t& metadata, server_t& currentServer, server_t& senderServer);

void receiveElectedMessage(metadata_t& metadata, server_t& currentServer, server_t& electedServer);

server_t getNextServer(const metadata_t& metadata, const server_t& currentServer);

void sendElectionMessage(server_t& targetServer, server_t& winningServer); //TODO

void sendElectedMessage(server_t& targetServer, server_t& electedServer); //TODO
