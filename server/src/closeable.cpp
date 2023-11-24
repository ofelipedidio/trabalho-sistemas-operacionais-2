#include <unistd.h>
#include <vector>

#include "../include/closeable.h"

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
