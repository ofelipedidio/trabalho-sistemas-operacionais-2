// Configurar sistemas
#include "../include/network.h"
#include "../include/http_server.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../include/closeable.h"

void sigint_handler(int param) {
    std::cerr << "\n[SIGINT_HANDLER] Closing all sockets" << std::endl;
    close_all_connections();
    exit(1);
}

int main(int argc, char** argv) {
    uint16_t port = 54321;
    if (argc >= 2) {
        std::string s(argv[1]);
        std::istringstream iss(s);
        iss >> port;
    }

    tcp_dump_1("0.0.0.0", port);
    return 0;
}
