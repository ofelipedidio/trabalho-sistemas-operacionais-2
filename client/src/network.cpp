#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
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
#include "../include/debug.h"

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
            bool running;
            netfs::file_metadata_t metadata;
            file_event_t file_event;

            running = true;
            while (running) {
                std::optional<network_task_t> _task = task_queue.try_pop();
                if (_task.has_value()) {
                    task = _task.value();
                    switch (task.type) {
                        case TASK_LIST_FILES:
                            DEBUG(std::cerr << "[Network thread] Handling a list_files task" << std::endl;)
                            if (request_list_files(connection, &status, &task.files)) {
                                DEBUG(std::cerr << "[Network thread] Succeded on the list_files task" << std::endl;)
                                task.success = true;
                                done_queue.push(task.task_id, task);
                            } else {
                                DEBUG(std::cerr << "[Network thread] Failed on the list_files task" << std::endl;)
                                running = false;
                                task.success = false;
                                done_queue.push(task.task_id, task);
                            }
                            break;
                        case TASK_UPLOAD:
                            DEBUG(std::cerr << "[Network thread] Handling a upload task" << std::endl;)
                            if (netfs::read_file(task.path, &metadata)) {
                                if (request_upload(connection, task.filename, metadata.contents, metadata.length, &status)) {
                                    if (status == STATUS_SUCCESS) {
                                        DEBUG(std::cerr << "[Network thread] Succeded on the upload task" << std::endl;)
                                        task.success = true;
                                        done_queue.push(task.task_id, task);
                                    } else if (status == STATUS_FILE_NOT_FOUND) {
                                        DEBUG(std::cerr << "[Network thread] Server doesn't have file `" << task.filename << "`" << std::endl;)
                                        task.success = false;
                                        done_queue.push(task.task_id, task);
                                    } else {
                                        DEBUG(std::cerr << "ERROR: [Network] Server was unable to receive file `" << task.filename << "`" << std::endl;)
                                        task.success = false;
                                        done_queue.push(task.task_id, task);
                                    }
                                } else {
                                    DEBUG(std::cerr << "[Network thread] Failed on the upload task" << std::endl;)
                                    running = false;
                                    task.success = false;
                                    done_queue.push(task.task_id, task);
                                }
                            } else {
                                DEBUG(std::cerr << "ERROR: [NetFS] Could not read file `" << task.path << "`" << std::endl;)
                                running = false;
                                task.success = false;
                                done_queue.push(task.task_id, task);
                            }
                            break;
                        case TASK_DOWNLOAD:
                            DEBUG(std::cerr << "[Network thread] Handling a download task" << std::endl;)

                            if (request_download(connection, task.filename, &status, &buf, &length)) {
                                if (status == STATUS_SUCCESS) {
                                    if (netfs::write_file(task.path, buf, length)) {
                                        DEBUG(std::cerr << "[Network thread] Succeded on the download task" << std::endl;)
                                        task.success = true;
                                        done_queue.push(task.task_id, task);
                                    } else {
                                        DEBUG(std::cerr << "ERROR: [NetFS] Could not write file `" << task.path << "`" << std::endl;)
                                        task.success = false;
                                        done_queue.push(task.task_id, task);
                                    }
                                } else {
                                    DEBUG(std::cerr << "ERROR: [Network] Server was unable to send file `" << task.filename << "`" << std::endl;)
                                    task.success = false;
                                    done_queue.push(task.task_id, task);
                                }
                            } else {
                                DEBUG(std::cerr << "[Network thread] Failed on the download task" << std::endl;)
                                running = false;
                                task.success = false;
                                done_queue.push(task.task_id, task);
                            }
                            break;
                        case TASK_DELETE:
                            DEBUG(std::cerr << "[Network thread] Handling a delete task" << std::endl;)
                            if (request_delete(connection, task.filename, &status)) {
                                if (status == STATUS_SUCCESS) {
                                    DEBUG(std::cerr << "[Network thread] Succeded on the delete task" << std::endl;)
                                    task.success = true;
                                    done_queue.push(task.task_id, task);
                                } else {
                                    DEBUG(std::cerr << "ERROR: [Network] Server was unable to delete file `" << task.filename << "`" << std::endl;)
                                    task.success = false;
                                    done_queue.push(task.task_id, task);
                                }
                            } else {
                                DEBUG(std::cerr << "[Network thread] Failed on the delete task" << std::endl;)
                                running = false;
                                task.success = false;
                                done_queue.push(task.task_id, task);
                            }
                            break;
                        case TASK_EXIT:
                            DEBUG(std::cerr << "[Network thread] Handling a exit task" << std::endl;)
                            if (request_exit(connection)) {
                                DEBUG(std::cerr << "[Network thread] Succeded on the exit task" << std::endl;)
                                task.success = true;
                                done_queue.push(task.task_id, task);
                            } else {
                                DEBUG(std::cerr << "[Network thread] Failed on the exit task" << std::endl;)
                                running = false;
                                task.success = false;
                                done_queue.push(task.task_id, task);
                            }
                            break;
                        default:
                            running = false;
                            break;
                    }
                }

                if (request_update(connection, &status, &file_event)) {
                    if (status == STATUS_SUCCESS) {
                        switch (file_event.type) {
                            case event_file_modified:
                                DEBUG(std::cerr << "[net] [update] modified `" << file_event.filename << "`" << std::endl;)
                                if (request_download(connection, file_event.filename, &status, &buf, &length)) {
                                    DEBUG(std::cerr << "[net] [update] modified `" << file_event.filename << "` (s)" << std::endl;)
                                    App::network_modified(file_event.filename, buf, length);
                                } else {
                                    DEBUG(std::cerr << "[net] [update] modified `" << file_event.filename << "` (f)" << std::endl;)
                                }
                                break;
                            case event_file_deleted:
                                DEBUG(std::cerr << "[net] [update] deleted `" << file_event.filename << "`" << std::endl;)
                                App::network_deleted(file_event.filename);
                                break;
                        }
                    }
                } else {
                    running = false;
                }

                if (!_task.has_value()) {
                    sleep(1);
                }
            }

            std::cerr << "[Network] closed connection" << std::endl;
            exit(EXIT_FAILURE);
            return nullptr;
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
            DEBUG(std::cerr << "[Network] Starting socket" << std::endl;)
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

            DEBUG(std::cerr << "[Network] Connecting to the server" << std::endl;)
            int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
            if (connect_response < 0) {
                std::cerr << "ERROR: [Network init] Could not connect to the server" << std::endl;
                return false;
            }

            connection = conn_new(
                    server_addr.sin_addr,
                    ntohs(server_addr.sin_port),
                    sockfd);

            DEBUG(std::cerr << "[Network] Performing handshake" << std::endl;)
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
    int upload_file(std::string username, std::string filename, std::string path) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_UPLOAD, 
                task_id, 
                username,
                filename,
                path,
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                false,
                });
        return task_id;
    }

    /*
     * Adds a download task to the task queue and returns the task id
     */
    int download_file(std::string username, std::string filename, std::string path) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_DOWNLOAD, 
                task_id, 
                username,
                filename,
                path,
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                false,
                });
        return task_id;
    }

    /*
     * Adds a delete task to the task queue and returns the task id
     */
    int delete_file(std::string username, std::string filename) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
                TASK_DELETE, 
                task_id, 
                username,
                filename,
                filename,
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                false,
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
                "",
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                false,
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
                "",
                0,
                nullptr,
                std::vector<netfs::file_description_t>(),
                false,
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
            *task = done_task;
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
        *task = done_task;
    }

    bool try_get_task_by_id(int task_id, network_task_t *task) {
        std::optional<network_task_t> _done_task = __internal::done_queue.try_pop_by_id(task_id);
        if (_done_task) {
            network_task_t done_task = _done_task.value();
            *task = done_task;
            return true;
        } else {
            return false;
        }
    }

    void get_task_by_id(int task_id, network_task_t *task) {
        network_task_t done_task = __internal::done_queue.pop_by_id(task_id);
        *task = done_task;
    }
}

