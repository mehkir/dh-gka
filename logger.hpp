#ifndef LOGGER
#define LOGGER

#include <iostream>

#define LOG
#ifdef LOG
    #define LOG_DEBUG(m) {       \
        std::cout<<m<<std::endl; \
    }
#else
    #define LOG_DEBUG(m)
#endif

#endif 