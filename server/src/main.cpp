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
} arguments_t;

int primary_init(arguments_t arguments) {
    // std::cerr << "[DEBUG] Starting backup" << std::endl;
    state_init(arguments.ip, arguments.port, primary);

    // std::cerr << "[DEBUG] Starting election thread" << std::endl;
    el_start_thread();
    // std::cerr << "[DEBUG] Starting communications thread" << std::endl;
    coms_thread_init();
    LOG_SYNC(std::cerr << "[DEBUG] Starting heartbeat thread" << std::endl);
    primary_heartbeat_thread_init();

    tcp_dump_1("0.0.0.0", arguments.port);

    // std::cerr << "[DEBUG] Idle" << std::endl;
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
    state_init(arguments.ip, arguments.port, backup);

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
        LOG_SYNC(std::cerr << "[DEBUG] [SETUP] Connecting to other server (" << std::hex << arguments.next_server_ip << std::dec << ":" << arguments.next_server_port << ")" << std::endl);
        connect_to_server(arguments.next_server_ip, COMMUNICATION_PORT(arguments.next_server_port), &conn);

        request = { .type = req_hello };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `other server` [1]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `other server` [2]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        request = { .type = req_get_primary };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `other server` [3]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `other server` [4]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        primary_ip = response.ip;
        primary_port = response.port;

        LOG_SYNC(std::cerr << "[DEBUG] [SETUP] Primary is " << std::hex << primary_ip << std::dec << ":" << primary_port << std::endl);

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
        LOG_SYNC(std::cerr << "[DEBUG] [SETUP] Connecting to primary (" << std::hex << primary_ip << std::dec << ":" << primary_port << ")" << std::endl);
        connect_to_server(primary_ip, COMMUNICATION_PORT(primary_port), &conn);

        request = { .type = req_hello };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `primary server` [1]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `primary server` [2]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        request = { .type = req_register };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `primary server` [3]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `primary server` [4]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        request = { .type = req_fetch_metadata };
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `primary server` [5]" << std::endl);
            close(conn->sockfd);
            conn_free(conn);
            init_done();
            return EXIT_FAILURE;
        }

        if (response.status != STATUS_OK) {
            set_should_stop(true);
            LOG_SYNC(std::cerr << "ERROR: Could not communicate with `primary server` [6]" << std::endl);
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


    init_done();

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
        if (argc != 4) {
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
        return primary_init(arguments);
    } else if (strcmp(argv[1], "b") == 0) {
        if (argc != 6) {
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
        return backup_init(arguments);
    } else {
        fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
        return EXIT_FAILURE;
    }

}

