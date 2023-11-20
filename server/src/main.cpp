// Configurar sistemas
#include "../include/network.h"
#include "../include/http_server.h"
#include <cstdint>
#include <sstream>
#include <string>

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
