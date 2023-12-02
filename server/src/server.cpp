#include "../include/server.h"

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

#include "../include/client.h"
#include "../include/protocol.h"
#include "../include/file_manager.h"
#include "../include/closeable.h"

#define BUF_SIZE 1024
#define LINE_CHAR_COUNT 32

inline client_t *connect_to_client(connection_t *connection) {
    // Perform handshake
    std::string username;
    if (!handshake(connection, &username)) {
        close(connection->sockfd);
        conn_free(connection);
        return nullptr;
    }

    std::cout << "[LISTEN] (ID: " << connection->connection_id << ") Identified as `" << username << "`" << std::endl;

    // Create client
    client_t *client = client_new(username, connection);
    if (client == nullptr) {
        std::cout << "[LISTEN] (ID: " << connection->connection_id << ") Connection refused" << std::endl;
        respond_handshake_fail(connection);
        return nullptr;
    }

    std::cout << "[LISTEN] (ID: " << connection->connection_id << ") Connection accepted" << std::endl;
    respond_handshake_success(connection);
    return client;
}

void *client_handler_thread(void *_arg) {
    client_t *client = connect_to_client((connection_t*) _arg);
    if (client == nullptr) {
        pthread_exit(NULL);
        return nullptr;
    }

    // Variable declaration
    std::string client_dir;
    packet_header_t header;
    std::string filename;
    uint64_t length;
    uint8_t *bytes;
    FileManager::file_metadata_t file_metadata;
    std::vector<FileManager::file_description_t> files;
    bool running;
    file_event_t file_event;

    // Variable initialization
    client_dir = "sync_dir_" + client->username;
    running = true;

    // Server loop
    while (running) {
        bool receive_success = receive_packet(client->connection, &header, &filename, &length, &bytes);
        if (!receive_success) {
            running = false;
            break;
        }

        switch (header.packet_type) {
            case PACKET_TYPE_DOWNLOAD:
                if (FileManager::read_file(client_dir + "/" + filename, &file_metadata)) {
                    respond_download_success(client->connection, file_metadata.contents, file_metadata.length);
                    free(file_metadata.contents);
                } else {
                    respond_download_fail(client->connection);
                }
                break;
            case PACKET_TYPE_UPLOAD:
                if (FileManager::write_file(client_dir + "/" + filename, bytes, length)) {
                    respond_upload_success(client->connection);
                    client_broadcast_file_modified(client->username, filename);
                } else {
                    respond_upload_fail(client->connection);
                }
                free(bytes);
                break;
            case PACKET_TYPE_DELETE:
                if (FileManager::delete_file(client_dir + "/" + filename)) {
                    respond_delete_success(client->connection);
                    client_broadcast_file_deleted(client->username, filename);
                } else {
                    respond_delete_fail(client->connection);
                }
                break;
            case PACKET_TYPE_LIST_FILES:
                if (FileManager::list_files(client_dir, &files)) {
                    respond_list_files_success(client->connection, files);
                    files.clear();
                } else {
                    respond_list_files_fail(client->connection);
                }
                break;
            case PACKET_TYPE_UPDATE:
                if (client_get_event(client, &file_event)) {
                    respond_update_some(client->connection, file_event);
                } else {
                    respond_update_none(client->connection);
                }
                break;
            default:
                running = false;
                break;
        }
    }

    // Invalidate client and exit
    client->active = false;
    client_remove(client);
    pthread_exit(nullptr);
    return nullptr;
}

void handle_connection(connection_t *connection) {
    add_connection(connection->sockfd);
    char buffer[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &connection->client_address, buffer, sizeof( buffer ));

    std::cout << "[LISTEN] Received a connection from " << buffer << ":" << htons(connection->server_port) << " (ID: " << connection->connection_id << ")" << std::endl;

    // Create thread
    pthread_t thread;
    pthread_create(&thread, NULL, client_handler_thread, connection);
}

bool tcp_dump_1(std::string ip, uint16_t port) {
    int listen_sockfd;
    int connection_sockfd;
    socklen_t client_length;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    // Initialize the socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        return false;
    }

    // Bind the socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_address.sin_zero), 8);

    if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        return false;
    }

    // Setup listen queue
    listen(listen_sockfd, 5);

    // Register server socket into the closeable connection list
    add_connection(listen_sockfd);

    // Listen for clients
    while (true) {
        client_length = sizeof(struct sockaddr_in);
        connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
        if (connection_sockfd == -1) {
            break;
        }

        connection_t *connection = conn_new(
                server_address.sin_addr,
                ntohs(server_address.sin_port),
                client_address.sin_addr,
                ntohs(client_address.sin_port),
                connection_sockfd);

        handle_connection(connection);
    }

    close(listen_sockfd);
    return true; 
}

