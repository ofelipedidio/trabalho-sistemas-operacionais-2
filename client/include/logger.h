#ifndef LOGGER
#define LOGGER 1

#include <iostream>
#include <mutex>

// Set debug level
#define LOG_DEBUG

inline std::mutex _log_mutex;

#ifdef LOG_DEBUG
#define log_debug(msg) _log_mutex.lock(); std::cerr << "DEBUG: [File \"" __FILE__ "\", line " << __LINE__ << "] " << msg << std::endl; _log_mutex.unlock();
#else
#define log_debug(msg) 
#endif

#define log_value(x) #x " = `" << x << "`"

#endif
