
void *heartbeat_listener_thread(void *args);

void *heartbeat_writer_thread(void *args);

bool heartbeat_thread_init();

bool primary_heartbeat_thread_init();

