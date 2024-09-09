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
 * \file services/health/Threads.h
 ** \~french
 * \brief Définition des classes InfoThread et Threads
 ** \~english
 * \brief Define classes InfoThread and Threads
 */

#pragma once

#include <map>
#include <chrono>

#include <rok4/thirdparty/json11.hpp>

#include "Request.h"

enum eThreadStatus {
    UNKNOWN,
    RUNNING,
    PENDING,
    AVAILABLE
};

const char* const threadstatus_name[] = {
    "UNKNOWN",
    "RUNNING",
    "PENDING",
    "AVAILABLE"
};

class InfoThread {
    private:

        /**
         * @brief PID or thread ID
         * 
         */
        long unsigned int pid;

        /**
         * @brief Thread status : PENDING | RUNNING | AVAIBLE
         * 
         */
        std::string status;

        /**
         * @brief Number of times the thread changes status
         * 
         */
        int count;

        /**
         * @brief Time to the last execution
         * 
         */
        std::time_t time;

        /**
         * @brief Duration to the last execution
         * 
         */
        long double duration;

    public:
        /**
         * @brief Construct a new Stat object
         * 
         */
        InfoThread(){};
        InfoThread(long unsigned int&);
        InfoThread(long unsigned int&, std::string);

        json11::Json to_json() const {
            return json11::Json::object {
                { "pid", (int) pid },
                { "status", status },
                { "count", count },
                { "time", (int) time },
                { "duration", (double) duration }
            };
        }

        /**
         * @brief Destroy the Stat object
         * 
         */
        ~InfoThread(){};

        /**
         * @brief 
         * 
         * @return std::string
         */
        long unsigned int get_pid();

        /**
         * @brief Get the Status object
         * 
         * @return std::string 
         */
        std::string get_status();

        /**
         * @brief Set the Status object
         * 
         */
        void set_status(std::string);

        /**
         * @brief Get the Count object
         * 
         * @return int 
         */
        int get_count();

        /**
         * @brief Set the Count object
         * 
         */
        void set_count();

        /**
         * @brief Set the Time object
         * 
         */
        void set_time(std::time_t);

        /**
         * @brief Get the Time object
         * 
         * @return long int
         */
        std::time_t get_time() {
            return time;
        }

        /**
         * @brief Get the Duration object
         * 
         * @return long double 
         */
        long double get_duration() {
            return duration;
        }

        /**
         * @brief Set the Duration object
         * 
         * @param double 
         */
        void set_duration(long double);

};

class Threads {
    private:
        /**
         * @brief Thread list information
         * 
         */
        static std::map<long unsigned int, InfoThread> threads;

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
        static void status(eThreadStatus);
        static void status(long unsigned int, eThreadStatus);

        /**
         * @brief Show status of all threads in JSON format
         * 
         * @return std::string 
         */
        static json11::Json to_json() {

            std::vector<InfoThread> res;
            for(auto const& t: threads) {
                res.push_back(t.second);
            }

            return json11::Json(res);
        };

        static std::string to_string (eThreadStatus st) {
            return std::string ( threadstatus_name[st] );
        }
};

