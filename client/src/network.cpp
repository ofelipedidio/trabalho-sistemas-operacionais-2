#include <atomic>
#include <cstdint>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <queue>
#include <string>
#include <strings.h>
#include <vector>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../include/async_queue.h"
#include "../include/network.h"
#include "../include/connection.h"
#include "../include/protocol.h"
#include "../include/app.h"

namespace Network {
    pthread_t network_thread;
    std::string ip;
    uint16_t port;
    std::atomic<int> last_task_id;
    int sockfd;
    connection_t *connection;

    namespace __internal {
        AsyncQueue::async_queue<Network::network_task_t> task_queue;
        AsyncQueue::async_queue<Network::network_task_t> done_queue;

        /*
         * The networking thread function
         */
        void *thread_function(void* p) {
            network_task_t task;
            uint8_t status;
            uint8_t *buf;
            uint64_t length;

            while (true) {
                task = task_queue.pop();

                // TODO - Didio: Process the task
                switch (task.type) {
                    case TASK_LIST_FILES:
                        std::cerr << "[Network thread] Handling a list_files task" << std::endl;
                        if (request_list_files(connection, &status, &task.files)) {
                            std::cerr << "[Network thread] Succeded on the list_files task" << std::endl;
                        } else {
                            std::cerr << "[Network thread] Failed on the list_files task" << std::endl;
                        }
                        break;
                    case TASK_UPLOAD:
                        std::cerr << "[Network thread] Handling a upload task" << std::endl;
                        if (request_upload(connection, task.filename, task.content, task.content_length, &status)) {
                            std::cerr << "[Network thread] Succeded on the upload task" << std::endl;
                        } else {
                            std::cerr << "[Network thread] Failed on the upload task" << std::endl;
                        }
                        break;
                    case TASK_DOWNLOAD:
                        std::cerr << "[Network thread] Handling a download task" << std::endl;
                        if (request_download(connection, task.filename, &status, &buf, &length)) {
                            std::cerr << "[Network thread] Succeded on the download task" << std::endl;
                        } else {
                            std::cerr << "[Network thread] Failed on the download task" << std::endl;
                        }
                        break;
                    case TASK_DELETE:
                        std::cerr << "[Network thread] Handling a delete task" << std::endl;
                        if (request_delete(connection, task.filename, &status)) {
                            std::cerr << "[Network thread] Succeded on the delete task" << std::endl;
                        } else {
                            std::cerr << "[Network thread] Failed on the delete task" << std::endl;
                        }
                        break;
                    case TASK_EXIT:
                        std::cerr << "[Network thread] Handling a exit task" << std::endl;
                        if (request_exit(connection)) {
                            std::cerr << "[Network thread] Succeded on the exit task" << std::endl;
                        } else {
                            std::cerr << "[Network thread] Failed on the exit task" << std::endl;
                        }
                        break;
                }

                done_queue.push(task.task_id, task);
            }

            pthread_exit(nullptr);
        }
    }

    /*
     * Initializes the network subsystem and starts the network thread
     */
    bool init(std::string ip, uint16_t port) {
        Network::ip = ip;
        Network::port = port;

        __internal::task_queue = AsyncQueue::async_queue<network_task_t>();
        __internal::done_queue = AsyncQueue::async_queue<network_task_t>();

        /*
         * Create socket and connect to server
         */
        {
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_aton(ip.c_str(), &server_addr.sin_addr);
            bzero(&server_addr.sin_zero, 8);

            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                std::cerr << "ERROR: [Network init] Could not create the socket" << std::endl;
                return false;
            }

            int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
            if (connect_response < 0) {
                std::cerr << "ERROR: [Network init] Could not connect to the server" << std::endl;
                return false;
            }

            connection = conn_new(
                    server_addr.sin_addr,
                    ntohs(server_addr.sin_port),
                    sockfd);

            uint8_t status;
            if (!request_handshake(connection, App::get_username(), &status)) {
                std::cerr << "ERROR: [Network init] Failed to handshake" << std::endl;
                return false;
            }

            if (status != STATUS_SUCCESS) {
                std::cerr << "ERROR: [Network init] Too many connections to the server with the same username" << std::endl;
                return false;
            }
        }

        pthread_create(&network_thread, NULL, __internal::thread_function, NULL);
        return true;
    }

    /*
     * Adds an upload task to the task queue and returns the task id
     */
    // TODO : accept filename and path
    int upload_file(std::string username, std::string path) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_UPLOAD, 
                task_id, 
                username,
                path,
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                });
        return task_id;
    }

    /*
     * Adds a download task to the task queue and returns the task id
     */
    int download_file(std::string username, std::string path) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_DOWNLOAD, 
                task_id, 
                username,
                path,
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                });
        return task_id;
    }

    /*
     * Adds a delete task to the task queue and returns the task id
     */
    int delete_file(std::string username, std::string path) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_DELETE, 
                task_id, 
                username,
                path,
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                });
        return task_id;
    }

    /*
     * Adds a exit task to the task queue and locks the thread until the networking subsystem is properly shutdown
     */
    int client_exit(std::string username) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_EXIT, 
                task_id, 
                username,
                "",
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                });
        return task_id;
    }

    /*
     * Adds a list files task to the task queue and returns the task id
     */
    int list_files(std::string username) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_LIST_FILES, 
                task_id, 
                username,
                "",
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                });
        return task_id;
    }

    /*
     * Attempts to get a done task from the done queue. 
     * Returns true, if there is a task in the done queue. Returns false otherwise.
     */
    bool try_get_task(network_task_t *task) {
        std::optional<network_task_t> _done_task = __internal::done_queue.try_pop();
        if (_done_task) {
            network_task_t done_task = _done_task.value();
            task->type = done_task.type;
            task->task_id = done_task.task_id;
            task->username = done_task.username;
            task->filename = done_task.filename;
            task->content = done_task.content;
            task->files = done_task.files;
            return true;
        } else {
            return false;
        }
    }

    /*
     * Attempts to get a done task from the done queue.
     * Immediately returns, if there is a task in the done queue, waits otherwise.
     */
    void get_task(network_task_t *task) {
        network_task_t done_task = __internal::done_queue.pop();

        // Copy the data from the done task to task
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
    }
    
    bool try_get_task_by_id(int task_id, network_task_t *task) {
        std::optional<network_task_t> _done_task = __internal::done_queue.try_pop_by_id(task_id);
        if (_done_task) {
            network_task_t done_task = _done_task.value();
            task->type = done_task.type;
            task->task_id = done_task.task_id;
            task->username = done_task.username;
            task->filename = done_task.filename;
            task->content = done_task.content;
            task->files = done_task.files;
            return true;
        } else {
            return false;
        }
    }

    void get_task_by_id(int task_id, network_task_t *task) {
        network_task_t done_task = __internal::done_queue.pop_by_id(task_id);

        // Copy the data from the done task to task
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
    }
}

