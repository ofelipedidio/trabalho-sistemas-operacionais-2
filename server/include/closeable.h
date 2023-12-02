#ifndef CLOSEABLE
#define CLOSEABLE

void add_connection(int fd);

void close_all_connections();

void sigint_handler(int param);

#endif // !CLOSEABLE
