#include <string>
#include <map>
#include <chrono>
#include <pthread.h>

#include "InfoThread.h"
#include "../Request.h"

#ifndef STATS_H_
#define STATS_H_

/**
 * @brief Thread status
 * 
 */
namespace ThreadStatus {
    /**
     * @brief 
     * 
     */
    enum eStatus {
        UNKNOWN,
        RUNNING,
        PENDING,
        AVAILABLE
    };

    /**
     * @brief 
     * 
     * @param st 
     * @return std::string 
     */
    std::string toString (eStatus st);
}

class Threads {
    private:
        /**
         * @brief Thread list information
         * 
         */
        static std::map<long unsigned int, InfoThread> m_threads;

        /**
         * @brief Construct a new Stats object
         * 
         */
        Threads(){};

    public:
        /**
         * @brief Destroy the Stats object
         * 
         */
        ~Threads(){};

        /**
         * @brief Add a new thread to the map
         * @warning use only in execution thread if id is NULL, 
         * or else in main thread !
         * 
         */
        static void add();
        static void add(long unsigned int);
        static void add(pthread_t);

        /**
         * @brief Update thread information (request, ...)
         * @warning use only in execution thread if id is NULL, 
         * or else in main thread !
         * 
         */
        static void update(Request*);
        static void update(long unsigned int, Request*);

        /**
         * @brief Update thread status
         * @warning use only in execution thread if id is NULL, 
         * or else in main thread !
         * 
         * @see Status
         */
        static void status(ThreadStatus::eStatus);
        static void status(long unsigned int, ThreadStatus::eStatus);
        static void status(pthread_t, ThreadStatus::eStatus);

        /**
         * @brief Show status of all threads in JSON format
         * 
         * @return std::string 
         */
        static std::string print();
};

#endif /* STATS_H_ */
