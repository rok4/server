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
 * \file Rok4Server.cpp
 * \~french
 * \brief Implémentation de la classe Rok4Server et du programme principal
 * \~english
 * \brief Implement the Rok4Server class, handling the event loop
 */

#include <errno.h>
#include <fcntl.h>
#include <proj.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cmath>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <curl/curl.h>
#include <fcgiapp.h>

#include "Rok4Server.h"
#include "config.h"

#include "services/Router.h"
#include "services/health/Threads.h"

void hangleSIGALARM(int id) {
    if (id == SIGALRM) {
        exit(0); /* exit on receiving SIGALRM signal */
    }
    signal(SIGALRM, hangleSIGALARM);
}

void* Rok4Server::thread_loop(void* arg) {
    Rok4Server* server = (Rok4Server*)(arg);
    FCGX_Request fcgxRequest;
    if (FCGX_InitRequest(&fcgxRequest, server->sock, FCGI_FAIL_ACCEPT_ON_INTR) != 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Le listener FCGI ne peut etre initialise";
    }

    while (server->isRunning()) {
        std::string content;

        int rc;
        if ((rc = FCGX_Accept_r(&fcgxRequest)) < 0) {
            if (rc == -4) {  // Cas du redémarrage
                BOOST_LOG_TRIVIAL(debug) << "Redémarrage : FCGX_InitRequest renvoie le code d'erreur " << rc;
            } else {
                BOOST_LOG_TRIVIAL(error) << "FCGX_InitRequest renvoie le code d'erreur " << rc;
                std::cerr << "FCGX_InitRequest renvoie le code d'erreur " << rc << std::endl;
            }

            break;
        }

        BOOST_LOG_TRIVIAL(debug) << "Thread " << pthread_self() << " traite une requete";
        Threads::status(eThreadStatus::RUNNING);

        Request* request = new Request(&fcgxRequest);
        Router::process_request(request, server);
        delete request;

        FCGX_Finish_r(&fcgxRequest);
        FCGX_Free(&fcgxRequest, 1);

        BOOST_LOG_TRIVIAL(debug) << "Thread " << pthread_self() << " en a fini avec la requete";
        Threads::status(eThreadStatus::AVAILABLE);
    }

    BOOST_LOG_TRIVIAL(debug) << "Extinction du thread";
    return 0;
}

Rok4Server::Rok4Server(ServerConf* svr, ServicesConf* svc) {
    sock = 0;
    servicesConf = svc;
    serverConf = svr;
    
    if (svr->cacheValidity > 0) {
        IndexCache::setValidity(svr->cacheValidity * 60);
    }
    if (svr->cacheSize > 0) {
        IndexCache::setCacheSize(svr->cacheSize);
    }

    threads = std::vector<pthread_t>(serverConf->getNbThreads());

    running = false;
}

Rok4Server::~Rok4Server() {
    delete serverConf;
    delete servicesConf;
}

void Rok4Server::initFCGI() {
    int init = FCGX_Init();
    BOOST_LOG_TRIVIAL(info) << "Listening on " << serverConf->socket;
    sock = FCGX_OpenSocket(serverConf->socket.c_str(), serverConf->backlog);
}

void Rok4Server::run(sig_atomic_t signal_pending) {
    running = true;

    for (int i = 0; i < threads.size(); i++) {
        pthread_create(&(threads[i]), NULL, Rok4Server::thread_loop, (void*)this);
	    Threads::add(threads[i]);
    }

    if (signal_pending != 0) {
        raise(signal_pending);
    }

    for (int i = 0; i < threads.size(); i++)
        pthread_join(threads[i], NULL);
}

void Rok4Server::terminate() {
    running = false;

    // Terminate FCGI Thread
    for (int i = 0; i < threads.size(); i++) {
        pthread_kill(threads[i], SIGQUIT);
    }
}

/******************* GETTERS / SETTERS *****************/

ServicesConf* Rok4Server::getServicesConf() { return servicesConf; }
ServerConf* Rok4Server::getServerConf() { return serverConf; }
std::vector<pthread_t>& Rok4Server::get_threads() {return threads;}

int Rok4Server::getFCGISocket() { return sock; }
void Rok4Server::setFCGISocket(int sockFCGI) { sock = sockFCGI; }
int Rok4Server::getPID() { return pid; }
void Rok4Server::setPID(int processID) { pid = processID; }
long Rok4Server::getTime() { return time; }
void Rok4Server::setTime(long processTime) { time = processTime; }
bool Rok4Server::isRunning() { return running; }
