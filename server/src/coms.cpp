#include "../include/coms.h"
#include <cstdlib>
#include <vector>
#include <iterator> // Add this include directive
#include <algorithm>
#include <iostream>
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


#define coms_exec(expr) \
    if (!expr) { \
        return false; \
    }

#define STATUS_OK 0
#define STATUS_INVALID_REQUEST_TYPE 10
#define STATUS_NOT_IMPLEMENTED 11

bool _coms_sync_execute_request(tcp_reader *reader, tcp_writer *writer, request_t request, response_t *out_response) {
    server_t *current_server = get_current_server();

    switch (request.type) {
        case req_transaction_start:
            coms_exec(write_u8(*writer, 12));
            break;
        case req_transaction_end:
            coms_exec(write_u8(*writer, 13));
            break;
        case req_fetch_metadata:
            coms_exec(write_u8(*writer, 14));
            break;
        case req_hello:
            coms_exec(write_u8(*writer, 15));
            coms_exec(write_u16(*writer, current_server->port));
            break;
    }

    coms_exec(flush(*writer));

    uint64_t metadata_length;
    uint64_t primary_index;
    server_t in_server;

    switch (request.type) {
        case req_transaction_start:
            coms_exec(read_u16(*reader, &out_response->status));
            break;

        case req_transaction_end:
            coms_exec(read_u16(*reader, &out_response->status));
            break;

        case req_fetch_metadata:
            out_response->metadata = { .servers = std::vector<server_t>() };
            // std::cerr << "[DEBUG] [Coms] [Meta] A1" << std::endl;
            coms_exec(read_u16(*reader, &out_response->status));
            // std::cerr << "[DEBUG] [Coms] [Meta] A2 (status = " << out_response->status << ")" << std::endl;
            if (out_response->status == STATUS_OK) {
                // std::cerr << "[DEBUG] [Coms] [Meta] A3" << std::endl;
                coms_exec(read_u64(*reader, &metadata_length));
                // std::cerr << "[DEBUG] [Coms] [Meta] A4" << std::endl;
                coms_exec(read_u64(*reader, &primary_index));
                // std::cerr << "[DEBUG] [Coms] [Meta] A5" << std::endl;
                for (uint64_t i = 0; i < metadata_length; i++) {
                    // std::cerr << "[DEBUG] [Coms] [Meta] A6" << std::endl;
                    coms_exec(read_u32(*reader, &in_server.ip));
                    // std::cerr << "[DEBUG] [Coms] [Meta] A7" << std::endl;
                    coms_exec(read_u16(*reader, &in_server.port));
                    // std::cerr << "[DEBUG] [Coms] [Meta] A*" << std::endl;
                    in_server.server_type = (i == primary_index) ? primary : backup;
                    out_response->metadata.servers.push_back(in_server);
                }
            }
            break;
        case req_hello:
            coms_exec(read_u16(*reader, &out_response->status));
            break;
    }

    return true;
}

bool coms_handle_request(tcp_reader *reader, tcp_writer *writer, server_t server) {
    // std::cerr << "[DEBUG] [Coms] [Hand] Handling request" << std::endl;
    uint8_t request_type;
    coms_exec(read_u8(*reader, &request_type));
    switch (request_type) {

        case 12:
            // req_transaction_start
            // std::cerr << "[DEBUG] [Coms] [Hand] Req = transaction start" << std::endl;
            // std::cerr << "[DEBUG] [Coms] [Hand] Done reading request" << std::endl;
            coms_exec(write_u16(*writer, STATUS_NOT_IMPLEMENTED));
            coms_exec(flush(*writer));
            break;

        case 13:
            // req_transaction_end
            // std::cerr << "[DEBUG] [Coms] [Hand] Req = transaction end" << std::endl;
            // std::cerr << "[DEBUG] [Coms] [Hand] Done reading request" << std::endl;
            coms_exec(write_u16(*writer, STATUS_NOT_IMPLEMENTED));
            coms_exec(flush(*writer));
            break;

        case 14:
            // req_fetch_metadata
            {
                std::cerr << "[DEBUG] [Coms] [Hand] Req = fetch metadata" << std::endl;
                // std::cerr << "[DEBUG] [Coms] [Hand] Done reading request" << std::endl;
                coms_exec(write_u16(*writer, STATUS_OK));
                metadata_t *metadata = acquire_metadata();
                uint64_t length = metadata->servers.size();
                uint64_t primary_index = length;
                for (uint64_t i = 0; i < length; i++) {
                    if (metadata->servers[i].server_type == primary) {
                        primary_index = i;
                        break;
                    }
                }
                coms_exec(write_u64(*writer, length));
                coms_exec(write_u64(*writer, primary_index));
                for (uint64_t i = 0; i < length; i++) {
                    coms_exec(write_u32(*writer, metadata->servers[i].ip));
                    coms_exec(write_u16(*writer, metadata->servers[i].port));
                }
                release_metadata();
                coms_exec(flush(*writer));
            }
            break;

        case 15:
            // req_hello
            {
                // std::cerr << "[DEBUG] [Coms] [Hand] Req = hello" << std::endl;
                coms_exec(read_u16(*reader, &server.port));
                // std::cerr << "[DEBUG] [Coms] [Hand] Done reading request" << std::endl;
                {
                    metadata_t *metadata = acquire_metadata();
                    metadata->servers.push_back(server);
                    release_metadata();
                }
                coms_exec(write_u16(*writer, STATUS_OK));
                coms_exec(flush(*writer));
            }
            break;

        default:
            // std::cerr << "[DEBUG] [Coms] [Hand] Req = error" << std::endl;
            coms_exec(write_u16(*writer, STATUS_INVALID_REQUEST_TYPE));
            coms_exec(flush(*writer));
            // std::cerr << "[DEBUG] [Coms] [Hand] Done" << std::endl;
            return false;
    }
    // std::cerr << "[DEBUG] [Coms] [Hand] Done" << std::endl;

    return true;
}

typedef struct {
    int listen_sockfd;
    struct sockaddr_in server_address;
} coms_listener_arguments_t;

void coms_handle_connection(connection_t *connection) {
        server_t server = {
            .ip = connection->client_address.s_addr,
            .port = connection->client_port,
            .server_type = backup,
        };

        LOG_SYNC(std::cerr << "[Coms] Received a connection from " << server << std::endl);

        bool error_occurred = false;

        while (!(error_occurred || should_stop())) {
            // std::cerr << "[DEBUG] [Coms Thread] Handling message (e = " << (error_occurred ? "true" : "false") << ")" << std::endl;
            if (!coms_handle_request(&connection->reader, &connection->writer, server)) {
                error_occurred = true;
                break;
            }
        }
}

void *coms_listener_thread(void *args) {
    struct sockaddr_in server_address;
    int listen_sockfd;

    {
        server_t *current_server = get_current_server();

        int connection_sockfd;
        socklen_t client_length;
        struct sockaddr_in client_address;

        // Initialize the socket
        // std::cerr << "[DEBUG] [Coms] Creating socket" << std::endl;
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(std::cerr << "aaaaaaaaaaaaaaaaaaaaa1" << std::endl);
            exit(1);
        }

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(current_server->port+2);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        // std::cerr << "[DEBUG] [Coms] Binding socket" << std::endl;
        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(std::cerr << "aaaaaaaaaaaaaaaaaaaaa2" << std::endl);
            exit(1);
        }

        // Setup listen queue
        // std::cerr << "[DEBUG] [Coms] Listening on socket" << std::endl;
        listen(listen_sockfd, 15);

        // Register server socket into the closeable connection list
        // std::cerr << "[DEBUG] [Coms] Storing socket" << std::endl;
        add_connection(listen_sockfd);
    }

    int connection_sockfd;
    struct sockaddr_in client_address;
    socklen_t client_length;

    // Listen for clients
    while (!should_stop()) {
        // std::cerr << "[DEBUG] [Coms Thread] Waiting for client" << std::endl;
        client_length = sizeof(struct sockaddr_in);
        connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
        if (connection_sockfd == -1) {
            break;
        }
        // std::cerr << "[DEBUG] [Coms Thread] Handling client" << std::endl;
        add_connection(connection_sockfd);

        connection_t *connection = conn_new(
                server_address.sin_addr,
                ntohs(server_address.sin_port),
                client_address.sin_addr,
                ntohs(client_address.sin_port),
                connection_sockfd);

        coms_handle_connection(connection);
    }

    close(listen_sockfd);
    pthread_exit(NULL);
    return NULL; 
}

bool coms_thread_init() {
    // Create the thread
    // std::cerr << "[DEBUG] [Coms] Creating thread" << std::endl;
    pthread_t thread;
    pthread_create(&thread, NULL, coms_listener_thread, NULL);
    return true;
}

void *heartbeat_listener_thread(void *args) {
    struct sockaddr_in server_address;
    int listen_sockfd;

    {
        server_t *current_server = get_current_server();

        // Initialize the socket
        // std::cerr << "[DEBUG] [Coms] Creating socket" << std::endl;
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(std::cerr << "bbbbbbbbbbbbbbbbbbbbb1" << std::endl);
            exit(1);
        }

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(current_server->port+3);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        // std::cerr << "[DEBUG] [Coms] Binding socket" << std::endl;
        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(std::cerr << "bbbbbbbbbbbbbbbbbbbbb2" << std::endl);
            exit(1);
        }

        // Setup listen queue
        // std::cerr << "[DEBUG] [Coms] Listening on socket" << std::endl;
        listen(listen_sockfd, 15);

        // Register server socket into the closeable connection list
        // std::cerr << "[DEBUG] [Coms] Storing socket" << std::endl;
        add_connection(listen_sockfd);
    }

    int connection_sockfd;
    struct sockaddr_in client_address;
    socklen_t client_length;

    // Listen for clients
    while (!should_stop()) {
        // std::cerr << "[DEBUG] [Coms Thread] Waiting for client" << std::endl;
        client_length = sizeof(struct sockaddr_in);
        connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
        if (connection_sockfd == -1) {
            break;
        }
        // std::cerr << "[DEBUG] [Coms Thread] Handling client" << std::endl;
        add_connection(connection_sockfd);

        connection_t *connection = conn_new(
                server_address.sin_addr,
                ntohs(server_address.sin_port),
                client_address.sin_addr,
                ntohs(client_address.sin_port),
                connection_sockfd);

        coms_handle_connection(connection);
        std::vector <connection_t*> *coneccoes = get_heartbeat_connections();
        (*coneccoes).push_back(connection);
        release_heartbeat_connections();
    }

    close(listen_sockfd);
    pthread_exit(NULL);
    return NULL; 
}

void *heartbeat_writer_thread(void *args) {
    while (!should_stop())
    {
        connection_t *conn;
        {

            server_t *primary_server = get_primary_server();
            server_t *current_server = get_current_server();

            if (server_eq(primary_server,current_server)){
                return NULL;
            }
            // Setup
            // std::cerr << "[DEBUG] Seting up sockets" << std::endl;
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons((*primary_server).port + 3);
            server_addr.sin_addr = { (*primary_server).ip };
            bzero(&server_addr.sin_zero, 8);

            // Create socket
            // std::cerr << "[DEBUG] Creating socket" << std::endl;
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                std::cerr << "ERROR: [Election connection init 1a] Could not create the socket" << std::endl;
                return NULL;
            }

            // Connect
            // std::cerr << "[DEBUG] Connecting to primary" << std::endl;
            int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
            if (connect_response < 0) {
                std::cerr << "ERROR: [Election connection init 1a] Could not connect to the server" << std::endl;
                close(sockfd);
                return NULL;
            }

            int optval = 1;
            socklen_t optlen = sizeof(optval);
            if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
                perror("setsockopt()");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
                perror("getsockopt()");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            std::cerr << "SO_KEEPALIVE is " << (optval ? "ON" : "OFF") << std::endl;

            // TODO - Kaiser: KEEP_ALIVE
            // TODO - Kaiser: botar um connection_t no state (talvez um mutex?).
            // TODO - Kaiser: Idealmente, essa conexão pega o metadata e conecta no primario dessa conexao, mas pode considerar que essa conexao sempre eh com o primario por enquanto

            add_connection(sockfd);

            // Create connection object
            // INTERNAL: the client's fields are set to the server's fields on purpose
            // TODO - Didio: figure out how to get client's IP and port
            // std::cerr << "[DEBUG] Connecting connection object" << std::endl;
            conn = conn_new(
                    server_addr.sin_addr,
                    ntohs(server_addr.sin_port),
                    server_addr.sin_addr,
                    ntohs(server_addr.sin_port),
                    sockfd);

            set_heartbeat_socket(conn);
        }
        
        while (!should_stop())
        {
            sleep(2);
            if (!write_u8(conn->writer,0) || !flush(conn->writer)){
                std::cerr << "[Heartbeat] connection reset" << std::endl;
                initiateElection();
                break;
            }
            std::cerr << "[Heartbeat] heartbeat sent from backup" << std::endl;
        }    
    }
    
    return NULL;
}

bool heartbeat_thread_init() {
    // Create the thread
    // std::cerr << "[DEBUG] [Coms] Creating thread" << std::endl;
    pthread_t thread,thread2;
    pthread_create(&thread, NULL, heartbeat_listener_thread, NULL);
    pthread_create(&thread2, NULL, heartbeat_writer_thread, NULL);
    return true;
}

bool primary_heartbeat_thread_init() {
    // Create the thread
    // std::cerr << "[DEBUG] [Coms] Creating thread" << std::endl;
    pthread_t thread;
    pthread_create(&thread, NULL, heartbeat_listener_thread, NULL);
    return true;
}
