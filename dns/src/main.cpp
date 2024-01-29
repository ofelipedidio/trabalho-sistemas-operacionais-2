#include <iostream>
#include <signal.h>
#include <cstddef>
#include <netdb.h>

#include "../include/closeable.h"
#include "../include/state.h"
#include "../include/dns.h"

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

uint16_t port;
uint32_t primary_ip;
uint16_t primary_port;

int main(int argc, char **argv) {
    signal(SIGINT, sigint_handler);

    if (argc != 4) {
        std::cerr << "Uso correto: " << argv[0] << " <porta> <ip do primario> <porta do primario>" << std::endl;
        return EXIT_FAILURE;
    }

    if (!check_port(argv[1], &port)) {
        std::cerr << "Uso correto: " << argv[0] << " <porta> <ip do primario> <porta do primario>" << std::endl;
        return EXIT_FAILURE;
    }

    if (!check_ip(argv[2], &primary_ip)) {
        std::cerr << "Uso correto: " << argv[0] << " <porta> <ip do primario> <porta do primario>" << std::endl;
        return EXIT_FAILURE;
    }

    if (!check_port(argv[3], &primary_port)) {
        std::cerr << "Uso correto: " << argv[0] << " <porta> <ip do primario> <porta do primario>" << std::endl;
        return EXIT_FAILURE;
    }

    state_init(primary_ip, primary_port);

    dns_thread_init(&port);

    while (true) {}
}

