#ifndef LOGGER
#define LOGGER

#include <iostream>

#ifdef LOGSTD
    #define LOG_STD(m) {         \
        std::cout<<m<<std::endl; \
    }
#else
    #define LOG_STD(m)
#endif

#ifdef LOGD
    #define LOG_DEBUG(m) {       \
        std::cout<<m<<std::endl; \
    }
#else
    #define LOG_DEBUG(m)
#endif

#endif 