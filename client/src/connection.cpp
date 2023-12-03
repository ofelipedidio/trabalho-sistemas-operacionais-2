#include "../include/connection.h"

#include <cstdint>

#include "../include/reader.h"
#include "../include/writer.h"

uint64_t client_id = 0;

connection_t *conn_new(
        struct in_addr server_address, 
        in_port_t server_port,
        int sockfd) {
    connection_t *connection = (connection_t*) malloc(sizeof(connection_t));
    connection->connection_id = client_id++;
    connection->server_address = server_address;
    connection->server_port = server_port;
    connection->sockfd = sockfd;
    connection->reader = init_reader(sockfd);
    connection->writer = init_writer(sockfd);
    return connection;
}

void conn_free(connection_t *connection) {
    free(connection);
}


