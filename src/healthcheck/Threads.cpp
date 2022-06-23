#include "Threads.h"
#include "InfoThread.h"
#include "../Request.h"

#include <string>
#include <string.h>
#include <chrono>
#include <ctime> 
#include <map>
#include <boost/log/trivial.hpp>

namespace ThreadStatus {
    
    const char* const strStatus[] = {
        "UNKNOWN",
        "RUNNING",
        "PENDING",
        "AVAILABLE"
    };

    std::string toString (eStatus st) {
        return std::string ( strStatus[st] );
    }
}

std::map<long unsigned int, InfoThread> Threads::m_threads;

void Threads::add() {
    pthread_t i = pthread_self();
    Threads::add(i);
}

void Threads::add(long unsigned int i) {
    BOOST_LOG_TRIVIAL(debug) << "add" << "(call thread " << i << ")";
    m_threads.insert(std::make_pair<long unsigned int, InfoThread>(std::move(i), InfoThread(i)));
}

void Threads::status(ThreadStatus::eStatus value) {
    pthread_t i = pthread_self();
    Threads::status(i, value);
}

void Threads::status(long unsigned int i, ThreadStatus::eStatus value) {
    BOOST_LOG_TRIVIAL(debug) << "status : " << ThreadStatus::toString(value) << "(call thread " << i << ")";
    std::map<long unsigned int, InfoThread>::iterator it;
    it = m_threads.find(i);
    if (it == m_threads.end()) {
        BOOST_LOG_TRIVIAL(debug) << "thread " << i << " not found !?";
        return;
    }
    it->second.setStatus(ThreadStatus::toString(value));
    BOOST_LOG_TRIVIAL(debug) << "status update : " << it->second.getStatus();

    if (value == ThreadStatus::PENDING) {
        auto start = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(start);
        it->second.setTime(time);
        it->second.setDuration(0);
    }

    if (value == ThreadStatus::RUNNING) {
        auto start = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(start);
        it->second.setTime(time);
        BOOST_LOG_TRIVIAL(debug) << "time update : " << it->second.getTime();
    }

    if (value == ThreadStatus::AVAILABLE) {
        auto end = std::chrono::system_clock::now();
        std::time_t time = it->second.getTime();
        auto start = std::chrono::system_clock::from_time_t(time);
        std::chrono::duration<double> elapsed_seconds = end-start;
        double duration = elapsed_seconds.count();
        it->second.setDuration(duration);
        BOOST_LOG_TRIVIAL(debug) << "duration update : " << it->second.getDuration();
        it->second.setCount();
        BOOST_LOG_TRIVIAL(debug) << "count update : " << it->second.getCount();
    }

}

std::string Threads::print() {
    std::ostringstream res;
    std::map<long unsigned int, InfoThread>::iterator it;
    it = m_threads.begin();
    while(it != m_threads.end()) {
        res << "  {\n";
        res << "    \"pid\":" << it->second.getPID() << ",\n";
        res << "    \"status\":" << "\"" << it->second.getStatus() << "\",\n";
        res << "    \"count\":" << it->second.getCount() << ",\n";
        res << "    \"time\":" << it->second.getTime() << ",\n";
        res << "    \"duration\":" << it->second.getDuration() << "\n";
        res << "  }";
        if (++it != m_threads.end()) {
            res << ",";
        }
        res << "\n";
    }
    return res.str();
}
