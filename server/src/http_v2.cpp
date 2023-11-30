#include <cstdint>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace http {
    typedef struct {
        struct in_addr server_address;
        in_port_t      server_port;
        struct in_addr client_address;
        in_port_t      client_port;
        int            sockfd;
    } connection_t;

    typedef struct {
        connection_t *_connection;
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> query;
        std::unordered_map<std::string, std::vector<std::string>> headers;
    } request_t;

    typedef struct {
        connection_t *_connection;
        uint32_t status_code;
        std::string status_message;
        std::unordered_map<std::string, std::vector<std::string>> headers;
    } response_t;

    std::optional<request_t> parse_request(connection_t *connection) {
        char buf[1024];
        ssize_t buf_index;

        request_t request;
        char c;
        ssize_t bytes_read;
        uint32_t index;
        uint32_t state;
        bool done;
        std::string query_key;
        std::string query_value;

#define STATE_METHOD 0
#define STATE_PATH 1
#define STATE_QUERY_KEY 2
#define STATE_QUERY_VALUE 3
#define STATE_VERSION 4
#define STATE_END 5

        state = STATE_METHOD;
        done = false;
        buf_index = 0;

        // State machine
        while (!done) {
            index = 0;
            bytes_read = read(connection->sockfd, &c, 1);
            if (bytes_read <= 0) {
                return {};
            }

            switch (state) {
                case STATE_METHOD:
                    switch (c) {
                        case ' ':
                            // Store method
                            buf[buf_index] = '\0';
                            request.method = std::string(buf);
                            buf_index = 0;
                            // Transition to next state
                            state = STATE_PATH;
                            break;
                        default:
                            buf[buf_index++] = c;
                            break;
                    }
                    break;
                case STATE_PATH:
                    switch (c) {
                        case ' ':
                            // Store path
                            buf[buf_index] = '\0';
                            request.path = std::string(buf);
                            buf_index = 0;
                            // Transition to next state
                            state = STATE_VERSION;
                            break;
                        case '?':
                            buf[buf_index] = '\0';
                            request.path = std::string(buf);
                            buf_index = 0;
                            // Transition to next state
                            state = STATE_QUERY_KEY;
                            break;
                        default:
                            buf[buf_index++] = c;
                            break;
                    }
                    break;
                case STATE_QUERY_KEY:
                    switch (c) {
                        case '=':
                            // Store key
                            buf[buf_index] = '\0';
                            query_key = std::string(buf);
                            buf_index = 0;
                            // Transition to next state
                            state = STATE_QUERY_VALUE;
                            break;
                        case '&':
                            // Store key
                            buf[buf_index] = '\0';
                            query_key = std::string(buf);
                            buf_index = 0;
                            // Empty value
                            query_value = std::string("");
                            // Store (key, value) on query
                            request.query.insert(query_key, query_value);
                            // Transition to next state
                            state = STATE_QUERY_KEY;
                            break;
                        case ' ':
                            // Store key
                            buf[buf_index] = '\0';
                            query_key = std::string(buf);
                            buf_index = 0;
                            // Empty value
                            query_value = std::string("");
                            // Store (key, value) on query
                            request.query.insert(query_key, query_value);
                            // Transition to next state
                            state = STATE_VERSION;
                            break;
                        default:
                            buf[buf_index++] = c;
                            break;
                    }
                    break;
                case STATE_QUERY_VALUE:
                    switch (c) {
                        case '&':
                            // Store value
                            buf[buf_index] = '\0';
                            query_value = std::string(buf);
                            buf_index = 0;
                            // Store (key, value) on query
                            request.query.insert(query_key, query_value);
                            // Transition to next state
                            state = STATE_QUERY_KEY;
                            break;
                        case ' ':
                            // Store value
                            buf[buf_index] = '\0';
                            query_value = std::string(buf);
                            buf_index = 0;
                            // Store (key, value) on query
                            request.query.insert(query_key, query_value);
                            // Transition to next state
                            state = STATE_VERSION;
                            break;
                        default:
                            buf[buf_index++] = c;
                            break;
                    }
                    break;
                case STATE_VERSION:
                    switch (c) {
                        case '\r':
                            // Transition to next state
                            state = STATE_END;
                            break;
                        default:
                            break;
                    }
                    break;
                case STATE_END:
                    switch (c) {
                        case '\n':
                            // Ended on a final state
                            done = true;
                            break;
                        default:
                            // Invalid transition
                            return {};
                    }
                    break;
            }
        }

        return request;
    }
}
