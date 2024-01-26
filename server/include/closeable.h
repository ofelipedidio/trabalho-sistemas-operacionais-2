#ifndef CLOSEABLE
#define CLOSEABLE

void add_connection(int fd);

void close_all_connections();

void sigint_handler(int param);

void sigpipe_handler(int signal_number);
#endif // !CLOSEABLE
