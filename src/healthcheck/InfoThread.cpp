#include <chrono>

#include "Threads.h"
#include "InfoThread.h"


InfoThread::InfoThread(long unsigned int& i) {
    m_pid = i;
    m_status = ThreadStatus::toString(ThreadStatus::PENDING);
    m_count = 0;
    m_duration = 0;
    m_time = 0;
}

InfoThread::InfoThread(long unsigned int& i, std::string st) {
    m_pid = i;
    m_status = st;
    m_count = 0;
    m_duration = 0;
    m_time = 0;
}

long unsigned int InfoThread::getPID() {
    return m_pid;
}

std::string InfoThread::getStatus() {
    return m_status;
}

void InfoThread::setStatus(std::string s) {
    m_status = s;
}

int InfoThread::getCount() {
    return m_count;
}

void InfoThread::setCount() {
    m_count++;
}

void InfoThread::setTime(std::time_t t) {
    m_time = t;
}

void InfoThread::setDuration(long double d) {
    m_duration = d;
}