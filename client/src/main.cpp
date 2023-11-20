// Test
#include <cstdint>
#include <sstream>
#ifdef TEST_MODE
#include "test.h"
#endif

#include "../include/network.h"
//#include "../include/fs_notify.h"
#include "../include/app.h"
#include <iostream>
#include <string>
#include <vector>

#include "../include/logger.h"

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

    // Start networking thread
    if (!Network::init(ip, port)) {
        std::cerr << "ERROR: could not initialize the network submodule" << std::endl;
        return 1;
    }

    Network::download_file(username, std::string("didio.txt"));
    
    /*
    // Start FS notify thread
    // TODO - Didio: Arrumar
    std::string dir("sync_dir");// ***
    dir.append(username);       // Felipe K - por favor tem q arrumar coloquei só pra compilar
    FSNotify::init(dir);        // ***

    FileManager::delete_file("asd");
    */

    // TODO - Didio: Start app
    App::init(username);

    while (true) {
        
    }

    return 0;
}

