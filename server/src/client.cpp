#include "../include/client.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <semaphore.h>
#include <unordered_map>
#include <string>
#include <vector>

#include "../include/connection.h"

#define MAX_ACTIVE_CONNECTIONS 2

std::vector<client_t*> clients;
sem_t global_mutex;

void client_init() {
    clients = std::vector<client_t*>();
    sem_init(&global_mutex, 0, 1);
}

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

// This assumes you have the mutex
bool can_connect(std::string username) {
    std::vector<std::size_t> invalid_clients = std::vector<std::size_t>();

    for (std::size_t i = 0; i < clients.size(); i++) {
        if (!_client_is_valid(clients[i])) {
            invalid_clients.push_back(i);
        }
    }

    if (invalid_clients.size() > 0) {
        for (std::size_t i = invalid_clients.size()-1; i >= 0; i--) {
            clients.erase(std::next(clients.begin(), invalid_clients[i]));
        }
    }

    int count = 0;
    for (std::size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->username == username) {
            count++;
        }
    }

    std::cerr << "(" << username << ", " << count << ")" << std::endl;
    return count < MAX_ACTIVE_CONNECTIONS;
}

client_t *client_new(std::string username, connection_t *connection) {
    sem_wait(&global_mutex);

    // Check wether this client has exceded the maximum 
    // allowed limit of connections per client
    if (!can_connect(username)) {
        sem_post(&global_mutex);
        return nullptr;
    }

    // Allocate memory for the client
    client_t *client = (client_t*) malloc(sizeof(client_t));
    if (client == NULL) {
        std::cout << "ERROR: [client_new with username = `" << username << "`] Could not allocate memory for the client" << std::endl;
        sem_post(&global_mutex);
        return nullptr;
    }

    // Initialize the client's variables
    client->username = username;
    client->connection = connection;
    client->active = true;
    client->pending_events = std::queue<file_event_t>();
    sem_init(&client->mutex, 0, 1);

    // Insert the new client into the client list
    clients.push_back(client);

    sem_post(&global_mutex);
    return client;
}

void client_remove(client_t *client) {
    sem_wait(&global_mutex);
    client->active = false;

    int client_index = -1;
    for (std::size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->connection->connection_id == client->connection->connection_id) {
            client_index = i;
            break;
        }
    }

    if (client_index < 0) {
        return;
    }

    clients.erase(std::next(clients.begin(), client_index));
    client_free(client);

    sem_post(&global_mutex);
}

void client_free(client_t *client) {
    close(client->connection->sockfd);
    conn_free(client->connection);
    free(client);
}

void client_broadcast_file_modified(std::string username, std::string filename) {
    sem_wait(&global_mutex);
    for (std::size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->username == username) {
            sem_wait(&clients[i]->mutex);
            std::cerr << "broadcasting file modified to `" << username << "`(" << clients[i]->connection->connection_id << ") for file `" << filename << "`" << std::endl;
            clients[i]->pending_events.push({event_file_modified, filename});
            sem_post(&clients[i]->mutex);
        }
    }
    sem_post(&global_mutex);
}

void client_broadcast_file_deleted(std::string username, std::string filename) {
    sem_wait(&global_mutex);
    for (std::size_t i = 0; i < clients.size(); i++) {
        if (clients[i]->username == username) {
            sem_wait(&clients[i]->mutex);
            std::cerr << "broadcasting file deleted to `" << username << "`(" << clients[i]->connection->connection_id << ") for file `" << filename << "`" << std::endl;
            clients[i]->pending_events.push({event_file_deleted, filename});
            sem_post(&clients[i]->mutex);
        }
    }
    sem_post(&global_mutex);
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

