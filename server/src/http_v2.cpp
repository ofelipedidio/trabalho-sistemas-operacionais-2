#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <ostream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include "../include/connection.h"
#include "../include/reader.h"
#include "../include/writer.h"

namespace http {
    typedef struct {
        connection_t *_connection;
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::vector<std::string>> query;
        std::unordered_map<std::string, std::vector<std::string>> headers;
    } request_t;

    typedef struct {
        connection_t *_connection;
        uint32_t status_code;
        std::string status_message;
        std::unordered_map<std::string, std::vector<std::string>> headers;
    } response_t;

    /*
     * Consumes characters from the connection and appends to the string until 
     * condition is met (the character used in the condition checking is consumed, 
     * but isn't appended to the string).
     *
     * If the stream closes before the condition is met, this function returns false, otherwise returns true.
     */
    bool read_until(connection_t *connection, std::string *string, bool (*condition)(char)) {
        // [Variable declarions]
        char c;
        ssize_t bytes_read;

        // [Processing]
        while (true) {
            bytes_read = read(connection->sockfd, &c, 1);
            if (bytes_read <= 0) {
                return false;
            }
            if (!condition(c)) {
                return true;
            }

            if (string != nullptr) {
                string->push_back(c);
            }
        }

        // One unnecessary return a day keeps the compiler warnings away :)
        return true;
    }

    /*
     * Ignored one character and returns whether is matches the input chracter.
     * If the stream is closed, this function returns false.
     */
    bool ignore_char(connection_t *connection, char c) {
        // [Variable declarions]
        char _c;
        ssize_t bytes_read;

        // [Processing]
        bytes_read = read(connection->sockfd, &_c, 1);
        if (bytes_read <= 0) {
            return false;
        }

        // [Return]
        return _c == c;
    }

    std::optional<request_t> parse_request(connection_t *connection) {
        // [Variable declarions]
        request_t request;
        char buf[1024];
        ssize_t buf_index;
        std::string query_key;
        std::string query_value;

        // [Initializing variables]
        request._connection = connection;
        request.method = "";
        request.path = "";
        request.query = std::unordered_map<std::string, std::vector<std::string>>();
        request.headers = std::unordered_map<std::string, std::vector<std::string>>();

        // [Processing]
        // Parse method
        read_until(connection, &request.method, [](char c) {return c != ' ';});
        std::cout << "Method: " << request.method << std::endl;

        // Parse path
        read_until(connection, &request.path, [](char c) {return c != ' ';});
        std::cout << "Path: " << request.method << std::endl;

        // Parse version
        read_until(connection, nullptr, [](char c) {return c != '\r';});
        ignore_char(connection, '\n');

        // [Return]
        return request;
    }
}
