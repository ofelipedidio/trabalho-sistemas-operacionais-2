#include <unistd.h>
#include <vector>
#include <iostream>

#include "../include/closeable.h"
#include "../include/election.h"

std::vector<int> socket_fds;

void add_connection(int fd) {
    socket_fds.push_back(fd);
}

void close_all_connections() {
    while (socket_fds.size() > 0) {
        auto sockfd = socket_fds.back();
        socket_fds.pop_back();
        close(sockfd);
    }
}

void sigint_handler(int param) {
    std::cerr << "\n[SIGINT_HANDLER] Closing all sockets" << std::endl;
    close_all_connections();
    exit(1);
}

void sigpipe_handler(int signal_number) {
    std::cerr << "\n[SIGPIPE_HANDLER]Caught SIGPIPE signal (" << signal_number << ")." << std::endl;
    std::cout << "INICIA A ELEIÇÃO PFV \n";
      
    // TODO - Didio: close connection with primary
    //closeSocket();
    initiateElection();
}

