#ifndef STR_DH_LOGGER
#define STR_DH_LOGGER

#define LOG
#ifdef LOG
    #define LOG_DEBUG(m) {       \
        std::cout<<m<<std::endl; \
    }
#else
    #define LOG_DEBUG(m)
#endif

#endif 