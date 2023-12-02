// Test
#include <cstdint>
#include <cstdlib>
#include <sstream>
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

    App::init(username);

    // Start networking thread
    if (!Network::init(ip, port)) {
        return EXIT_FAILURE;
    }

    Network::download_file(username, std::string("didio.txt"));
    
    FileManager::delete_file("asd");
    
    FSNotify::init(username);

    Cli::init(username);
  
    return 0;
}

