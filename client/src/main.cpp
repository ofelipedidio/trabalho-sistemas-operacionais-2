// Test
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <unistd.h>
#ifdef TEST_MODE
#include "test.h"
#endif

#include "../include/network.h"
//#include "../include/fs_notify.h"
#include "../include/app.h"
#include "../include/cli.h"

#include <iostream>
#include <string>
#include <vector>

#include "../include/logger.h"
#include <cstdio>

int main(int argc, char **argv) {
    if (argc <= 3) {
        std::cerr << "ERRO! O app requer 3 arguments: " << argv[0] << " <username> <ip do servidor> <porta>" << std::endl;
        return 1;
    }

    // Parse input
    std::string username(argv[1]);
    std::string ip(argv[2]);
    std::string port_str(argv[3]);

    uint16_t port;
    std::istringstream iss(port_str);
    iss >> port;

    system(("mkdir -p sync_dir_" + username).c_str());

    std::cerr << "[Initialization] Starting App" << std::endl;
    App::init(username);

    // Start networking thread
    std::cerr << "[Initialization] Starting Network" << std::endl;
    if (!Network::init(ip, port)) {
        return EXIT_FAILURE;
    }
  
    std::cerr << "[Initialization] Starting FSNotify" << std::endl;
    FSNotify::init(username);

    std::cerr << "[Initialization] Starting Cli" << std::endl;
    Cli::init(username);

    return 0;
}

