#include "../include/server.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <semaphore.h>
#include <unordered_map>
#include <string>
#include <vector>

#define MAX_ACTIVE_CONNECTIONS 2
// std::unordered_map<std::string, std::vector<client_connection_t*>> connections;
std::unordered_multimap<std::string, client_t*> clients;

/*
 * For a client to be considered active, it has to be considered active 
 * by the application and have an active TCP connection
 */
bool _client_is_valid(client_t *client) {
    if (!client->active) {
        return false;
    }

    uint32_t error_code;
    uint32_t error_code_size = sizeof(error_code);
    getsockopt(client->connection->sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
    
    if (error_code != 0) {
        return false;
    }

    return true;
}

bool can_connect(std::string username) {
    auto range = clients.equal_range(username);

    for (auto it = range.first; it != range.second; ) {
        if (_client_is_valid(it->second)) {
            client_free(it->second);
            clients.erase(it);
        } else {
            it++;
        }
    }

    return clients.count(username) < MAX_ACTIVE_CONNECTIONS;
}

client_t *client_new(std::string username, connection_t *connection) {
    // Check wether this client has exceded the maximum 
    // allowed limit of connections per client
    if (!can_connect(username)) {
        return nullptr;
    }

    // Allocate memory for the client
    client_t *client = (client_t*) malloc(sizeof(client_t));
    
    // Initialize the client's variables
    client->username = username;
    client->connection = connection;
    client->active = true;
    client->pending_events = std::queue<file_event_t>();
    sem_init(&client->mutex, 0, 1);

    // Insert the new client into the client list
    clients.insert({username, client});
    return client;
}

void client_free(client_t *client) {
    close(client->connection->sockfd);
    free(client->connection);
    free(client);
}

void client_broadcast_file_created(std::string username, std::string filename) {
    auto range = clients.equal_range(username);
    for (auto it = range.first; it != range.second; ) {
        client_t *client = it->second;
        sem_wait(&client->mutex);
        client->pending_events.push({event_file_created, filename});
        sem_post(&client->mutex);
    }
}

void client_broadcast_file_modified(std::string username, std::string filename) {
    auto range = clients.equal_range(username);
    for (auto it = range.first; it != range.second; ) {
        client_t *client = it->second;
        sem_wait(&client->mutex);
        client->pending_events.push({event_file_modified, filename});
        sem_post(&client->mutex);
    }
}

void client_broadcast_file_deleted(std::string username, std::string filename) {
    auto range = clients.equal_range(username);
    for (auto it = range.first; it != range.second; ) {
        client_t *client = it->second;
        sem_wait(&client->mutex);
        client->pending_events.push({event_file_deleted, filename});
        sem_post(&client->mutex);
    }
}

bool client_get_event(client_t *client, file_event_t *out_event) {
    sem_wait(&client->mutex);
    if (client->pending_events.size() > 0) {
        file_event_t event = client->pending_events.front();
        client->pending_events.pop();
        *out_event = event;
        sem_post(&client->mutex);
        return true;
    }
    sem_post(&client->mutex);
    return false;
}

