#ifndef LOGGER
#define LOGGER

#include <iostream>
#include <mutex>

// Set log level
#define LOG_DEBUG
#define LOG_ERROR

inline std::mutex _log_mutex;

#ifdef LOG_DEBUG
#define log_debug(msg) _log_mutex.lock(); std::cerr << "DEBUG: [File \"" __FILE__ "\", line " << __LINE__ << "] " << msg << std::endl; _log_mutex.unlock();
#else
#define log_debug(msg) 
#endif

#ifdef LOG_ERROR
#define log_error(msg) std::cerr << "ERROR: [File \"" __FILE__ "\", line " << __LINE__ << "] " << msg << "\n";
#else
#define log_error(msg)
#endif

#define log_value(x) #x " = `" << x << "`"

#endif
