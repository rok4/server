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
 * \file configurations/Server.h
 ** \~french
 * \brief Définition de la classe ServerConfiguration
 ** \~english
 * \brief Define classe ServerConfiguration
 */

class ServerConfiguration;

#pragma once

#include <vector>
#include <string>
#include <map>

#include <rok4/utils/TileMatrixSet.h>
#include <rok4/utils/Configuration.h>
#include <rok4/style/Style.h>

#include "configurations/Layer.h"
#include "Rok4Server.h"

#include "config.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Gestion de la configuration générale du serveur
 */
class ServerConfiguration : public Configuration
{
    friend class Rok4Server;
    friend class CommonService;
    friend class TmsService;

    public:
        ServerConfiguration(std::string path);
        ~ServerConfiguration();

        bool is_enabled();

        std::string get_log_output() ;
        int get_log_file_period() ;
        std::string get_log_file_prefix() ;
        boost::log::v2_mt_posix::trivial::severity_level get_log_level() ;

        std::string get_services_configuration_file() ;

        std::string get_layers_list() ;
        std::map<std::string, Layer*>& get_layers() ;
        void add_layer(Layer* l) ;
        void delete_layer(std::string id) ;
        int get_layers_count() ;
        Layer* get_layer(std::string id) ;

        
        int get_threads_count() ;
        std::string get_socket() ;

    protected:

        std::string services_configuration_file;

        std::string log_output;
        std::string log_file_prefix;
        int log_file_period;
        boost::log::v2_mt_posix::trivial::severity_level log_level;

        int threads_count;

        /**
         * \~french \brief Taille du cache des index des dalles
         * \~english \brief Cache size
         */
        int cache_size;
        /**
         * \~french \brief Temps de validité du cache en minutes
         * \~english \brief Cache validity period, in minutes
         */
        int cache_validity;

        /**
         * \~french \brief Fichier ou objet contenant la liste des descipteurs de couche
         * \~english \brief File or object containing layers' descriptors list
         */
        std::string layers_list;
        /**
         * \~french \brief Liste des couches disponibles
         * \~english \brief Available layers list
         */
        std::map<std::string, Layer*> layers;

        /**
         * \~french \brief Adresse du socket d'écoute (vide si lancement géré par un tiers)
         * \~english \brief Listening socket address (empty if lauched in managed mode)
         */
        std::string socket;
        /**
         * \~french \brief Profondeur de la file d'attente du socket
         * \~english \brief Socket listen queue depth
         */
        int backlog;

        /**
         * \~french \brief Définit si le serveur doit honorer les requêtes de consultation
         * \~english \brief Define whether broadcast request should be honored
         */
        bool enabled;

    private:

        bool parse(json11::Json& doc);

};



