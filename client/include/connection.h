#ifndef CONNECTION
#define CONNECTION

#include <cstdint>
#include <netinet/in.h>
#include "../include/reader.h"
#include "../include/writer.h"

typedef struct {
    uint64_t connection_id;
    struct in_addr server_address;
    in_port_t      server_port;
    struct in_addr client_address;
    in_port_t      client_port;
    int            sockfd;
    tcp_reader reader;
    tcp_writer writer;
} connection_t;

connection_t *conn_new(struct in_addr server_address, in_port_t server_port, struct in_addr client_address, in_port_t client_port, int sockfd);

void conn_free(connection_t *connection);

#endif // !CONNECTION

