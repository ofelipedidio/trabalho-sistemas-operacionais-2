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

    // std::cerr << "[DEBUG] Idle" << std::endl;
    while (true) { }

    return EXIT_FAILURE;
}

bool connect_to_server(uint32_t ip, uint16_t port, connection_t **out_connection) {
    connection_t *conn;
    {
        // Setup
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = { ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            LOG_SYNC(std::cerr << "ERROR: [Creating connection] Could not create the socket" << std::endl);
            return false;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1a] Could not connect to the server" << std::endl);
            close(sockfd);
            return false;
        }

        add_connection(sockfd);

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);
    }
    *out_connection = conn;

    return true;
}

bool heartbeat_handshake(server_t primary_server){
    connection_t *conn;
    {
        // Setup
        // std::cerr << "[DEBUG] Seting up sockets" << std::endl;
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(primary_server.port + 3);
        server_addr.sin_addr = { primary_server.ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        // std::cerr << "[DEBUG] Creating socket" << std::endl;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1a] Could not create the socket" << std::endl);
            return false;
        }

        // Connect
        // std::cerr << "[DEBUG] Connecting to primary" << std::endl;
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1a] Could not connect to the server" << std::endl);
            close(sockfd);
            return false;
        }

        int optval = 1;
        socklen_t optlen = sizeof(optval);
        if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
            perror("setsockopt()");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
            perror("getsockopt()");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        LOG_SYNC(std::cerr << "SO_KEEPALIVE is " << (optval ? "ON" : "OFF") << std::endl);

        // TODO - Kaiser: KEEP_ALIVE
        // TODO - Kaiser: botar um connection_t no state (talvez um mutex?).
        // TODO - Kaiser: Idealmente, essa conexão pega o metadata e conecta no primario dessa conexao, mas pode considerar que essa conexao sempre eh com o primario por enquanto

        add_connection(sockfd);

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        // std::cerr << "[DEBUG] Connecting connection object" << std::endl;
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);

        set_heartbeat_socket(conn);
        return true;
    }
}


bool initial_handshake(server_t primary_server) {
    // std::cerr << "[DEBUG] Starting handshake" << std::endl;
    // Connect to the elected server
    connection_t *conn;
    {
        // Setup
        // std::cerr << "[DEBUG] Seting up sockets" << std::endl;
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(primary_server.port + 2);
        server_addr.sin_addr = { primary_server.ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        // std::cerr << "[DEBUG] Creating socket" << std::endl;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            LOG_SYNC(std::cerr << "ERROR: [Election connection init 1a] Could not create the socket" << std::endl);
            return false;
        }

        // Connect
        // std::cerr << "[DEBUG] Connecting to primary" << std::endl;
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            std::cerr << "ERROR: [Election connection init 1a] Could not connect to the server" << std::endl;
            close(sockfd);
            return false;
        }

        int optval = 1;
        socklen_t optlen = sizeof(optval);
        if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
            perror("setsockopt()");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
            perror("getsockopt()");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        LOG_SYNC(std::cerr << "SO_KEEPALIVE is " << (optval ? "ON" : "OFF") << std::endl);

        // TODO - Kaiser: KEEP_ALIVE
        // TODO - Kaiser: botar um connection_t no state (talvez um mutex?).
        // TODO - Kaiser: Idealmente, essa conexão pega o metadata e conecta no primario dessa conexao, mas pode considerar que essa conexao sempre eh com o primario por enquanto

        add_connection(sockfd);

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        // std::cerr << "[DEBUG] Connecting connection object" << std::endl;
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);

        set_heartbeat_socket(conn);

    }

    // Execute hello request
    {
        // std::cerr << "[DEBUG] Sending hello message" << std::endl;
        request_t request = { .type = req_hello };
        response_t response;
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            // Catch network errors
            LOG_SYNC(std::cerr << "ERROR 100a" << std::endl);
            // std::cerr << "[DEBUG] Closing connection" << std::endl;
            close(conn->sockfd);
            // std::cerr << "[DEBUG] Freeing connection" << std::endl;
            conn_free(conn);
            return false;
        }

        // Catch logic errors
        if (response.status != 0) {
            LOG_SYNC(std::cerr << "ERROR 101a" << std::endl);
            // std::cerr << "[DEBUG] Closing connection" << std::endl;
            close(conn->sockfd);
            // std::cerr << "[DEBUG] Freeing connection" << std::endl;
            conn_free(conn);
            return false;
        }
    }

    // Execute register request
    {
        // std::cerr << "[DEBUG] Sending hello message" << std::endl;
        request_t request = { .type = req_register };
        response_t response;
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            // Catch network errors
            LOG_SYNC(std::cerr << "ERROR 100a" << std::endl);
            // std::cerr << "[DEBUG] Closing connection" << std::endl;
            close(conn->sockfd);
            // std::cerr << "[DEBUG] Freeing connection" << std::endl;
            conn_free(conn);
            return false;
        }

        // Catch logic errors
        if (response.status != 0) {
            LOG_SYNC(std::cerr << "ERROR 101a" << std::endl);
            // std::cerr << "[DEBUG] Closing connection" << std::endl;
            close(conn->sockfd);
            // std::cerr << "[DEBUG] Freeing connection" << std::endl;
            conn_free(conn);
            return false;
        }
    }

    // Execute fetch metadata
    {
        // std::cerr << "[DEBUG] Sending metadata message" << std::endl;
        request_t request = { .type = req_fetch_metadata };
        response_t response;
        if (!_coms_sync_execute_request(&conn->reader, &conn->writer, request, &response)) {
            // Catch network errors
            LOG_SYNC(std::cerr << "ERROR 100b" << std::endl);
            // std::cerr << "[DEBUG] Closing connection" << std::endl;
            close(conn->sockfd);
            // std::cerr << "[DEBUG] Freeing connection" << std::endl;
            conn_free(conn);
            return false;
        }

        // Catch logic errors
        if (response.status != 0) {
            LOG_SYNC(std::cerr << "ERROR 101b" << std::endl);
            // std::cerr << "[DEBUG] Closing connection" << std::endl;
            close(conn->sockfd);
            // std::cerr << "[DEBUG] Freeing connection" << std::endl;
            conn_free(conn);
            return false;
        }

        // Handle response
        // std::cerr << "[DEBUG] Acquiring metadata" << std::endl;
        metadata_t *metadata = acquire_metadata();
        // Critical section
        {
            // std::cerr << "[DEBUG] Updating metadata (len = " << response.metadata.servers.size() << ")" << std::endl;
            metadata->servers.clear();
            for (auto server : response.metadata.servers) {
                LOG_SYNC(std::cerr << "aaa: " << server << std::endl);
                metadata->servers.push_back(server);
            }
        }
        // std::cerr << "[DEBUG] Releasing metadata" << std::endl;
        release_metadata();
    }

    // Close the connection and free the allocated memory
    // std::cerr << "[DEBUG] Closing connection" << std::endl;
    //close(conn->sockfd);
    // std::cerr << "[DEBUG] Freeing connection" << std::endl;
    //conn_free(conn);
    return true;
}

int backup_init(arguments_t arguments) {
    // std::cerr << "[DEBUG] Starting backup" << std::endl;
    state_init(arguments.ip, arguments.port, backup);

    // std::cerr << "[DEBUG] Starting election thread" << std::endl;
    el_start_thread();
    // std::cerr << "[DEBUG] Starting communications thread" << std::endl;
    coms_thread_init();

    server_t primary_server = {
        .ip = arguments.next_server_ip,
        .port = arguments.next_server_port,
    };

    LOG_SYNC(std::cerr << "Primary server: " << primary_server << std::endl);

    // std::cerr << "[DEBUG] Calling handshake" << std::endl;
    if (!initial_handshake(primary_server)) {
        LOG_SYNC(std::cerr << "aaaa1" << std::endl);
    }

    if (!heartbeat_handshake(primary_server))
    {
        LOG_SYNC(std::cerr << "bbbb1" << std::endl);
    }
    

    sleep(2);

    acquire_metadata();
    release_metadata();

    //initiateElection();


    heartbeat_thread_init();
    // std::cerr << "[DEBUG] Waiting" << std::endl;
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

    if (argc < 2) {
        fprintf(stderr, "Uso correto: %s p <ip do servidor> <porta do servidor>\n", argv[0]);
            fprintf(stderr, "Uso correto: %s b <ip do servidor> <porta do servidor> <ip de outro servidor> <porta de outro servidor>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, sigpipe_handler);

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

