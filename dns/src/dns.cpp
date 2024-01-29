#include "../include/dns.h"

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
#include "../include/state.h"
#include "../include/connection.h"


void *dns_thread_init(void *args) {
    uint16_t port = *((uint16_t*) args);

    // Variables
    struct sockaddr_in server_address;
    int listen_sockfd;
    int connection_sockfd;
    socklen_t client_length;
    struct sockaddr_in client_address;

    // Configure socket to listen
    {
        // std::cerr << "[DEBUG] [Coms] Binding socket" << std::endl;
        // Initialize the socket
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(perror("\033[31mERROR: [DNS] Call to 'socket' failed"); std::cerr << "\0330m");
            exit(1);
        }

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(perror("\033[31mERROR: [DNS] Call to 'bind' failed\033[0m"));
            exit(1);
        }

        // Setup listen queue
        listen(listen_sockfd, 15);

        // Register server socket into the closeable connection list
        add_connection(listen_sockfd);
    }
    LOG_SYNC(std::cout << "[DNS] Setup successful" << std::endl);

    // Handle clients
    {
        client_length = sizeof(struct sockaddr_in);

        while (true) {
            // Accept connecting clients and break on 
            connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
            if (connection_sockfd == -1) {
                LOG_SYNC(perror("\033[31mERROR: [DNS] Call to 'accept' failed\033[0m"));
                break;
            }

            add_connection(connection_sockfd);

            connection_t *connection = conn_new(
                    server_address.sin_addr,
                    ntohs(server_address.sin_port),
                    client_address.sin_addr,
                    ntohs(client_address.sin_port),
                    connection_sockfd);

            uint8_t message_type;

            if (!read_u8(connection->reader, &message_type)) {
                LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Could not read message type\033[0m" << std::endl);
                close(connection->sockfd);
                continue;
            }

            if (message_type == 10) {
                uint32_t primary_server_ip;
                uint16_t primary_server_port;

                get_primary_server(&primary_server_ip, &primary_server_port);

                if (!write_u32(connection->writer, primary_server_ip)) {
                    LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Could not send ip\033[0m" << std::endl);
                    close(connection->sockfd);
                    continue;
                }

                if (!write_u16(connection->writer, primary_server_port)) {
                    LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Could not send port\033[0m" << std::endl);
                    close(connection->sockfd);
                    continue;
                }

                if (!flush(connection->writer)) {
                    LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Could not flush\033[0m" << std::endl);
                    close(connection->sockfd);
                    continue;
                }

                LOG_SYNC(std::cerr << "\033[32mERROR: [DNS] Sent primary server (" << std::hex << primary_server_ip << std::dec << ":" << primary_server_port << ")\033[0m" << std::endl);
            } else if (message_type == 20) {
                uint32_t primary_server_ip;
                uint16_t primary_server_port;

                if (!read_u32(connection->reader, &primary_server_ip)) {
                    LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Could not send ip\033[0m" << std::endl);
                    close(connection->sockfd);
                    continue;
                }

                if (!read_u16(connection->reader, &primary_server_port)) {
                    LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Could not send port\033[0m" << std::endl);
                    close(connection->sockfd);
                    continue;
                }

                set_primary_server(primary_server_ip, primary_server_port);
                LOG_SYNC(std::cerr << "\033[32mERROR: [DNS] Updated primary server to " << std::hex << primary_server_ip << std::dec << ":" << primary_server_port << "\033[0m" << std::endl);
            } else {
                LOG_SYNC(std::cerr << "\033[31mERROR: [DNS] Received invalid message type (" << ((uint32_t) message_type) << ")\033[0m" << std::endl);
                close(connection->sockfd);
                continue;
            }
            close(connection->sockfd);
        }
    }

    // Thread teardown
    {
        LOG_SYNC(std::cerr << "[DNS] Closing" << std::endl;);

        close(listen_sockfd);
        exit(EXIT_FAILURE);
        return NULL; 
    }
}

bool dns_thread_init(uint16_t *port) {
    pthread_t thread;
    pthread_create(&thread, NULL, dns_thread_init, port);
    return true;
}
