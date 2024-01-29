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
#include "../include/state.h"
#include "../include/coms.h"

#define BUF_SIZE 1024
#define LINE_CHAR_COUNT 32

inline client_t *connect_to_client(connection_t *connection) {
    // Perform handshake
    std::string username;
    bool from_primary;
    if (!handshake(connection, &username,&from_primary)) {
        close(connection->sockfd);
        conn_free(connection);
        return nullptr;
    }

    std::cout << "[LISTEN] (ID: " << connection->connection_id << ") Identified as `" << username << "`" << std::endl;

    // Create client
    client_t *client = client_new(username, connection,from_primary);
    if (client == nullptr) {
        std::cout << "[LISTEN] (ID: " << connection->connection_id << ") Connection refused" << std::endl;
        respond_handshake_fail(connection);
        return nullptr;
    }

    std::cout << "[LISTEN] (ID: " << connection->connection_id << ") Connection accepted" << std::endl;
    system(("mkdir -p sync_dir_" + username).c_str());
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
    netfs::file_metadata_t file_metadata;
    std::vector<netfs::file_description_t> files;
    bool running;
    file_event_t file_event;
    char buffer[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &client->connection->client_address, buffer, sizeof( buffer ));

    // Variable initialization
    client_dir = "sync_dir_" + client->username;
    running = true;
    uint8_t status;
    metadata_t metadatacopy;
    metadata_t* backupconnections;

    // Server loop
    while (running) {
        bool receive_success = receive_packet(client->connection, &header, &filename, &length, &bytes);
        if (!receive_success) {
            running = false;
            break;
        }

        switch (header.packet_type) {
            case PACKET_TYPE_DOWNLOAD:
                std::cerr << "[" << client->connection->connection_id << "] Received a download request" << std::endl;
                if (netfs::read_file(client_dir + "/" + filename, &file_metadata)) {
                    std::cerr << "[" << client->connection->connection_id << "] Resolved a download request" << std::endl;
                    if (!respond_download_success(client->connection, file_metadata.contents, file_metadata.length)) {
                        std::cerr << "dumbass didn't check" << std::endl;
                    }
                    free(file_metadata.contents);
                } else {
                    std::cerr << "[" << client->connection->connection_id << "] Failed a download request" << std::endl;
                    if (!respond_download_fail(client->connection)) {
                        running = false;
                    }
                }
                break;
            case PACKET_TYPE_UPLOAD:
                std::cerr << "[" << client->connection->connection_id << "] Received a upload request" << std::endl;
                if(get_current_server()->server_type == primary){
                    backupconnections = acquire_metadata();
                    metadatacopy = (*backupconnections);
                    release_metadata();
                    for (auto backup_server : metadatacopy.servers)
                    {
                        if (backup_server.server_type == primary)
                        {
                            continue;
                        }
                        std::cerr << "Upload request to: " << backup_server << std::endl;
                        connection_t *connbackup;
                        if(!connect_to_server(backup_server.ip,backup_server.port,&connbackup)){
                            //TODO handle this BS
                        }

                        if(!request_handshake(connbackup,client->username,true,&status) || status != STATUS_SUCCESS){
                            //TODO handle this BS
                        }

                        if (!request_upload(connbackup, filename, bytes, length, &status)) {
                            // TODO: handle error
                        }
                        if (status != STATUS_SUCCESS) {
                            // TODO: rollback
                            if (!respond_upload_fail(client->connection)) {
                                // TODO: handle error
                            }
                        }
                        close(connbackup->sockfd);
                    }                
                }

                if (netfs::write_file(client_dir + "/" + filename, bytes, length)) {
                    std::cerr << "[" << client->connection->connection_id << "] Resolved a upload request" << std::endl;
                    if (!respond_upload_success(client->connection)) {
                        running = false;
                    }
                    client_broadcast_file_modified(client->username, filename);
                } else {
                    std::cerr << "[" << client->connection->connection_id << "] Failed a upload request" << std::endl;
                    if (!respond_upload_fail(client->connection)) {
                        running = false;
                    }
                }
                free(bytes);
                break;
            case PACKET_TYPE_DELETE:
                std::cerr << "[" << client->connection->connection_id << "] Received a delete request" << std::endl;
                
                if(get_current_server()->server_type == primary){
                    backupconnections = acquire_metadata();
                    metadatacopy = (*backupconnections);
                    release_metadata();
                    for (auto backup_server : metadatacopy.servers)
                    {
                        if (backup_server.server_type == primary)
                        {
                            continue;
                        }
                        std::cerr << "Delete request to: " << backup_server << std::endl;
                        connection_t *connbackup;
                        if(!connect_to_server(backup_server.ip,backup_server.port,&connbackup)){
                            //TODO handle this BS
                        }

                        if(!request_handshake(connbackup,client->username,true,&status) || status != STATUS_SUCCESS){
                            //TODO handle this BS
                        }

                        if (!request_delete(connbackup, filename, &status)) {
                            // TODO: handle error
                        }
                        if (status != STATUS_SUCCESS) {
                            // TODO: rollback
                            if (!respond_upload_fail(client->connection)) {
                                // TODO: handle error
                            }
                        }
                        close(connbackup->sockfd);
                    }
                }

                if (netfs::delete_file(client_dir + "/" + filename)) {
                    std::cerr << "[" << client->connection->connection_id << "] Resolved a delete request" << std::endl;
                    if (!respond_delete_success(client->connection)) {
                        running = false;
                    }
                    client_broadcast_file_deleted(client->username, filename);
                } else {
                    std::cerr << "[" << client->connection->connection_id << "] Failed a delete request" << std::endl;
                    if (!respond_delete_fail(client->connection)) {
                        running = false;
                    }
                }
                break;
            case PACKET_TYPE_LIST_FILES:
                std::cerr << "[" << client->connection->connection_id << "] Received a list_files request" << std::endl;
                if (netfs::list_files(client_dir, &files)) {
                    std::cerr << "[" << client->connection->connection_id << "] Resolved a list_files request" << std::endl;
                    if (!respond_list_files_success(client->connection, files)) {
                        running = false;
                    }
                    files.clear();
                } else {
                    std::cerr << "[" << client->connection->connection_id << "] Failed a list_files request" << std::endl;
                    if (!respond_list_files_fail(client->connection)) {
                        running = false;
                    }
                }
                break;
            case PACKET_TYPE_UPDATE:
                if (client_get_event(client, &file_event)) {
                    if (!respond_update_some(client->connection, file_event)) {
                        running = false;
                    }
                } else {
                    if (!respond_update_none(client->connection)) {
                        running = false;
                    }
                }
                break;
            default:
                running = false;
                break;
        }
    }

    std::cerr << "[" << client->connection->connection_id << "] Closed connection" << std::endl;

    // Invalidate client and exit
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

