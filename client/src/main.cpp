#include "../include/network.h"
#include "../include/fs_notify.h"
#include "../include/app.h"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    if (argc - 1 < 3) {
        std::cerr << "ERRO! O app requer 3 arguments: cliente <username> <ip do servidor> <porta>" << std::endl;
        return 0;
    }

    std::string username(argv[1]);
    std::string ip(argv[2]);
    std::string port(argv[3]);

    // TODO - Didio: Start networking thread
    // TODO - Didio: Start FS notify thread

    // Start app
    App::init(username);

    return 0;
}
