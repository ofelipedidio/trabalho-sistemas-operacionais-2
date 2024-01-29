// TODO - Didio: This file is riddled with horrible, horrible error messages. After testing, they needs to be replaced with actual error handling or, at least, good error messages

#include <iterator>
#include <algorithm>
#include <asm-generic/errno.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/stat.h>
#include <pthread.h>

#include "../include/closeable.h"
#include "../include/election.h"
#include "../include/state.h"

#define MESSAGE_ELECTION 10
#define MESSAGE_ELECTED 20

// INTERNAL
bool comp(const server_t a, const server_t b){
    if( a.ip > b.ip ) return true;
    if( a.ip < b.ip) return false;
    return (a.port > b.port);
}

bool server_eq(const server_t *a, const server_t *b) {
    return a->ip == b->ip && a->port == b->port;
}

void initiateElection(server_t primary_server) {
    server_t *currentServer = get_current_server();
    if((*currentServer).server_type == primary) return;
    metadata_t *metadata = acquire_metadata();
    {
        metadata_t new_metadata;
        for(auto server : metadata->servers) {
            if (!server_eq(&server, &primary_server)) {
                new_metadata.servers.push_back(server);
            }
        }
        if (new_metadata.servers.size() == metadata->servers.size()) {
            LOG_SYNC(std::cout << "[ELECTION_CLIENT] Election initiation request ignored because the primary has already changed" << std::endl);
            release_metadata();
            return;
        }
        *metadata = new_metadata;
    }
    // TODO - Didio: handle closing connections with primary and such

    release_metadata();
    
    // Get next server
    server_t nextServer = getNextServer(*currentServer);

    // Send election message to the next server
    sendElectionMessage(nextServer, *currentServer);
}

/*
 * Get the next server from a given server in the ring
 */
server_t getNextServer(const server_t currentServer) {
    // INTERNAL: == is not defined for (const server_t, const server_t)

    // Return value
    // INTERNAL: Fallback: set next server as the current server
    server_t next_server = currentServer;

    // Acquire the metadata
    metadata_t *metadata = acquire_metadata();
    // Critical section
    {
        // Iterates over the servers to find the index of the current server
        std::size_t self = metadata->servers.size();
        for (std::size_t i = 0; i < metadata->servers.size(); i++) {
            if (server_eq(&metadata->servers[i], &currentServer)) {
                self = i;
                break;
            }
        }

        // Check for data consistency
        if (self == metadata->servers.size()) {
            release_metadata();
            LOG_SYNC(std::cerr << "[Consistency fail] Failed to find currentServer in metadata!" << std::endl);
            exit(EXIT_FAILURE);
        }

        // Iterate over the servers to find the next non-primary server
        next_server = metadata->servers[(self+1) % metadata->servers.size()];
    }
    release_metadata();

    return next_server;
}

/*
 * Prints the server to stderr
 */
void printServer(std::ostream &stream, const server_t server){
    stream << std::hex << server.ip << std::dec << ':' << server.port << "[" << (server.server_type == primary ? "p" : "b") << "]";
}

/***************************************\
*                                       *
*       Outbound message handling       *
*                                       *
\***************************************/

/*
 * Send election message to nextServer
 */
void sendElectionMessage(server_t nextServer, server_t winningServer) {
    // Connect to the next server
    LOG_SYNC(std::cout << "[ELECTION_CLIENT] Sending <election, " << winningServer << "> to " << nextServer << std::endl);
    connection_t *conn;
    {
        // Setup
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(nextServer.port+1);
        server_addr.sin_addr = { nextServer.ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Could not create the socket\033[0m" << std::endl);
            return;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Could not connect to the server\033[0m" << std::endl);
            return;
        }

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);
    }

    // Send election message
    if (!write_u8(conn->writer, MESSAGE_ELECTION)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (message type)\033[0m" << std::endl);
        return;
    }
    if (!write_u32(conn->writer, winningServer.ip)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (ip)\033[0m" << std::endl);
        return;
    }
    if (!write_u16(conn->writer, winningServer.port)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (port)\033[0m" << std::endl);
        return;
    }
    if (!flush(conn->writer)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (flush)\033[0m" << std::endl);
        return;
    }

    LOG_SYNC(std::cout << "\033[35m[ELECTION_CLIENT] Sent <election, " << winningServer << "> to " << nextServer << " successfully\033[0m" << std::endl);
}

/*
 * Send elected message to nextServer
 */
void sendElectedMessage(server_t nextServer, server_t electedServer) {
    // Connect to the next server
    LOG_SYNC(std::cout << "[ELECTION_CLIENT] Sending <elected, " << electedServer << "> to " << nextServer << std::endl);
    connection_t *conn;
    {
        // Setup
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(nextServer.port+1);
        server_addr.sin_addr = { nextServer.ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Could not create the socket\033[0m" << std::endl);
            return;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Could not connect to the server\033[0m" << std::endl);
            return;
        }

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);
    }

    // Send elected message
    if (!write_u8(conn->writer, MESSAGE_ELECTED)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (message type)\033[0m" << std::endl);
        return;
    }
    if (!write_u32(conn->writer, electedServer.ip)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (ip)\033[0m" << std::endl);
        return;
    }
    if (!write_u16(conn->writer, electedServer.port)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (port)\033[0m" << std::endl);
        return;
    }
    if (!flush(conn->writer)) {
        LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed write attempt (flush)\033[0m" << std::endl);
        return;
    }

    LOG_SYNC(std::cout << "\033[35m[ELECTION_CLIENT] Sent <elected, " << electedServer << "> to " << nextServer << " successfully\033[0m" << std::endl);
}

/*
 * Updates state to acknoleadge that the current server is the primary
 */
void setElected() {
    metadata_t *metadata = acquire_metadata();
    // Critical section
    server_t *current_server = get_current_server();
    {
        current_server->server_type = primary;
        for (int i = 0; i < metadata->servers.size(); i++) {
            if (server_eq(current_server, &metadata->servers[i])) {
                metadata->servers[i].server_type = primary;
                break;
            }
        }
    }
    release_metadata();

    {
        uint32_t dns_ip;
        uint16_t dns_port;
        connection_t *conn;

        get_dns(&dns_ip, &dns_port);
        if (!connect_to_server(dns_ip, dns_port, &conn)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed to connect to DNS\033[0m" << std::endl);
            exit(EXIT_FAILURE);
        }

        if (!write_u8(conn->writer, 20)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed to communicate to DNS (message type)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        if (!write_u32(conn->writer, current_server->ip)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed to communicate to DNS (ip)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        if (!write_u16(conn->writer, current_server->port)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed to communicate to DNS (port)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        if (!flush(conn->writer)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_CLIENT] Failed to communicate to DNS (flush)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        close(conn->sockfd);
    }

    LOG_SYNC(std::cout << "\033[32m[ELECTION] This server is now the primary server\033[0m" << std::endl);
}

/*
 * Connect to the elected server and get updated metadata
 */
bool updateElected(server_t electedServer) {
    // Connect to the elected server
    connection_t *conn;
    {
        // Setup
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(electedServer.port+2);
        server_addr.sin_addr = { electedServer.ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [Election connection init 1] Could not create the socket\033[0m" << std::endl);
            return false;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [Election connection init 1] Could not connect to the server\033[0m" << std::endl);
            return false;
        }

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);
    }

    // Execute request
    request_t request = { .type = req_fetch_metadata };
    response_t response;
    if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
        // Catch network errors
        LOG_SYNC(std::cerr << "\033[31mERROR 100\033[0m" << std::endl);
        return false;
    }

    // Catch logic errors
    if (response.status != 0) {
        LOG_SYNC(std::cerr << "\033[31mERROR 101\033[0m" << std::endl);
        return false;
    }

    // Handle response
    metadata_t *metadata = acquire_metadata();
    // Critical section
    {
        metadata->servers.clear();
        for (auto server : response.metadata.servers) {
            LOG_SYNC(std::cerr << "aaa: " << server << std::endl);
            metadata->servers.push_back(server);
        }
    }
    release_metadata();

    // Close the connection and free the allocated memory
    close(conn->sockfd);
    conn_free(conn);
    return true;
}

/**************************************\
*                                      *
*       Inbound message handling       *
*                                      *
\**************************************/

/*
 * Inbound election message handler
 */
void handle_async_election_message(server_t winningServer) {
    LOG_SYNC(std::cout << "\033[35m[ELECTION_SERVER] Received <election, " << winningServer << "> message\033[0m" << std::endl);

    metadata_t *metadata = acquire_metadata();
    {
        metadata_t new_metadata;
        for(auto server : metadata->servers) {
            if (server.server_type != primary) {
                new_metadata.servers.push_back(server);
            }
        }
        *metadata = new_metadata;
    }

    release_metadata();
    server_t *currentServer = get_current_server();
    server_t nextServer = getNextServer(*currentServer);
    if(server_eq(currentServer, &winningServer)) {
        setElected();
        return sendElectedMessage(nextServer, *currentServer);
    }
    if(comp(*currentServer, winningServer)) {
        return sendElectionMessage(nextServer, *currentServer);
    } else {
        return sendElectionMessage(nextServer, winningServer);
    }
}

/*
 * Inbound elected message handler
 */
void handle_async_elected_message(server_t electedServer){
    LOG_SYNC(std::cout << "\033[35m[ELECTION_SERVER] Received <elected, " << electedServer << "> message\033[0m" << std::endl);

    server_t *currentServer = get_current_server();
    if (!server_eq(currentServer, &electedServer)) {
        updateElected(electedServer);
        server_t nextServer = getNextServer(*currentServer);
        sendElectedMessage(nextServer, electedServer);
    }
}

/*
 * Structure that holds data that is sent when the election thread is created
 */
typedef struct {
    int listen_sockfd;
    struct sockaddr_in server_address;
} el_listener_arguments_t;

/*
 * Thread that handles election messages
 */
void *election_listener_thread(void *args) {
    struct sockaddr_in server_address;
    int listen_sockfd;

    {
        server_t *current_server = get_current_server();
        int connection_sockfd;
        socklen_t client_length;
        struct sockaddr_in client_address;

        // Initialize the socket
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(perror("\033[31mERROR: [ELECTION_SERVER] Call to 'socket' failed"); std::cerr << "\033[0m");
            exit(1);
        }

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(current_server->port+1);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(perror("ERROR: [ELECTION_SERVER] Call to 'bind' failed"));
            exit(1);
        }

        // Setup listen queue
        listen(listen_sockfd, 15);

        // Register server socket into the closeable connection list
        add_connection(listen_sockfd);
    }

    int connection_sockfd;
    struct sockaddr_in client_address;
    socklen_t client_length;

    uint8_t request;
    server_t server;

    LOG_SYNC(std::cout << "[ELECTION_SERVER] Setup successful" << std::endl);

    // Listen for clients
    while (!should_stop()) {
        client_length = sizeof(struct sockaddr_in);
        connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
        if (connection_sockfd == -1) {
            break;
        }
        add_connection(connection_sockfd);

        connection_t *conn = conn_new(
                server_address.sin_addr,
                ntohs(server_address.sin_port),
                client_address.sin_addr,
                ntohs(client_address.sin_port),
                connection_sockfd);

        server_t server = {
            .ip = conn->client_address.s_addr,
            .port = conn->client_port,
            .server_type = backup,
        };
        LOG_SYNC(std::cerr << "[ELECTION_SERVER] Received a connection from " << server << std::endl);

        // Read message
        if (!read_u8(conn->reader, &request)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_SERVER] Failed to read request (message type)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
            break;
        }
        if (!read_u32(conn->reader, &server.ip)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_SERVER] Failed to read request (ip)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
            break;
        }
        if (!read_u16(conn->reader, &server.port)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_SERVER] Failed to read request (port)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
            break;
        }

        // Call the appropriate handler
        if (request == 10) {
            handle_async_election_message(server);
        } else if (request == 20) {
            handle_async_elected_message(server);
        } else {
            LOG_SYNC(std::cerr << "\033[31mERROR: [ELECTION_SERVER] Read unknown request message type (" << ((uint32_t) request) << ")\033[0m" << std::endl);
            close(conn->sockfd);
            break;
        }
        close(conn->sockfd);
    }

    close(listen_sockfd);
    exit(EXIT_FAILURE);
    return NULL; 
}

bool el_start_thread() {
    // Create the thread
    pthread_t thread;
    pthread_create(&thread, NULL, election_listener_thread, NULL);
    return true;
}

