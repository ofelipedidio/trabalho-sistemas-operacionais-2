#ifndef CONNECTION
#define CONNECTION

#include <netinet/in.h>
#include "../include/reader.h"
#include "../include/writer.h"

typedef struct {
    struct in_addr server_address;
    in_port_t      server_port;
    struct in_addr client_address;
    in_port_t      client_port;
    int            sockfd;
    tcp_reader<32> reader;
    tcp_writer<32> writer;
} connection_t;

connection_t *conn_new(struct in_addr server_address, in_port_t server_port, struct in_addr client_address, in_port_t client_port, int sockfd);

void conn_free(connection_t *connection);

#endif // !CONNECTION
