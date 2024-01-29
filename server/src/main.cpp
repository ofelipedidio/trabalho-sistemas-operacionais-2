#include <complex>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../include/state.h"
#include "../include/server.h"
#include "../include/closeable.h"
#include "../include/client.h"
#include "../include/election.h"
#include "../include/coms.h"
#include "../include/heartbeat.h"

typedef struct {
    /*******************************************************\
    * The ip and port on which the server will be listening *
    \*******************************************************/
    uint32_t ip;
    uint16_t port;

    /***********************************\
    * The type of the server on startup *
    \***********************************/
    server_type_t initial_type;

    /************************************************************\
    * The ip and port on which the server will look for metadata *
    \************************************************************/
    uint32_t next_server_ip;
    uint16_t next_server_port;

    uint32_t dns_ip;
    uint16_t dns_port;
} arguments_t;

int primary_init(arguments_t arguments) {
    state_init(arguments.ip, arguments.port, primary, arguments.dns_ip, arguments.dns_port);

    el_start_thread();
    coms_thread_init();
    primary_heartbeat_thread_init();

    server_t *current_server = get_current_server();
    {
        uint32_t dns_ip;
        uint16_t dns_port;
        connection_t *conn;

        get_dns(&dns_ip, &dns_port);
        if (!connect_to_server(dns_ip, dns_port, &conn)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [SETUP] Failed to connect to DNS\033[0m" << std::endl);
            exit(EXIT_FAILURE);
        }

        if (!write_u8(conn->writer, 20)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [SETUP] Failed to communicate to DNS (message type)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        if (!write_u32(conn->writer, current_server->ip)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [SETUP] Failed to communicate to DNS (ip)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        if (!write_u16(conn->writer, current_server->port)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [SETUP] Failed to communicate to DNS (port)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        if (!flush(conn->writer)) {
            LOG_SYNC(std::cerr << "\033[31mERROR: [SETUP] Failed to communicate to DNS (flush)\033[0m" << std::endl);
            close(conn->sockfd);
            exit(EXIT_FAILURE);
        }

        close(conn->sockfd);
    }

    tcp_dump_1("0.0.0.0", arguments.port);


    while (true) { }

    return EXIT_FAILURE;
}

#define FILE_PORT(port) (port)
#define ELECTION_PORT(port) ((port)+1)
#define COMMUNICATION_PORT(port) ((port)+2)
#define HEARTBEAT_PORT(port) ((port)+3)

int backup_init(arguments_t arguments) {
    uint32_t primary_ip;
    uint16_t primary_port;

    // [On startup]
    // 0. Start state
    state_init(arguments.ip, arguments.port, backup, arguments.dns_ip, arguments.dns_port);

    // 1. Start listeners
    el_start_thread();
    coms_thread_init();

    // 2. Connect to other server and get primary
    {
        // Variables
        connection_t *conn;
        request_t request;
        response_t response;

        // Execution
        LOG_SYNC(std::cerr << "[SETUP] Connecting to other server (" << std::hex << arguments.next_server_ip << std::dec << ":" << arguments.next_server_port << ")" << std::endl);
        connect_to_server(arguments.next_server_ip, COMMUNICATION_PORT(arguments.next_server_port), &conn);

        request = { .type = req_hello };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `other server` [1]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `other server` [2]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        request = { .type = req_get_primary };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `other server` [3]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `other server` [4]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        primary_ip = response.ip;
        primary_port = response.port;

        close(conn->sockfd);
        conn_free(conn);
    }

    // 3. Connect to primary and fetch metadata
    connection_t *conn;
    {
        // Variables
        request_t request;
        response_t response;

        server_t *primary_server = get_primary_server();

        // Execution
        LOG_SYNC(std::cerr << "[SETUP] Connecting to primary (" << std::hex << primary_ip << std::dec << ":" << primary_port << ")" << std::endl);
        connect_to_server(primary_ip, COMMUNICATION_PORT(primary_port), &conn);

        request = { .type = req_hello };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `primary server` [1]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `primary server` [2]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        request = { .type = req_register };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `primary server` [3]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `primary server` [4]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        request = { .type = req_fetch_metadata };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `primary server` [5]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: [SETUP] Could not communicate with `primary server` [6]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        {
            metadata_t *metadata = acquire_metadata();
            metadata->servers = response.metadata.servers;
            release_metadata();
        }
        close(conn->sockfd);
        conn_free(conn);
    }

    // 4. Start heartbeat
    heartbeat_thread_init();

    while (true) {}

    return EXIT_FAILURE;
}

bool check_ip(char *str, uint32_t *ip) {
    struct hostent *a = gethostbyname(str);
    if (a == NULL) {
        return false;
    }
    struct in_addr *addr = (struct in_addr*) a->h_addr_list[0];
    *ip = addr->s_addr;
    return true;
}

bool check_port(char *str, uint16_t *port) {
    uint64_t temp = 0;
    while (*str != '\0') {
        if ('0' <= *str && *str <= '9') {
            temp = 10 * temp + (*str - '0');
            if (temp > 0xFFFF) {
                return false;
            }
        } else {
            return false;
        }
        str++;
    }
    *port = temp;
    return true;
}

int main(int argc, char **argv) {
    arguments_t arguments;

    signal(SIGPIPE, sigpipe_handler);
    signal(SIGINT, sigint_handler);

    client_init();
    if (argc < 2) {
        fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
        return EXIT_FAILURE;
    }


    if (strcmp(argv[1], "p") == 0) {
        if (argc != 6) {
            fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_ip(argv[2], &arguments.ip)) {
            fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_port(argv[3], &arguments.port)) {
            fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_ip(argv[4], &arguments.dns_ip)) {
            fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_port(argv[5], &arguments.dns_port)) {
            fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return primary_init(arguments);
    } else if (strcmp(argv[1], "b") == 0) {
        if (argc != 8) {
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_ip(argv[2], &arguments.ip)) {
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_port(argv[3], &arguments.port)) {
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_ip(argv[4], &arguments.next_server_ip)) {
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_port(argv[5], &arguments.next_server_port)) {
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_ip(argv[6], &arguments.dns_ip)) {
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor> <ip do dns> <porta do dns>\n", argv[0]);
            return EXIT_FAILURE;
        }
        if (!check_port(argv[7], &arguments.dns_port)) {
            fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor> <ip do dns> <porta do dns>\n", argv[0]);
            return EXIT_FAILURE;
        }
        return backup_init(arguments);
    } else {
        fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
        fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
        return EXIT_FAILURE;
    }

}

