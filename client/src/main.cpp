// Test
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <system_error>
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

    std::cerr << "[Initialization] Starting App" << std::endl;
    App::init(username);

    // Start networking thread
    std::cerr << "[Initialization] Starting Network" << std::endl;
    if (!Network::init(ip, port)) {
        return EXIT_FAILURE;
    }

    system(("rm -rf sync_dir_" + username).c_str());
    system(("mkdir -p sync_dir_" + username).c_str());

    sleep(5);

    int task_id = Network::list_files(username);
    Network::network_task_t task;
    Network::get_task_by_id(task_id, &task);
    if (!task.success) {
        std::cerr << "[Initialization] Could not perform get_sync_dir" << std::endl;
        return EXIT_FAILURE;
    }

    for (auto file : task.files) {
        std::cerr << "[Initialization] Downloading file `" << file.filename << "`..." << std::endl;
        task_id = Network::download_file(username, file.filename, "sync_dir_" + username + "/" + file.filename);
        Network::get_task_by_id(task_id, &task);
        if (!task.success) {
            std::cerr << "[Initialization] Download failed!" << std::endl;
        }
    }

    std::cerr << "[Initialization] Starting FSNotify" << std::endl;
    FSNotify::init(username);

    std::cerr << "[Initialization] Starting Cli" << std::endl;
    Cli::init(username);

    return 0;
}

