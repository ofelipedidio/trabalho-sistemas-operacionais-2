#include "../include/coms.h"
#include <asm-generic/errno-base.h>
#include <cstdlib>
#include <vector>
#include <iterator> // Add this include directive
#include <algorithm>
#include <iostream>
#include <asm-generic/errno.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <sys/stat.h>
#include <pthread.h>

#include "../include/closeable.h"
#include "../include/election.h"
#include "../include/state.h"


#define coms_exec(expr) \
    if (!expr) { \
        return false; \
    }

bool _coms_sync_execute_request(tcp_reader *reader, tcp_writer *writer, request_t request, response_t *out_response) {
    server_t *current_server = get_current_server();

    uint64_t metadata_length;
    uint64_t primary_index;
    server_t in_server;


    switch (request.type) {
        case req_transaction_start:
            coms_exec(write_u8(*writer, 12));
            break;
        case req_transaction_end:
            coms_exec(write_u8(*writer, 13));
            break;
        case req_fetch_metadata:
            coms_exec(write_u8(*writer, 14));
            break;
        case req_hello:
            coms_exec(write_u8(*writer, 15));
            break;
        case req_register:
            coms_exec(write_u8(*writer, 16));
            coms_exec(write_u16(*writer, current_server->port));
            break;
    }

    coms_exec(flush(*writer));

    switch (request.type) {
        case req_transaction_start:
            coms_exec(read_u16(*reader, &out_response->status));
            break;

        case req_transaction_end:
            coms_exec(read_u16(*reader, &out_response->status));
            break;

        case req_fetch_metadata:
            out_response->metadata = { .servers = std::vector<server_t>() };
            coms_exec(read_u16(*reader, &out_response->status));
            if (out_response->status == STATUS_OK) {
                coms_exec(read_u64(*reader, &metadata_length));
                coms_exec(read_u64(*reader, &primary_index));
                for (uint64_t i = 0; i < metadata_length; i++) {
                    coms_exec(read_u32(*reader, &in_server.ip));
                    coms_exec(read_u16(*reader, &in_server.port));
                    in_server.server_type = (i == primary_index) ? primary : backup;
                    out_response->metadata.servers.push_back(in_server);
                }
            }
            break;

        case req_hello:
            coms_exec(read_u16(*reader, &out_response->status));
            break;

        case req_register:
            coms_exec(read_u16(*reader, &out_response->status));
            break;
    }

    return true;
}

bool coms_handle_request(tcp_reader *reader, tcp_writer *writer, server_t server) {
    LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request" << std::endl);

    uint8_t request_type;
    coms_exec(read_u8(*reader, &request_type));
    switch (request_type) {

        case 12:
            // req_transaction_start
            LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request (req_transaction_start)" << std::endl);
            coms_exec(write_u16(*writer, STATUS_NOT_IMPLEMENTED));
            coms_exec(flush(*writer));
            break;

        case 13:
            // req_transaction_end
            LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request (req_transaction_end)" << std::endl);
            coms_exec(write_u16(*writer, STATUS_NOT_IMPLEMENTED));
            coms_exec(flush(*writer));
            break;

        case 14:
            // req_fetch_metadata
            {
                LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request (req_fetch_metadata)" << std::endl);
                coms_exec(write_u16(*writer, STATUS_OK));
                metadata_t *metadata = acquire_metadata();
                uint64_t length = metadata->servers.size();
                uint64_t primary_index = length;
                for (uint64_t i = 0; i < length; i++) {
                    if (metadata->servers[i].server_type == primary) {
                        primary_index = i;
                        break;
                    }
                }
                coms_exec(write_u64(*writer, length));
                coms_exec(write_u64(*writer, primary_index));
                for (uint64_t i = 0; i < length; i++) {
                    coms_exec(write_u32(*writer, metadata->servers[i].ip));
                    coms_exec(write_u16(*writer, metadata->servers[i].port));
                }
                release_metadata();
                coms_exec(flush(*writer));
            }
            break;

        case 15:
            // req_hello
            {
                LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request (req_hello)" << std::endl);
                coms_exec(write_u16(*writer, STATUS_OK));
                coms_exec(flush(*writer));
            }
            break;

        case 16:
            // req_register
            {
                LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request (req_register)" << std::endl);
                coms_exec(read_u16(*reader, &server.port));
                {
                    metadata_t *metadata = acquire_metadata();
                    metadata->servers.push_back(server);
                    release_metadata();
                }
                coms_exec(write_u16(*writer, STATUS_OK));
                coms_exec(flush(*writer));
            }
            break;

        default:
            LOG_SYNC(std::cerr << "[DEBUG] [COMS] Handling request (invalid signature [" << ((int) request_type) << "])" << std::endl);
            coms_exec(write_u16(*writer, STATUS_INVALID_REQUEST_TYPE));
            coms_exec(flush(*writer));
            LOG_SYNC(std::cerr << "[DEBUG] [COMMS] Done" << std::endl);
            return false;
    }
    LOG_SYNC(std::cerr << "[DEBUG] [COMMS] Done" << std::endl);

    return true;
}

/*
 * Calles `coms_handle_request` in a loop until the connection fails
 */
void coms_handle_connection(connection_t *connection, server_t server) {
    LOG_SYNC(std::cerr << "[DEBUG] [COMS] Received a connection from " << server << std::endl);

    bool error_occurred = false;

    while (!(error_occurred || should_stop())) {
        // std::cerr << "[DEBUG] [Coms Thread] Handling message (e = " << (error_occurred ? "true" : "false") << ")" << std::endl;
        if (!coms_handle_request(&connection->reader, &connection->writer, server)) {
            error_occurred = true;
            break;
        }
    }
}

std::string errno_print(int err) {
    switch (err) {
        case 1:
            return "EPERM (Operation not permitted)";
        case 2:
            return "ENOENT (No such file or directory)";
        case 3:
            return "ESRCH (No such process)";
        case 4:
            return "EINTR (Interrupted system call)";
        case 5:
            return "EIO (I/O error)";
        case 6:
            return "ENXIO (No such device or address)";
        case 7:
            return "E2BIG (Argument list too long)";
        case 8:
            return "ENOEXEC (Exec format error)";
        case 9:
            return "EBADF (Bad file number)";
        case 10:
            return "ECHILD (No child processes)";
        case 11:
            return "EAGAIN (Try again)";
        case 12:
            return "ENOMEM (Out of memory)";
        case 13:
            return "EACCES (Permission denied)";
        case 14:
            return "EFAULT (Bad address)";
        case 15:
            return "ENOTBLK (Block device required)";
        case 16:
            return "EBUSY (Device or resource busy)";
        case 17:
            return "EEXIST (File exists)";
        case 18:
            return "EXDEV (Cross-device link)";
        case 19:
            return "ENODEV (No such device)";
        case 20:
            return "ENOTDIR (Not a directory)";
        case 21:
            return "EISDIR (Is a directory)";
        case 22:
            return "EINVAL (Invalid argument)";
        case 23:
            return "ENFILE (File table overflow)";
        case 24:
            return "EMFILE (Too many open files)";
        case 25:
            return "ENOTTY (Not a typewriter)";
        case 26:
            return "ETXTBSY (Text file busy)";
        case 27:
            return "EFBIG (File too large)";
        case 28:
            return "ENOSPC (No space left on device)";
        case 29:
            return "ESPIPE (Illegal seek)";
        case 30:
            return "EROFS (Read-only file system)";
        case 31:
            return "EMLINK (Too many links)";
        case 32:
            return "EPIPE (Broken pipe)";
        case 33:
            return "EDOM (Math argument out of domain of func)";
        case 34:
            return "ERANGE (Math result not representable)";
        case 35:
            return "EDEADLK (Resource deadlock would occur)";
        case 36:
            return "ENAMETOOLONG (File name too long)";
        case 37:
            return "ENOLCK (No record locks available)";
        case 38:
            return "ENOSYS (Invalid system call number)";
        case 39:
            return "ENOTEMPTY (Directory not empty)";
        case 40:
            return "ELOOP (Too many symbolic links encountered)";
        case 42:
            return "ENOMSG (No message of desired type)";
        case 43:
            return "EIDRM (Identifier removed)";
        case 44:
            return "ECHRNG (Channel number out of range)";
        case 45:
            return "EL2NSYNC (Level 2 not synchronized)";
        case 46:
            return "EL3HLT (Level 3 halted)";
        case 47:
            return "EL3RST (Level 3 reset)";
        case 48:
            return "ELNRNG (Link number out of range)";
        case 49:
            return "EUNATCH (Protocol driver not attached)";
        case 50:
            return "ENOCSI (No CSI structure available)";
        case 51:
            return "EL2HLT (Level 2 halted)";
        case 52:
            return "EBADE (Invalid exchange)";
        case 53:
            return "EBADR (Invalid request descriptor)";
        case 54:
            return "EXFULL (Exchange full)";
        case 55:
            return "ENOANO (No anode)";
        case 56:
            return "EBADRQC (Invalid request code)";
        case 57:
            return "EBADSLT (Invalid slot)";
        case 59:
            return "EBFONT (Bad font file format)";
        case 60:
            return "ENOSTR (Device not a stream)";
        case 61:
            return "ENODATA (No data available)";
        case 62:
            return "ETIME (Timer expired)";
        case 63:
            return "ENOSR (Out of streams resources)";
        case 64:
            return "ENONET (Machine is not on the network)";
        case 65:
            return "ENOPKG (Package not installed)";
        case 66:
            return "EREMOTE (Object is remote)";
        case 67:
            return "ENOLINK (Link has been severed)";
        case 68:
            return "EADV (Advertise error)";
        case 69:
            return "ESRMNT (Srmount error)";
        case 70:
            return "ECOMM (Communication error on send)";
        case 71:
            return "EPROTO (Protocol error)";
        case 72:
            return "EMULTIHOP (Multihop attempted)";
        case 73:
            return "EDOTDOT (RFS specific error)";
        case 74:
            return "EBADMSG (Not a data message)";
        case 75:
            return "EOVERFLOW (Value too large for defined data type)";
        case 76:
            return "ENOTUNIQ (Name not unique on network)";
        case 77:
            return "EBADFD (File descriptor in bad state)";
        case 78:
            return "EREMCHG (Remote address changed)";
        case 79:
            return "ELIBACC (Can not access a needed shared library)";
        case 80:
            return "ELIBBAD (Accessing a corrupted shared library)";
        case 81:
            return "ELIBSCN (.lib section in a.out corrupted)";
        case 82:
            return "ELIBMAX (Attempting to link in too many shared libraries)";
        case 83:
            return "ELIBEXEC (Cannot exec a shared library directly)";
        case 84:
            return "EILSEQ (Illegal byte sequence)";
        case 85:
            return "ERESTART (Interrupted system call should be restarted)";
        case 86:
            return "ESTRPIPE (Streams pipe error)";
        case 87:
            return "EUSERS (Too many users)";
        case 88:
            return "ENOTSOCK (Socket operation on non-socket)";
        case 89:
            return "EDESTADDRREQ (Destination address required)";
        case 90:
            return "EMSGSIZE (Message too long)";
        case 91:
            return "EPROTOTYPE (Protocol wrong type for socket)";
        case 92:
            return "ENOPROTOOPT (Protocol not available)";
        case 93:
            return "EPROTONOSUPPORT (Protocol not supported)";
        case 94:
            return "ESOCKTNOSUPPORT (Socket type not supported)";
        case 95:
            return "EOPNOTSUPP (Operation not supported on transport endpoint)";
        case 96:
            return "EPFNOSUPPORT (Protocol family not supported)";
        case 97:
            return "EAFNOSUPPORT (Address family not supported by protocol)";
        case 98:
            return "EADDRINUSE (Address already in use)";
        case 99:
            return "EADDRNOTAVAIL (Cannot assign requested address)";
        case 100:
            return "ENETDOWN (Network is down)";
        case 101:
            return "ENETUNREACH (Network is unreachable)";
        case 102:
            return "ENETRESET (Network dropped connection because of reset)";
        case 103:
            return "ECONNABORTED (Software caused connection abort)";
        case 104:
            return "ECONNRESET (Connection reset by peer)";
        case 105:
            return "ENOBUFS (No buffer space available)";
        case 106:
            return "EISCONN (Transport endpoint is already connected)";
        case 107:
            return "ENOTCONN (Transport endpoint is not connected)";
        case 108:
            return "ESHUTDOWN (Cannot send after transport endpoint shutdown)";
        case 109:
            return "ETOOMANYREFS (Too many references: cannot splice)";
        case 110:
            return "ETIMEDOUT (Connection timed out)";
        case 111:
            return "ECONNREFUSED (Connection refused)";
        case 112:
            return "EHOSTDOWN (Host is down)";
        case 113:
            return "EHOSTUNREACH (No route to host)";
        case 114:
            return "EALREADY (Operation already in progress)";
        case 115:
            return "EINPROGRESS (Operation now in progress)";
        case 116:
            return "ESTALE (Stale file handle)";
        case 117:
            return "EUCLEAN (Structure needs cleaning)";
        case 118:
            return "ENOTNAM (Not a XENIX named type file)";
        case 119:
            return "ENAVAIL (No XENIX semaphores available)";
        case 120:
            return "EISNAM (Is a named type file)";
        case 121:
            return "EREMOTEIO (Remote I/O error)";
        case 122:
            return "EDQUOT (Quota exceeded)";
        case 123:
            return "ENOMEDIUM (No medium found)";
        case 124:
            return "EMEDIUMTYPE (Wrong medium type)";
        case 125:
            return "ECANCELED (Operation Canceled)";
        case 126:
            return "ENOKEY (Required key not available)";
        case 127:
            return "EKEYEXPIRED (Key has expired)";
        case 128:
            return "EKEYREVOKED (Key has been revoked)";
        case 129:
            return "EKEYREJECTED (Key was rejected by service)";
        case 130:
            return "EOWNERDEAD (Owner died)";
        case 131:
            return "ENOTRECOVERABLE (State not recoverable)";
        case 132:
            return "ERFKILL (Operation not possible due to RF-kill)";
        case 133:
            return "EHWPOISON (Memory page has hardware error)";
        default:
            return std::to_string(err) + " (unknown)";
    }
}

void *coms_listener_thread(void *args) {
    LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = THREAD_START" << std::endl;);

    // Variables
    struct sockaddr_in server_address;
    int listen_sockfd;
    int connection_sockfd;
    socklen_t client_length;
    struct sockaddr_in client_address;

    // Configure socket to listen
    {
        server_t *current_server = get_current_server();

        // Initialize the socket
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(std::cerr << "ERROR: [COMMUNICATIONS] Call to socket returned -1 (errno = " << errno_print(errno) << ")" << std::endl);
            exit(1);
        }
        LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = SOCKET_CREATED" << std::endl;);

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(current_server->port+2);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        // std::cerr << "[DEBUG] [Coms] Binding socket" << std::endl;
        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(std::cerr << "ERROR: [COMMUNICATIONS] Call to bind returned -1 (errno = " << errno_print(errno) << ")" << std::endl);
            exit(1);
        }

        LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = SOCKET_BOUND" << std::endl;);

        // Setup listen queue
        listen(listen_sockfd, 15);

        LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = SOCKET_LISTENING" << std::endl;);

        // Register server socket into the closeable connection list
        add_connection(listen_sockfd);
    }

    // Handle clients
    {
        client_length = sizeof(struct sockaddr_in);

        while (!should_stop()) {
            LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = WAITING_FOR_CLIENT" << std::endl;);

            // Accept connecting clients and break on 
            connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
            if (connection_sockfd == -1) {
                LOG_SYNC(std::cerr << "ERROR: [COMMUNICATIONS] Call to accept returned -1 (errno = " << errno_print(errno) << ")" << std::endl);
                break;
            }

            add_connection(connection_sockfd);

            LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = HANDLING_CLIENT" << std::endl;);

            connection_t *connection = conn_new(
                    server_address.sin_addr,
                    ntohs(server_address.sin_port),
                    client_address.sin_addr,
                    ntohs(client_address.sin_port),
                    connection_sockfd);

            server_t server = {
                .ip = connection->client_address.s_addr,
                .port = connection->client_port,
                .server_type = backup,
            };

            coms_handle_connection(connection, server);

            LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = CLIENT_HANDLED" << std::endl;);
        }
    }

    // Thread teardown
    {
        LOG_SYNC(std::cerr << "[DEBUG] [COMMUNICATIONS] State = DONE" << std::endl;);

        close(listen_sockfd);
        pthread_exit(NULL);
        return NULL; 
    }
}

bool coms_thread_init() {
    // Create the thread
    // std::cerr << "[DEBUG] [Coms] Creating thread" << std::endl;
    pthread_t thread;
    pthread_create(&thread, NULL, coms_listener_thread, NULL);
    return true;
}

void *heartbeat_listener_thread(void *args) {
    struct sockaddr_in server_address;
    int listen_sockfd;

    {
        server_t *current_server = get_current_server();

        // Initialize the socket
        // std::cerr << "[DEBUG] [Coms] Creating socket" << std::endl;
        listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd == -1) {
            LOG_SYNC(std::cerr << "bbbbbbbbbbbbbbbbbbbbb1" << std::endl);
            exit(1);
        }

        // Bind the socket
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(current_server->port+3);
        server_address.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_address.sin_zero), 8);

        // std::cerr << "[DEBUG] [Coms] Binding socket" << std::endl;
        if (bind(listen_sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            LOG_SYNC(std::cerr << "bbbbbbbbbbbbbbbbbbbbb2" << std::endl);
            exit(1);
        }

        // Setup listen queue
        // std::cerr << "[DEBUG] [Coms] Listening on socket" << std::endl;
        listen(listen_sockfd, 15);

        // Register server socket into the closeable connection list
        // std::cerr << "[DEBUG] [Coms] Storing socket" << std::endl;
        add_connection(listen_sockfd);
    }

    int connection_sockfd;
    struct sockaddr_in client_address;
    socklen_t client_length;

    // Listen for clients
    while (!should_stop()) {
        // std::cerr << "[DEBUG] [Coms Thread] Waiting for client" << std::endl;
        client_length = sizeof(struct sockaddr_in);
        connection_sockfd = accept(listen_sockfd, (struct sockaddr *) &client_address, &client_length);
        if (connection_sockfd == -1) {
            break;
        }
        // std::cerr << "[DEBUG] [Coms Thread] Handling client" << std::endl;
        add_connection(connection_sockfd);

        connection_t *connection = conn_new(
                server_address.sin_addr,
                ntohs(server_address.sin_port),
                client_address.sin_addr,
                ntohs(client_address.sin_port),
                connection_sockfd);

        // coms_handle_connection(connection,);
        std::vector <connection_t*> *coneccoes = get_heartbeat_connections();
        (*coneccoes).push_back(connection);
        release_heartbeat_connections();
    }

    close(listen_sockfd);
    pthread_exit(NULL);
    return NULL; 
}

void *heartbeat_writer_thread(void *args) {
    while (!should_stop())
    {
        connection_t *conn;
        {

            server_t *primary_server = get_primary_server();
            server_t *current_server = get_current_server();

            if (server_eq(primary_server,current_server)){
                return NULL;
            }
            // Setup
            // std::cerr << "[DEBUG] Seting up sockets" << std::endl;
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons((*primary_server).port + 3);
            server_addr.sin_addr = { (*primary_server).ip };
            bzero(&server_addr.sin_zero, 8);

            // Create socket
            // std::cerr << "[DEBUG] Creating socket" << std::endl;
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                std::cerr << "ERROR: [Election connection init 1a] Could not create the socket" << std::endl;
                return NULL;
            }

            // Connect
            // std::cerr << "[DEBUG] Connecting to primary" << std::endl;
            int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
            if (connect_response < 0) {
                std::cerr << "ERROR: [Election connection init 1a] Could not connect to the server" << std::endl;
                close(sockfd);
                return NULL;
            }

            int optval = 1;
            socklen_t optlen = sizeof(optval);
            if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
                perror("setsockopt()");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
                perror("getsockopt()");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            std::cerr << "SO_KEEPALIVE is " << (optval ? "ON" : "OFF") << std::endl;

            add_connection(sockfd);

            // Create connection object
            // INTERNAL: the client's fields are set to the server's fields on purpose
            // TODO - Didio: figure out how to get client's IP and port
            // std::cerr << "[DEBUG] Connecting connection object" << std::endl;
            conn = conn_new(
                    server_addr.sin_addr,
                    ntohs(server_addr.sin_port),
                    server_addr.sin_addr,
                    ntohs(server_addr.sin_port),
                    sockfd);

            set_heartbeat_socket(conn);
        }
        
        while (!should_stop())
        {
            sleep(2);
            if (!write_u8(conn->writer,0) || !flush(conn->writer)){
                std::cerr << "[Heartbeat] connection reset" << std::endl;
                initiateElection();
                break;
            }
            //std::cerr << "[Heartbeat] heartbeat sent from backup" << std::endl;
        }    
    }
    
    return NULL;
}

bool heartbeat_thread_init() {
    // Create the thread
    // std::cerr << "[DEBUG] [Coms] Creating thread" << std::endl;
    pthread_t thread,thread2;
    pthread_create(&thread, NULL, heartbeat_listener_thread, NULL);
    pthread_create(&thread2, NULL, heartbeat_writer_thread, NULL);
    return true;
}

bool primary_heartbeat_thread_init() {
    // Create the thread
    // std::cerr << "[DEBUG] [Coms] Creating thread" << std::endl;
    pthread_t thread;
    pthread_create(&thread, NULL, heartbeat_listener_thread, NULL);
    return true;
}

bool connect_to_server(uint32_t ip, uint16_t port, connection_t **out_connection) {
    connection_t *conn;
    {
        // Setup
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = { ip };
        bzero(&server_addr.sin_zero, 8);

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            std::cerr << "ERROR: [Creating connection] Could not create the socket" << std::endl;
            return false;
        }

        // Connect
        int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
        if (connect_response < 0) {
            std::cerr << "ERROR: [Election connection init 1a] Could not connect to the server" << std::endl;
            close(sockfd);
            return false;
        }

        add_connection(sockfd);

        // Create connection object
        // INTERNAL: the client's fields are set to the server's fields on purpose
        // TODO - Didio: figure out how to get client's IP and port
        conn = conn_new(
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                server_addr.sin_addr,
                ntohs(server_addr.sin_port),
                sockfd);
    }
    *out_connection = conn;

    return true;
}