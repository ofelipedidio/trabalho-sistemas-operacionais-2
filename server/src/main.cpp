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

typedef struct {
    /************************************************\
    * The port on which the server will be listening *
    \************************************************/
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
    state_init();
    // Servidor de heartbeat + comunicação entre servidores
    coms_server_t server = coms_server_init(arguments.port+1);

    coms_get_metadata(server);

    // Servidor de arquivos
    tcp_dump_1("0.0.0.0", arguments.port);

    return EXIT_FAILURE;
    /*
    client_init();

    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, sigpipe_handler);
    uint16_t port = 4000;
    if (argc >= 2) {
        std::string s(argv[1]);
        std::istringstream iss(s);
        iss >> port;
    }

    tcp_dump_1("0.0.0.0", port);
    */
}

int backup_init(arguments_t arguments) {
    return EXIT_FAILURE;
    /*
    client_init();

    signal(SIGINT, sigint_handler);
    uint16_t port = 4000;
    if (argc >= 2) {
        std::string s(argv[1]);
        std::istringstream iss(s);
        iss >> port;
    }

    tcp_dump_1("0.0.0.0", port);
    */
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

    // This isn't a great way of getting the arguments, but it gets the job done
    if (argc >= 2) {
        if (check_port(argv[1], &arguments.port)) {
            if (argc >= 3) {
                if (strcmp(argv[2], "p") == 0) {
                    if (argc == 3) {
                        return primary_init(arguments);
                    } else {
                        fprintf(stderr, "ERRO: excesso de argumentos!\n");
                        fprintf(stderr, "\n");
                        fprintf(stderr, "Uso correto: %s <porta do servidor> p\n", argv[0]);
                        return EXIT_FAILURE;
                    }
                } else if (strcmp(argv[2], "b") == 0) {
                    if (argc >= 4) {
                        if (check_ip(argv[3], &arguments.next_server_ip)) {
                            fprintf(stderr, "addr1 = %x\n", arguments.next_server_ip);
                            if (argc >= 5) {
                                if (check_port(argv[4], &arguments.next_server_port)) {
                                    return backup_init(arguments);
                                } else {
                                    fprintf(stderr, "ERRO: \"%s\" nao eh um valor valido para a porta do primario!\n", argv[3]);
                                    fprintf(stderr, "\n");
                                    fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
                                    return EXIT_FAILURE;
                                }
                            }
                        } else {
                            fprintf(stderr, "ERRO: \"%s\" nao eh um valor valido para o IP do primario!\n", argv[3]);
                            fprintf(stderr, "\n");
                            fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
                            return EXIT_FAILURE;
                        }
                    } else {
                        fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
                        return EXIT_FAILURE;
                    }
                } else {
                    fprintf(stderr, "ERRO: \"%s\" nao eh um valor valido para o tipo de servidor, utilize 'p' ou 'b'!\n", argv[2]);
                    fprintf(stderr, "\n");
                    fprintf(stderr, "Uso correto: %s <porta do servidor> p\n", argv[0]);
                    fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
                    return EXIT_FAILURE;
                }
            } else {
                fprintf(stderr, "Uso correto: %s <porta do servidor> p\n", argv[0]);
                fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
            }
        } else {
            fprintf(stderr, "ERRO: \"%s\" nao eh um valor valido para a porta do servidor!\n", argv[1]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Uso correto: %s <porta do servidor> p\n", argv[0]);
            fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Uso correto: %s <porta do servidor> p\n", argv[0]);
        fprintf(stderr, "Uso correto: %s <porta do servidor> b <ip do primario> <porta do primario>\n", argv[0]);
        return EXIT_FAILURE;
    }
}
