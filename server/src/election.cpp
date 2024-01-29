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

void initiateElection() {
    server_t *currentServer = get_current_server();
    if((*currentServer).server_type == primary) return;
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
    // TODO - Didio: handle closing connections with primary and such

    release_metadata();
    
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [1] Initiated" << std::endl);
    // Get current server
    
    // Get next server
    server_t nextServer = getNextServer(*currentServer);

    LOG_SYNC(std::cerr << "[DEBUG] [Election] [3] Next server is " << nextServer << std::endl);

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
            fprintf(stderr, "[Consistency fail] Failed to find currentServer in metadata!\n");
            exit(EXIT_FAILURE);
        }

        // Iterate over the servers to find the next non-primary server
        next_server = metadata->servers[(self+1) % metadata->servers.size()];
        LOG_SYNC(std::cerr << "[DEBUG] [Election] [2] Iterated through " << next_server << std::endl);
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
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [4] (election) Sending election to " << nextServer << std::endl);
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
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1] Could not create the socket" << std::endl);
            return;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 2] Could not connect to the server" << std::endl);
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
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [5] (election) Connection established" << std::endl);

    // Send election message
    if (!write_u8(conn->writer, MESSAGE_ELECTION)) {
        LOG_SYNC(std::cerr << "ERROR 1" << std::endl);
        return;
    }
    if (!write_u32(conn->writer, winningServer.ip)) {
        LOG_SYNC(std::cerr << "ERROR 2" << std::endl);
        return;
    }
    if (!write_u16(conn->writer, winningServer.port)) {
        LOG_SYNC(std::cerr << "ERROR 3" << std::endl);
        return;
    }
    // Flush writer
    if (!flush(conn->writer)) {
        LOG_SYNC(std::cerr << "ERROR 4" << std::endl);
        return;
    }
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [6] (election) Message sent" << std::endl);
}

/*
 * Send elected message to nextServer
 */
void sendElectedMessage(server_t nextServer, server_t electedServer) {
    // Connect to the next server
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [8] (elected) Sending election to " << nextServer << std::endl);
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
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 3] Could not create the socket" << std::endl);
            return;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 4] Could not connect to the server" << std::endl);
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
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [9] (elected) Connection established" << std::endl);

    // Send elected message
    if (!write_u8(conn->writer, MESSAGE_ELECTED)) {
        LOG_SYNC(std::cerr << "ERROR 1" << std::endl);
        return;
    }
    if (!write_u32(conn->writer, electedServer.ip)) {
        LOG_SYNC(std::cerr << "ERROR 2" << std::endl);
        return;
    }
    if (!write_u16(conn->writer, electedServer.port)) {
        LOG_SYNC(std::cerr << "ERROR 3" << std::endl);
        return;
    }
    // Flush writer
    if (!flush(conn->writer)) {
        LOG_SYNC(std::cerr << "ERROR 4" << std::endl);
        return;
    }
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [10] (elected) Message sent" << std::endl);
}

/*
 * Updates state to acknoleadge that the current server is the primary
 */
void setElected() {
    metadata_t *metadata = acquire_metadata();
    // Critical section
    {
        server_t *current_server = get_current_server();
        current_server->server_type = primary;
        for (int i = 0; i < metadata->servers.size(); i++) {
            if (server_eq(current_server, &metadata->servers[i])) {
                metadata->servers[i].server_type = primary;
                break;
            }
        }
    }
    release_metadata();

    // TODO - Didio: client stuff (update connection and such)
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
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1] Could not create the socket" << std::endl);
            return false;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1] Could not connect to the server" << std::endl);
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
        LOG_SYNC(std::cerr << "ERROR 100" << std::endl);
        return false;
    }

    // Catch logic errors
    if (response.status != 0) {
        LOG_SYNC(std::cerr << "ERROR 101" << std::endl);
        return false;
    }

    // Handle response
    // std::cerr << "[DEBUG] Acquiring metadata" << std::endl;
    metadata_t *metadata = acquire_metadata();
    // Critical section
    {
        // std::cerr << "[DEBUG] Updating metadata (len = " << response.metadata.servers.size() << ")" << std::endl;
        metadata->servers.clear();
        for (auto server : response.metadata.servers) {
            LOG_SYNC(std::cerr << "aaa: " << server << std::endl);
            metadata->servers.push_back(server);
        }
    }
    // std::cerr << "[DEBUG] Releasing metadata" << std::endl;
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
    // TODO - Didio: handle closing connections with primary and such

    release_metadata();
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [7] (election) Receive message with server (" << winningServer << ")" << std::endl);

    server_t *currentServer = get_current_server();
    // Get next server
    server_t nextServer = getNextServer(*currentServer);
    // Check if the message has reached the highest id server
    if(server_eq(currentServer, &winningServer)) {
        setElected();
        return sendElectedMessage(nextServer, *currentServer);
    }
    // Check if the current server has higher id then the winning server
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
    LOG_SYNC(std::cerr << "[DEBUG] [Election] [11] (elected) Receive message with server (" << electedServer << ")" << std::endl);

    server_t *currentServer = get_current_server();
    LOG_SYNC(std::cerr << "[Election] election finished (" << electedServer << ")" << std::endl);
    // Check if the current server is the elected server (election is concluded) and forward the message if it isn't
    if (!server_eq(currentServer, &electedServer)) {
        // Update the current server's metadata with the elected server's metadata
        updateElected(electedServer);
        // Get the next server
        server_t nextServer = getNextServer(*currentServer);
        // Send elected message to the next server
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
    LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Thread created" << std::endl);

    struct sockaddr_in server_address;
    int listen_sockfd;

    {
        server_t *current_server = get_current_server();
        LOG_SYNC(std::cerr << "[DEBUG] [ELECTION] current_server = " << *current_server << std::endl);

        int connection_sockfd;
        socklen_t client_length;
        struct sockaddr_in client_address;

        // Initialize the socket
        LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Creating socket" << std::endl);
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(std::cerr << "aaaaaaaaaaaaaaaaaaaaa1" << std::endl);
            exit(1);
        }

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(current_server->port+1);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        LOG_SYNC(std::cerr << "[DEBUG] [ELECTION] Binding socket to " << std::hex << server_address.sin_addr.s_addr << std::dec << ":" << ntohs(server_address.sin_port) << std::endl);
        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(std::cerr << "aaaaaaaaaaaaaaaaaaaaa2" << std::endl);
            exit(1);
        }

        // Setup listen queue
        LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Listening on socket" << std::endl);
        listen(listen_sockfd, 15);

        // Register server socket into the closeable connection list
        LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Storing socket" << std::endl);
        add_connection(listen_sockfd);
    }

    int connection_sockfd;
    struct sockaddr_in client_address;
    socklen_t client_length;

    uint8_t request;
    server_t server;

    wait_for_init();

    // Listen for clients
    while (!should_stop()) {
        LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Waiting for client" << std::endl);
        client_length = sizeof(struct sockaddr_in);
        connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
        if (connection_sockfd == -1) {
            break;
        }
        LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Handling client" << std::endl);
        add_connection(connection_sockfd);

        connection_t *conn = conn_new(
                server_address.sin_addr,
                ntohs(server_address.sin_port),
                client_address.sin_addr,
                ntohs(client_address.sin_port),
                connection_sockfd);

        server_t server = {
            .ip = client_address.sin_addr.s_addr,
            .port = client_address.sin_port,
            .server_type = backup,
        };

        LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Received a connection from " << server << std::endl);

        // Read message
        if (!read_u8(conn->reader, &request)) {
            LOG_SYNC(std::cerr << "[DEBUG] [Election Thread] Received a connection from ");
            LOG_SYNC(fprintf(stderr, "Could not read request from election thread!\n"));
            close(conn->sockfd);
            exit(EXIT_FAILURE);
            break;
        }
        if (!read_u32(conn->reader, &server.ip)) {
            fprintf(stderr, "Could not read ip from election thread!\n");
            close(conn->sockfd);
            exit(EXIT_FAILURE);
            break;
        }
        if (!read_u16(conn->reader, &server.port)) {
            fprintf(stderr, "Could not read port from election thread!\n");
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
            fprintf(stderr, "Read unknown request (%d)", (uint32_t) request);
            close(conn->sockfd);
            break;
        }
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

