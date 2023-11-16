#ifndef LOGGER
#define LOGGER 1

#include <iostream>

// Set debug level
#define LOG_DEBUG

#ifdef LOG_DEBUG
#define log_debug(msg) std::cerr << "DEBUG: " << msg << " [File \"" __FILE__ "\", line " << __LINE__ << "]" << std::endl
#else
#define log_debug(msg) 
#endif

#define log_value(x) #x " = " << x

#endif
