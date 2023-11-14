// Test
#ifdef TEST_MODE
#include "test.h"
#endif

#include "../include/network.h"
#include "../include/fs_notify.h"
#include "../include/app.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    if (argc <= 3) {
        std::cerr << "ERRO! O app requer 3 arguments: " << argv[0] << " <username> <ip do servidor> <porta>" << std::endl;
        return 1;
    }

    std::string username(argv[1]);
    std::string ip(argv[2]);
    std::string port(argv[3]);

    // Start networking thread
    Network::init(ip, port);

    // Start FS notify thread
    FSNotify::init();

    // TODO - Didio: Start app

    App::init(username);

    return 0;
}

