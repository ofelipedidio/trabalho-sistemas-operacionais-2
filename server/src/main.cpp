#include <iostream>
#include <sstream>
#include <signal.h>

#include "../include/server.h"
#include "../include/closeable.h"
#include "../include/client.h"

int main(int argc, char** argv) {
    client_init();

    signal(SIGINT, sigint_handler);
    uint16_t port = 4000;
    if (argc >= 2) {
        std::string s(argv[1]);
        std::istringstream iss(s);
        iss >> port;
    }

    tcp_dump_1("0.0.0.0", port);
    return 0;
}
