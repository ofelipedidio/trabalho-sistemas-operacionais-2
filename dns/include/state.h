#include <cstdint>
#include <semaphore.h>

typedef struct {
    uint32_t primary_ip;
    uint16_t primary_port;

    sem_t logging_mutex;
} program_state_t;

/****************\
* Initialization *
\****************/
void state_init(uint32_t ip, uint16_t port);

void acquire_logging_mutex();

void release_logging_mutex();

void get_primary_server(uint32_t *ip, uint16_t *port);

void set_primary_server(uint32_t ip, uint16_t port);

#define LOG_SYNC(x) acquire_logging_mutex(); x; release_logging_mutex(); std::cerr.flush(); std::cout.flush()
