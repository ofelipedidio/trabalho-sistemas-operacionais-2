// Emite pacotes para o servicor

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include "../../client/include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define LINE_CHAR_COUNT 32

bool tcp_dump(std::string ip, uint16_t port) {
    char buf[BUF_SIZE];

    int sockfd, newsockfd, n;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;

    log_debug("Initializing socket");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        log_debug("Could not create a socket");
        return false;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    log_debug("Binding socket to port " << port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        log_debug("Could not bind the socket to port " << port);
        return false;
    }

    log_debug("Listening on socket");
    listen(sockfd, 5);

    while (true) {
        clilen = sizeof(struct sockaddr_in);
        log_debug("Waiting for client");
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd == -1) {
            log_debug("Could not accept a connection");
            return false;
        }
        log_debug("Got a connection from (" << inet_ntoa(cli_addr.sin_addr) << ":" << cli_addr.sin_port << ")");

        while (true) {
            log_debug("Trying to read");
            /* read from the socket */
            n = read(newsockfd, buffer, BUF_SIZE);
            if (n < 0) {
                log_debug("Could not read socket");
                log_debug("Closing connection");
                close(newsockfd);
                break;
            }
            log_debug("Read " << n << "bytes");
            if (n == 0) {
                log_debug("Closing connection");
                close(newsockfd);
                break;
            }
            
            for (int i = 0; i < n; i++) {
                std::cout << "Read:";
                int j;
                for (j = i; j < i + LINE_CHAR_COUNT && j < n; j++) {
                    if ((j-i) % 8 == 0) {
                        std::cout << " ";
                    }
                    std::cout << " " << std::hex << std::setfill('0') << std::setw(2) << (unsigned int) buffer[j];
                }
                std::cout << " | ";
                for (j = i; j < i + LINE_CHAR_COUNT && j < n; j++) {
                    if ((j-i) % 8 == 0) {
                        std::cout << " ";
                    }
                    switch ((char) buffer[j]) {
                        case 0:
                            std::cout << "\033[31m" << '0' << "\033[0m";
                            break;
                        case 1:
                            std::cout << "\033[31m" << '1' << "\033[0m";
                            break;
                        case 2:
                            std::cout << "\033[31m" << '2' << "\033[0m";
                            break;
                        case 3:
                            std::cout << "\033[31m" << '3' << "\033[0m";
                            break;
                        case 4:
                            std::cout << "\033[31m" << '4' << "\033[0m";
                            break;
                        case 5:
                            std::cout << "\033[31m" << '5' << "\033[0m";
                            break;
                        case 6:
                            std::cout << "\033[31m" << '6' << "\033[0m";
                            break;
                        case 7:
                            std::cout << "\033[31m" << '7' << "\033[0m";
                            break;
                        case 8:
                            std::cout << "\033[31m" << '8' << "\033[0m";
                            break;
                        case 11:
                            std::cout << "\033[31m" << 'b' << "\033[0m";
                            break;
                        case 12:
                            std::cout << "\033[31m" << 'c' << "\033[0m";
                            break;
                        case 14:
                            std::cout << "\033[31m" << 'e' << "\033[0m";
                            break;
                        case 15:
                            std::cout << "\033[31m" << 'f' << "\033[0m";
                            break;
                        case '\n':
                            std::cout << "\033[31m" << 'n' << "\033[0m";
                            break;
                        case '\t':
                            std::cout << "\033[31m" << 't' << "\033[0m";
                            break;
                        case '\r':
                            std::cout << "\033[31m" << 'r' << "\033[0m";
                            break;

                        default:
                            std::cout << (char) buffer[j];
                            break;
                    }
                }
                i = j;
                std::cout << std::endl;
            }
        }
    }

    close(sockfd);
    return true; 
}
