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
 * \file Rok4Server.h
 * \~french
 * \brief Definition de la classe Rok4Server et du programme principal
 * \~english
 * \brief Define the Rok4Server class, handling the event loop
 */

class ServicesConf;
class Server;

#ifndef _ROK4SERVER_
#define _ROK4SERVER_

#include "fcgiapp.h"
#include <csignal>
#include <stdio.h>
#include <pthread.h>
#include <map>
#include <vector>

#include "configurations/Server.h"
#include "configurations/Services.h"
#include "configurations/Layer.h"

#include "Request.h"

#include "config.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un serveur Rok4 stocke les informations de configurations des services
 * Il définit aussi la boucle d'évènement nécessaire pour répondre aux requêtes transmises via FCGI
 * \brief Gestion du programme principal, lien entre les modules
 * \~english
 * The Rok4 Server stores services configuration.
 * It also define the event loop to handle the FCGI request
 * \brief Handle the main program (event loop) and links
 */
class Rok4Server {

private:
    /**
     * \~french \brief Liste des processus léger
     * \~english \brief Threads liste
     */
    std::vector<pthread_t> threads;

    /**
     * \~french \brief Défini si le serveur est en cours d'éxécution
     * \~english \brief Define whether the server is running
     */
    volatile bool running;

    /**
     * \~french \brief Identifiant du socket
     * \~english \brief Socket identifier
     */
    int sock;

    /**
     * \~french \brief Identifiant du process
     * \~english \brief Process identifier
     */
    int pid;

    /**
     * \~french \brief TimeStamp du process
     * \~english \brief Process timestamp
     */
    long time;

    /**
     * \~french \brief Configurations des services
     * \~english \brief Services configuration
     */
    ServicesConf* servicesConf;

    /**
     * \~french \brief Configuration du serveur
     * \~english \brief Server configuration
     */
    ServerConf* serverConf;

    /**
     * \~french
     * \brief Boucle principale exécutée par chaque thread à l'écoute des requêtes des utilisateurs.
     * \param[in] arg pointeur vers l'instance de Rok4Server
     * \return true si présent
     * \~english
     * \brief Main event loop executed by each thread, listening to user request
     * \param[in] arg pointer to the Rok4Server instance
     * \return true if present
     */
    static void* thread_loop ( void* arg );

public:
    /**
     * \~french Retourne la configuration des services
     * \~english Return the services configurations
     */
    ServicesConf* getServicesConf() ;
    /**
     * \~french Retourne la configuration du serveur
     * \~english Return the server configuration
     */
    ServerConf* getServerConf() ;

    /**
     * \~french Retourne la liste des threads
     * \~english Return the threads list
     */
    std::vector<pthread_t>& get_threads() ;

    /**
     * \~french
     * \brief Lancement des threads du serveur
     * \~english
     * \brief Start server's thread
     */
    void run(sig_atomic_t signal_pending = 0);
    /**
     * \~french
     * \brief Initialise le socket FastCGI
     * \~english
     * \brief Initialize the FastCGI Socket
     */
    void initFCGI();
    /**
     * \~french
     * Utilisé pour le rechargement de la configuration du serveur
     * \brief Retourne la représentation interne du socket FastCGI
     * \return la représentation interne du socket
     * \~english
     * \brief Get the internal FastCGI socket representation, usefull for configuration reloading.
     * \return the internal FastCGI socket representation
     */
    int getFCGISocket() ;

    /**
     * \~french
     * Utilisé pour le rechargement de la configuration du serveur
     * \brief Restaure le socket FastCGI
     * \param sockFCGI la représentation interne du socket
     * \~english
     * Useful for configuration reloading
     * \brief Set the internal FastCGI socket representation
     * \param sockFCGI the internal FastCGI socket representation
     */
    void setFCGISocket ( int sockFCGI ) ;
    
    /**
     * \~french
     * \brief Stocke le PId du process principal
     * \~english
     * \brief Set the main process PID
     */
    void setPID ( int processID );

    /**
     * \~french
     * \brief Obtient le PID du process principal
     * \~english
     * \brief Get the main process PID
     */
    int getPID();

    /**
     * \~french
     * \brief Stocke la date du process principal
     * \~english
     * \brief Set the main process time
     */
    void setTime ( long processTime );

    /**
     * \~french
     * \brief Obtient la date du process principal
     * \~english
     * \brief Get the main process time
     */
    long getTime();

     /**
     * \~french
     * \brief Demande l'arrêt du serveur
     * \~english
     * \brief Ask for server shutdown
     */
    void terminate();

    /**
     * \~french
     * \brief Retourne l'état du serveur
     * \return true si en fonctionnement
     * \~english
     * \brief Return the server state
     * \return true if running
     */
    bool isRunning() ;

    /**
     * \brief Construction du serveur
     */
    Rok4Server ( ServerConf* svr, ServicesConf* svc);
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Rok4Server ();

};

#endif

