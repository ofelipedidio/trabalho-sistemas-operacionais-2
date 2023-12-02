#include "../include/connection.h"

#include "../include/reader.h"
#include "../include/writer.h"

connection_t *conn_new(
        struct in_addr server_address, 
        in_port_t server_port,
        struct in_addr client_address,
        in_port_t client_port,
        int sockfd) {
    connection_t *connection = (connection_t*) malloc(sizeof(connection_t));
    connection->server_address = server_address;
    connection->server_port = server_port;
    connection->client_address = client_address;
    connection->client_port = client_port;
    connection->sockfd = sockfd;
    connection->reader = init_reader<32>(sockfd);
    connection->writer = init_writer<32>(sockfd);
    return connection;
}

void conn_free(connection_t *connection) {
    free(connection);
}

