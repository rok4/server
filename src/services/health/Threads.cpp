/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file services/health/Threads.cpp
 ** \~french
 * \brief Implémentation des classes Threads et InfoThread
 ** \~english
 * \brief Implements classes Threads and InfoThread
 */
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <boost/log/trivial.hpp>

#include "services/health/Threads.h"

std::map<long unsigned int, InfoThread> Threads::m_threads;

void Threads::add() {
    pthread_t i = pthread_self();
    Threads::add(i);
}

void Threads::add(long unsigned int i) {
    BOOST_LOG_TRIVIAL(debug) << "add" << "(call thread " << i << ")";
    m_threads.insert(std::make_pair<long unsigned int, InfoThread>(std::move(i), InfoThread(i)));
}

void Threads::status(eThreadStatus value) {
    pthread_t i = pthread_self();
    Threads::status(i, value);
}

void Threads::status(long unsigned int i, eThreadStatus value) {
    BOOST_LOG_TRIVIAL(debug) << "status : " << Threads::toString(value) << "(call thread " << i << ")";
    std::map<long unsigned int, InfoThread>::iterator it;
    it = m_threads.find(i);
    if (it == m_threads.end()) {
        BOOST_LOG_TRIVIAL(debug) << "thread " << i << " not found !?";
        return;
    }
    it->second.setStatus(Threads::toString(value));
    BOOST_LOG_TRIVIAL(debug) << "status update : " << it->second.getStatus();

    if (value == eThreadStatus::PENDING) {
        auto start = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(start);
        it->second.setTime(time);
        it->second.setDuration(0);
    }

    if (value == eThreadStatus::RUNNING) {
        auto start = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(start);
        it->second.setTime(time);
        BOOST_LOG_TRIVIAL(debug) << "time update : " << it->second.getTime();
    }

    if (value == eThreadStatus::AVAILABLE) {
        auto end = std::chrono::system_clock::now();
        std::time_t time = it->second.getTime();
        auto start = std::chrono::system_clock::from_time_t(time);
        std::chrono::duration<double> elapsed_seconds = end - start;
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

InfoThread::InfoThread(long unsigned int& i) {
    m_pid = i;
    m_status = Threads::toString(eThreadStatus::PENDING);
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