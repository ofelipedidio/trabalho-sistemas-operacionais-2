#include <asm-generic/errno-base.h>
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

#include "../../client/include/logger.h"
#include "../include/http_server.h"
#include "../include/reader.h"
#include "../include/file_manager.h"
#include "../include/closeable.h"

#define BUF_SIZE 1024
#define LINE_CHAR_COUNT 32

struct thread_arguments {
    struct in_addr server_address;
    in_port_t server_port;
    struct in_addr client_address;
    in_port_t client_port;
    int sockfd;
};

struct app_request {
    std::string method;
    std::string file;
    std::string username;
};

std::string assemble_dir(std::string& username) {
    return "sync_dir_" + username;
}

std::string assemble_filename(std::string& username, std::string filename) {
    return assemble_dir(username) + "/" + filename;
}

#define read_until(reader, buffer, buffer_size, index, cond, adv) while (!(cond)) { \
    buffer[index++] = peek(reader, 0); \
    advance(reader, 1); \
} \
advance(reader, adv); \
buffer[index] = '\0'; \
index = 0;

template<int SIZE>
inline void parse_request_header(
        tcp_reader<SIZE> *reader,
        char *temp_buffer,
        std::string *method,
        std::string *path,
        std::string *version,
        uint64_t *content_length,
        std::string *transfer_encoding) {
    int index = 0;

    // Read request line
    read_until(*reader, temp_buffer, SIZE, index, peek(*reader, 0) == ' ', 1);
    *method = std::string(temp_buffer);
    read_until(*reader, temp_buffer, SIZE, index, peek(*reader, 0) == ' ', 1);
    *path = std::string(temp_buffer);
    read_until(*reader, temp_buffer, SIZE, index, peek(*reader, 0) == '\r' && peek(*reader, 1) == '\n', 2);
    *version = std::string(temp_buffer);

    // Read headers
    while (!(peek(*reader, 0) == '\r' && peek(*reader, 1) == '\n')) {
        std::string header_key = "";
        std::string header_value = "";

        read_until(*reader, temp_buffer, BUF_SIZE, index, peek(*reader, 0) == ':' && peek(*reader, 1) == ' ', 2);
        header_key = std::string(temp_buffer);
        read_until(*reader, temp_buffer, BUF_SIZE, index, peek(*reader, 0) == '\r' && peek(*reader, 1) == '\n', 2);
        header_value = std::string(temp_buffer);

        // Check if key is of interest
        if (header_key == "Content-Length") {
            std::istringstream iss(header_value);
            iss >> *content_length;
        } else if (header_key == "Transfer-Encoding") {
            *transfer_encoding = header_value;
        }
    }
    advance(*reader, 2);
}

/*
 * Return true = read filename + read username
 * Return false = only read filename
 */
inline bool parse_path(std::string& path, char *temp_buffer, std::string *filename, std::string *username) {
    int index = 0;
    int i = 0;
    std::string key;

    while (i < path.size() && path[i] == '/') {
        i++;
    }

    // Read filename
    while (i < path.size() && !(path[i] == '?')) {
        temp_buffer[index++] = path[i++];
    }
    i++;
    temp_buffer[index] = '\0';
    *filename = std::string(temp_buffer);
    index = 0;

    // Read query parameters
    while (i < path.size()) {
        // Read key
        while (i < path.size() && !(path[i] == '=' || path[i] == '&')) {
            temp_buffer[index++] = path[i++];
        }
        i++;
        temp_buffer[index] = '\0';
        key = std::string(temp_buffer);
        index = 0;

        // Read value
        while (i < path.size() && path[i] != '&') {
            temp_buffer[index++] = path[i++];
        }
        i++;
        temp_buffer[index] = '\0';
        index = 0;

        // Get username
        if (key == "username") {
            *username = std::string(temp_buffer);
            return true;
        }
    }
    return false;
}

template<int SIZE>
inline void create_user_dir(std::string& username, tcp_writer<SIZE>& writer) {
    std::string user_dir = assemble_dir(username);
    int mkdir_error = mkdir(user_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    switch (mkdir_error) {
        case EACCES:
            log_error("This user does not have permission to create the user's sync_dir");
            break;
        case ENAMETOOLONG:
            log_error("Tried to create a sync_dir with a name too long (" log_value(user_dir) ")");
            break;
        case ENOENT:
            log_error("A component of the prefix path of the user's sync_dir does not exist");
            break;
        case ENOTDIR:
            log_error("A component of the prefix path of the user's sync_dir is not a directory");
            break;

        default:
            break;
    }

    switch (mkdir_error) {
        case ENOENT:
        case ENOTDIR:
        case ENAMETOOLONG:
        case EACCES:
            char message[] = "The only Transfer-Encoding allowed is chunked";
            uint64_t message_length = strlen(message);
            write(writer, "HTTP/1.1 400 Bad Request\r\n");
            write(writer, "Content-Length: ");
            write(writer, message_length);
            write(writer, "\r\n");
            write(writer, "\r\n");
            write(writer, message);
            flush(writer);
            exit(EXIT_FAILURE);
            break;
    }
}

template<int SIZE>
inline bool handle_get(std::string& username, std::string& filename, tcp_writer<SIZE>& writer, char *temp_buffer) {
    std::string file = assemble_filename(username, filename);
    FILE* fd = fopen(file.c_str(), "rb");

    if (fd == NULL || ferror(fd)) {
        write(writer, "HTTP/1.1 404 Not Found\r\n");
        write(writer, "Content-Length: 0\r\n");
        write(writer, "\r\n");
        flush(writer);
    } else {
        write(writer, "HTTP/1.1 200 Ok\r\n");
        write(writer, "Transfer-Encoding: chunked");
        write(writer, "\r\n");
        write(writer, "X-Username: ");
        write(writer, username);
        write(writer, "\r\n");
        write(writer, "\r\n");

        while (!feof(fd)) {
            uint64_t read_size = fread(temp_buffer, sizeof(char), SIZE, fd);
            write_hex(writer, read_size);
            write(writer, "\r\n");
            write(writer, (uint8_t*) temp_buffer, read_size);
            write(writer, "\r\n");
        }
        write(writer, "0");
        write(writer, "\r\n");
        write(writer, "\r\n");
        flush(writer);
    }

    return true;
}

template<int SIZE>
inline void handle_post(
        tcp_reader<SIZE>& reader,
        tcp_writer<SIZE>& writer,
        char *temp_buffer,
        std::string& username,
        std::string& filename,
        std::string& transfer_encoding,
        uint64_t content_length) {
    uint64_t index = 0;
    std::string temp;
    uint64_t chunk_length;

    if (transfer_encoding == "chunked") {
        log_debug("Reading file chunked");
        std::string file = assemble_filename(username, filename);
        FILE *fd = std::fopen(file.c_str(), "wb");
        while (true) {
            read_until(reader, temp_buffer, SIZE, index, peek(reader, 0) == '\r' && peek(reader, 1) == '\n', 2);
            temp = std::string(temp_buffer);

            std::istringstream iss(temp);
            // TODO - Didio: Check if `iss >> hex >> var` parses hex numbers that don't start with "0x" (this code assumes it does)
            iss >> std::hex >> chunk_length;
            log_error("Chunk size" log_value(chunk_length));

            if (chunk_length > 0) {
                for (uint64_t i = 0; i < content_length; i++) {
                    uint64_t j;
                    for (j = i; j < content_length && j-i < BUF_SIZE; j++) {
                        temp_buffer[j-i] = peek(reader, 0);
                        advance(reader, 1);
                    }
                    std::fwrite(temp_buffer, sizeof(uint8_t), j-i, fd);
                    i = j;
                }
            } else {
                std::fclose(fd);
                advance(reader, 2);
                break;
            }
        }

        write(writer, "HTTP/1.1 200 Ok\r\n");
        write(writer, "Content-Length: 0");
        write(writer, "\r\n");
        write(writer, "\r\n");
        flush(writer);
    } else if (transfer_encoding == "") {
        log_debug("Reading file content_length(" << content_length << ")");
        std::string file = assemble_filename(username, filename);
        FILE *fd = std::fopen(file.c_str(), "wb");
        for (uint64_t i = 0; i < content_length; i++) {
            uint64_t j;
            for (j = i; j < content_length && j-i < BUF_SIZE; j++) {
                temp_buffer[j-i] = peek(reader, 0);
                advance(reader, 1);
            }
            std::fwrite(temp_buffer, sizeof(uint8_t), j-i, fd);
            i = j;
        }
        std::fclose(fd);

        write(writer, "HTTP/1.1 200 Ok\r\n");
        write(writer, "Content-Length: 0");
        write(writer, "\r\n");
        write(writer, "\r\n");
        flush(writer);
    } else {
        char message[] = "The only Transfer-Encoding allowed is chunked";
        uint64_t message_length = strlen(message);
        write(writer, "HTTP/1.1 400 Bad Request\r\n");
        write(writer, "Content-Length: ");
        write(writer, message_length);
        write(writer, "\r\n");
        write(writer, "\r\n");
        write(writer, message);
        flush(writer);
    }
}

template<int SIZE>
void handle_delete(
        tcp_writer<SIZE>& writer,
        std::string& username,
        std::string& filename) {
    std::string file = assemble_filename(username, filename);
    if (FileManager::delete_file(std::string(filename))) {
        write(writer, "HTTP/1.1 200 Ok\r\n");
        write(writer, "Content-Length: 0");
        write(writer, "\r\n");
        write(writer, "\r\n");
        flush(writer);
    } else {
        char message[] = "The only Transfer-Encoding allowed is chunked";
        uint64_t message_length = strlen(message);
        write(writer, "HTTP/1.1 400 Bad Request\r\n");
        write(writer, "Content-Length: ");
        write(writer, message_length);
        write(writer, "\r\n");
        write(writer, "\r\n");
        write(writer, message);
        flush(writer);
    }
}

template<int SIZE>
void handle_list(
        tcp_writer<SIZE>& writer,
        std::string& username) {
            std::string dir = assemble_dir(username);
            std::vector<FileManager::file_description> files = FileManager::list_files(dir);

            write(writer, "HTTP/1.1 200 Ok\r\n");
            write(writer, "Transfer-Encoding: chunked");
            write(writer, "\r\n");
            write(writer, "X-Username: ");
            write(writer, username);
            write(writer, "\r\n");
            write(writer, "\r\n");

            for (auto &file : files) {
                // File chunk
                uint64_t length = file.first.size() + 2 + 8 + 2;
                write_hex(writer, length);
                write(writer, "\r\n");
                write(writer, file.first);
                write(writer, "\r\n");
                write(writer, file.second);
                write(writer, "\r\n");
                write(writer, "\r\n");
            }

            // End chunk
            write(writer, "0");
            write(writer, "\r\n");
            write(writer, "\r\n");
            flush(writer);
}

void handle_update() {
}

void handle_exit() {
}

void *http_thread(void *_arg) {
    // Read arguments
    struct thread_arguments *arg = (struct thread_arguments*) _arg;
    struct in_addr server_address = arg->server_address;
    in_port_t      server_port    = arg->server_port;
    struct in_addr client_address = arg->client_address;
    in_port_t      client_port    = arg->client_port;
    int            sockfd         = arg->sockfd;
    free(arg);

#define LOG_LABEL "[" << inet_ntoa(client_address) << ":" << client_port << "] "

    log_debug(LOG_LABEL "Starting a thread")

    // Reader
    tcp_reader<BUF_SIZE> reader = init_reader<BUF_SIZE>(sockfd);
    tcp_writer<BUF_SIZE> writer = init_writer<BUF_SIZE>(sockfd);

    // Buffer
    char temp_buffer[BUF_SIZE];
    int index;

    // HTTP request
    std::string method;
    std::string path;
    std::string version;
    uint64_t content_length;
    std::string transfer_encoding;

    // Path parameter
    std::string filename = "";
    std::string username = "";

    while (ready(reader)) {
        log_debug(LOG_LABEL "Starting to read request");

        parse_request_header<BUF_SIZE>(&reader, temp_buffer, &method, &path, &version, &content_length, &transfer_encoding);

        if (!parse_path(path, temp_buffer, &filename, &username)) {
            // If no username is provided, the connection is reset
            write(writer, "HTTP/1.1 401 Unauthorized\r\n");
            write(writer, "Connection: close\r\n");
            write(writer, "\r\n");
            flush(writer);
            break;
        }

        log_debug(LOG_LABEL "Got a `" << method << "` request from user `" << username << "` with file `" << filename << "`");

        create_user_dir(username, writer);

        if (method == "GET") {
            handle_get(username, filename, writer, temp_buffer);
        } else if (method == "POST") {
            handle_post(reader, writer, temp_buffer, username, filename, transfer_encoding, content_length);
        } else if (method == "DELETE") {
            handle_delete(writer, username, filename);
        } else if (method == "LIST") {
            handle_list(writer, username);
        } else if (method == "UPDATE") {
            handle_update();
        } else if (method == "EXIT") {
            handle_exit();
        } else {
            // If an unexpected method is requested, the connection is reset
            write(writer, "HTTP/1.1 405 Method Not Allowed\r\n");
            write(writer, "Connection: close\r\n");
            write(writer, "\r\n");
            flush(writer);
            break;
        }
        log_debug(LOG_LABEL "Finished request");
    }

    log_debug(LOG_LABEL "closing connection");

    close(sockfd);
    pthread_exit(nullptr);
    return nullptr;
}

bool tcp_dump_1(std::string ip, uint16_t port) {
    int sockfd;
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    // Initialize the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return false;
    }

    // Bind the socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return false;
    }

    // Setup listen queue
    listen(sockfd, 5);

    // Register server socket into the closeable connection list
    add_connection(sockfd);

    // Listen for clients
    while (true) {
        clilen = sizeof(struct sockaddr_in);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1) {
            break;
        }
        
        // TODO - Didio: initial handshake

        add_connection(newsockfd);

        // Construct thread arguments
        struct thread_arguments *arguments = (struct thread_arguments*) malloc(sizeof(struct thread_arguments));
        arguments->server_address = serv_addr.sin_addr;
        arguments->server_port = ntohs(serv_addr.sin_port);
        arguments->client_address = cli_addr.sin_addr;
        arguments->client_port = ntohs(cli_addr.sin_port);
        arguments->sockfd = newsockfd;

        // Create thread
        pthread_t thread;
        pthread_create(&thread, NULL, http_thread, (void*) arguments);
    }

    close(sockfd);
    return true; 
}

