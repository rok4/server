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
 * \file main.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Le serveur ROk4 peut fonctionner dans 2 modes distinct :
 *  - autonome, en définissant l'adresse et le port d'écoute dans le fichier de configuration
 *  - controlé, les paramètres d'écoute sont données par un processus maitre
 *
 * Paramètre d'entrée :
 *  - le chemin vers le fichier de configuration du serveur
 *
 * Signaux écoutés :
 *  - \b SIGHUP réinitialise la configuration du serveur
 *  - \b SIGQUIT & \b SIGUSR1 éteint le serveur
 * \brief Exécutable du serveur ROK4
 * \~english
 * The ROK4 server can be started in two mods :
 *  - autonomous, by defining in the config files the adress and port to listen to
 *  - managed, by letting a master process define the adress and port to liste to
 *
 * Command line parameter :
 *  - path to the server configuration file
 *
 * Listened Signal :
 *  - \b SIGHUP reinitialise the server configuration
 *  - \b SIGQUIT & \b SIGUSR1 shut the server down
 * \brief ROK4 Server executable
 */

#include "Rok4Server.h"
#include <proj.h>
#include <csignal>
#include <sys/time.h>
#include <locale>
#include <limits>
#include <chrono>
#include "config.h"
#include "curl/curl.h"
#include <time.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <rok4/utils/Cache.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

Rok4Server* rok4server_instance;
Rok4Server* rok4server_instance_tmp;
bool reload;
static bool logger_initialized = false;

std::string server_configuration_path;

// Minimum time between two signal to be defered.
// Earlier signal would be ignored.
// in microseconds
static const double signal_defering_min_time = 1000000LL;

volatile sig_atomic_t signal_pending = 0;
volatile sig_atomic_t defer_signal;
volatile timeval signal_timestamp;

/**
 * \~french
 * \brief Affiche les paramètres de la ligne de commande
 * \~english
 * \brief Display the command line parameters
 */
void usage() {
    std::cerr << "Usage : rok4 [-f server_configuration_path]" <<std::endl;
}

/**
* \brief Initialisation du serveur ROK4
* \return : pointeur sur le serveur ROK4, NULL en cas d'erreur (forcement fatale)
*/

Rok4Server* load_configuration() {

    ServerConfiguration* server_configuration = new ServerConfiguration( server_configuration_path );
    if ( ! server_configuration->is_ok() ) {
        std::cerr << "FATAL: Cannot load server configuration " << std::endl;
        std::cerr << "FATAL: " << server_configuration->get_error_message() << std::endl;
        return NULL;
    }

    if ( ! logger_initialized ) {
        /* Initialisation du logger */
        boost::log::core::get()->set_filter( boost::log::trivial::severity >= server_configuration->get_log_level() );
        logging::add_common_attributes();
        boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");

        if ( server_configuration->get_log_output() == "rolling_file") {
            logging::add_file_log (
                keywords::file_name = server_configuration->get_log_file_prefix()+"-%Y-%m-%d-%H-%M-%S.log",
                keywords::time_based_rotation = sinks::file::rotation_at_time_interval(boost::posix_time::seconds(server_configuration->get_log_file_period())),
                keywords::format = "%TimeStamp%\t%ProcessID%\t%ThreadID%\t%Severity%\t%Message%",
                keywords::auto_flush = true
            );
        } else if ( server_configuration->get_log_output() == "static_file") {
            logging::add_file_log (
                keywords::file_name = server_configuration->get_log_file_prefix(),
                keywords::format = "%TimeStamp%\t%ProcessID%\t%ThreadID%\t%Severity%\t%Message%",
                keywords::auto_flush = true
            );
        } else if ( server_configuration->get_log_output() == "standard_output") {
            logging::add_console_log (
                std::cout,
                keywords::format = "%TimeStamp%\t%ProcessID%\t%ThreadID%\t%Severity%\t%Message%"
            );
        }

        std::cout <<  "Envoi des messages dans la sortie du logger" << std::endl;
        BOOST_LOG_TRIVIAL(info) <<   "*** DEBUT DU FONCTIONNEMENT DU LOGGER ***" ;
        logger_initialized = true;
    }

    // Construction des parametres de service
    ServicesConfiguration* services_configuration = new ServicesConfiguration ( server_configuration->get_services_configuration_file() );
    if ( ! services_configuration->is_ok() ) {
        BOOST_LOG_TRIVIAL(fatal) << "Cannot load services configuration " << std::endl;
        BOOST_LOG_TRIVIAL(fatal) << services_configuration->get_error_message() << std::endl;
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Chargement des layers

    BOOST_LOG_TRIVIAL(info) << "LAYERS LOADING" ;
    
    std::string list_path = server_configuration->get_layers_list();

    if (list_path != "") {
        // Lecture de la liste des couches

        ContextType::eContextType storage_type;
        std::string tray_name, fo_name;
        ContextType::split_path(list_path, storage_type, fo_name, tray_name);

        Context* context = StoragePool::get_context(storage_type, tray_name);
        if (context == NULL) {
            BOOST_LOG_TRIVIAL(fatal) << "Cannot add " + ContextType::to_string(storage_type) + " storage context to read layers list" << std::endl;
            return NULL;
        }

        int size = -1;
        uint8_t* data = context->read_full(size, fo_name);

        if (size < 0) {
            BOOST_LOG_TRIVIAL(fatal) << "Cannot read layers list " + list_path << std::endl;
            if (data != NULL) delete[] data;
            return NULL;
        }
        
        std::istringstream list_content(std::string((char*) data, size));
        delete[] data; 
        std::string layer_desc;    
        while (std::getline(list_content, layer_desc)) {
            Layer* layer = new Layer(layer_desc );
            if ( layer->is_ok() ) {
                server_configuration->add_layer ( layer );
            } else {
                BOOST_LOG_TRIVIAL(error) << "Cannot load layer " << layer_desc << ": " << layer->get_error_message();
                delete layer;
            }
        }
    }

    BOOST_LOG_TRIVIAL(info) << server_configuration->get_layers_count() << " layer(s) loaded" ;

    // Instanciation du serveur
    return new Rok4Server ( server_configuration, services_configuration );
}

/**
 * \~french
 * \brief Force le rechargement de la configuration
 * \~english
 * \brief Force configuration reload
 */
void reload_configuration ( int signum ) {
    if ( defer_signal ) {
        timeval now;
        gettimeofday ( &now, NULL );
        double delta = ( now.tv_sec - signal_timestamp.tv_sec ) *1000000LL + ( now.tv_usec - signal_timestamp.tv_usec );
        if ( delta > signal_defering_min_time ) {
            signal_pending = signum;
        }
    } else {
        defer_signal++;
        signal_pending = 0;
        timeval begin;
        gettimeofday ( &begin, NULL );
        signal_timestamp.tv_sec = begin.tv_sec;
        signal_timestamp.tv_usec = begin.tv_usec;
        reload = true;
        std::cout<<  "Rechargement du serveur rok4" << "["<< getpid() <<"]" <<std::endl;

        rok4server_instance_tmp = load_configuration();
        if ( ! rok4server_instance_tmp ){
            std::cout<<  "Erreur lors du rechargement du serveur rok4" << "["<< getpid() <<"]" <<std::endl;
            return;
        }
        rok4server_instance->terminate();
    }
}
/**
 * \~french
 * \brief Force le serveur à s'éteindre
 * \~english
 * \brief Force server shutdown
 */
void shutdown_server ( int signum ) {
    if ( defer_signal ) {
        // Do nothing because rok4 is going to shutdown...
    } else {
        defer_signal++;
        reload = false;
        rok4server_instance->terminate();
    }
}

/**
 * \~french
 * \brief Retourne l'emplacement des fichier de traduction
 * \return repertoire des traductions
 * \~english
 * \brief Return the translation files path
 * \return translation directory
 */
std::string get_locale_path() {
    char result[ 4096 ];
    char procPath[20];
    sprintf ( procPath,"/proc/%u/exe",getpid() );
    ssize_t count = readlink ( procPath, result, 4096 );
    std::string exePath ( result, ( count > 0 ) ? count : 0 );
    std::string localePath ( exePath.substr ( 0,exePath.rfind ( "/" ) ) );
    localePath.append ( "/../share/locale" );
    return localePath;
}

/**
 * \~french
 * \brief Fonction principale
 * \return 1 en cas de problème, 0 sinon
 * \~english
 * \brief Main function
 * \return 1 if error, else 0
 */
int main ( int argc, char** argv ) {

    bool first_start = true;
    int sock = 0;
    reload = true;
    defer_signal = 1;
    /* install Signal Handler for Conf Reloadind and Server Shutdown*/
    struct sigaction sa;
    sigemptyset ( &sa.sa_mask );
    sa.sa_flags = 0;
    sa.sa_handler = reload_configuration;
    sigaction ( SIGHUP, &sa,0 );

    sa.sa_handler = shutdown_server;
    sigaction ( SIGQUIT, &sa,0 );

    // On n'utilise pas la locale pour les numériques, pour garder le point comme séparateur de décimale
    //setlocale ( LC_ALL,"" );
    setlocale ( LC_COLLATE,"" );
    setlocale ( LC_CTYPE,"" );
    setlocale ( LC_MONETARY,"" );
    setlocale ( LC_TIME,"" );

    //CURL initialization - one time for the whole program
    curl_global_init(CURL_GLOBAL_ALL);

    /* the following loop is for fcgi debugging purpose */
    int stop_sleep = 0;
    while ( getenv ( "SLEEP" ) != NULL && stop_sleep == 0 ) {
        sleep ( 2 );
    }

    // Lecture des arguments de la ligne de commande
    server_configuration_path=DEFAULT_SERVER_CONF_PATH;
    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'f': // fichier de configuration du serveur
                if ( i++ >= argc ) {
                    std::cerr<< "Invalid -f option" <<std::endl;
                    usage();
                    return 1;
                }
                server_configuration_path.assign ( argv[i] );
                break;
            default:
                usage();
                return 1;
            }
        }
    }

    // Demarrage du serveur
    while ( reload ) {

        reload = false;
        int pid = getpid();
        std::cout<<  "Server start " << "["<< pid <<"]" <<std::endl;

        if ( first_start ) {
            rok4server_instance = load_configuration();
            if ( !rok4server_instance ) {
                return 1;
            }
            rok4server_instance->initialize_fcgi();
            first_start = false;
        } else {
            std::cout<<  "Configuration update " << "["<< pid <<"]" <<std::endl;
            if ( rok4server_instance_tmp ) {
                std::cout<<  "Servers switch " << "["<< pid <<"]" <<std::endl;
                rok4server_instance = rok4server_instance_tmp;
                rok4server_instance_tmp = 0;
                TmsBook::empty_trash();
                StyleBook::empty_trash();
            }
            rok4server_instance->set_fcgi_socket ( sock );
        }

        auto start = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(start);
        rok4server_instance->set_pid(pid);
        rok4server_instance->set_time(time);

        // Remove Event Lock
        defer_signal--;
        
        rok4server_instance->run(signal_pending);

        TmsBook::send_to_trash();
        StyleBook::send_to_trash();

        if ( reload ) {
            // Rechargement du serveur
            BOOST_LOG_TRIVIAL(info) << "Configuration reload" ;
            sock = rok4server_instance->get_fcgi_socket();
        } else {
            // Extinction du serveur
            BOOST_LOG_TRIVIAL(info) << "Server shutdown" ;
        }

        delete rok4server_instance;
    }

    TmsBook::empty_trash();
    StyleBook::empty_trash();
    CurlPool::clean_curls();
    ProjPool::clean_projs();
    StoragePool::clean_storages();
    IndexCache::clean_indexes();

    //CRYPTO clean - one time for the whole program
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    //CURL clean - one time for the whole program
    curl_global_cleanup();
    //Clear proj6 cache
    proj_cleanup();

    return 0;
}
