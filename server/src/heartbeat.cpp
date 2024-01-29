#include "../include/coms.h"
#include <asm-generic/errno-base.h>
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


void *heartbeat_listener_thread(void *args) {
    struct sockaddr_in server_address;
    int listen_sockfd;

    {
        server_t *current_server = get_current_server();
        LOG_SYNC(std::cerr << "[DEBUG] [HEARTBEAT] current_server = " << *current_server << std::endl);

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
        LOG_SYNC(std::cerr << "[DEBUG] [HEARTBEAT] Binding socket to " << std::hex << server_address.sin_addr.s_addr << std::dec << ":" << ntohs(server_address.sin_port) << std::endl);
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

    wait_for_init();

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

        // coms_handle_connection(connection,);
        std::vector <connection_t*> *coneccoes = get_heartbeat_connections();
        (*coneccoes).push_back(connection);
        release_heartbeat_connections();
    }

    close(listen_sockfd);
    exit(EXIT_FAILURE);
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

            if (!connect_to_server(primary_server->ip, primary_server->port+3, &conn)) {
                LOG_SYNC(std::cerr << "[DEBUG] [HEARTBEAT] connect_to_server failed" << std::endl);
                exit(EXIT_FAILURE);
            }

            int optval = 1;
            socklen_t optlen = sizeof(optval);
            if(setsockopt(conn->sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
                perror("[DEBUG] [HEARTBEAT] setsockopt failed");
                close(conn->sockfd);
                exit(EXIT_FAILURE);
            }

            if(getsockopt(conn->sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
                perror("[DEBUG] [HEARTBEAT] getsockopt failed");
                close(conn->sockfd);
                exit(EXIT_FAILURE);
            }

            std::cerr << "SO_KEEPALIVE is " << (optval ? "ON" : "OFF") << std::endl;

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

