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

class ServerConf;

#ifndef SERVERXML_H
#define SERVERXML_H

#include <vector>
#include <string>
#include <map>
#include "Rok4Server.h"
#include <rok4/utils/TileMatrixSet.h>

#include <rok4/utils/Configuration.h>
#include "Layer.h"
#include <rok4/style/Style.h>

#include "config.h"

class ServerConf : public Configuration
{
    friend class Rok4Server;

    public:
        ServerConf(std::string path);
        ~ServerConf();

        std::string getLogOutput() ;
        int getLogFilePeriod() ;
        std::string getLogFilePrefix() ;
        boost::log::v2_mt_posix::trivial::severity_level getLogLevel() ;

        std::string getServicesConfigFile() ;

        std::string getLayersList() ;
        void addLayer(Layer* l) ;
        void removeLayer(std::string id) ;
        int getNbLayers() ;
        Layer* getLayer(std::string id) ;

        
        int getNbThreads() ;
        std::string getSocket() ;

    protected:

        std::string serverConfigFile;
        std::string servicesConfigFile;

        std::string logOutput;
        std::string logFilePrefix;
        int logFilePeriod;
        boost::log::v2_mt_posix::trivial::severity_level logLevel;

        int nbThread;

        /**
         * \~french \brief Taille du cache des index des dalles
         * \~english \brief Cache size
         */
        int cacheSize;
        /**
         * \~french \brief Temps de validité du cache en minutes
         * \~english \brief Cache validity period, in minutes
         */
        int cacheValidity;

        /**
         * \~french \brief Fichier ou objet contenant la liste des descipteurs de couche
         * \~english \brief Fil or object containing layers' descriptors list
         */
        std::string layerList;
        /**
         * \~french \brief Liste des couches disponibles
         * \~english \brief Available layers list
         */
        std::map<std::string, Layer*> layersList;

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
         * \~french \brief Définit si le serveur doit honorer les requêtes d'administration
         * \~english \brief Define whether administration request should be honored
         */
        bool supportAdmin;

    private:

        bool parse(json11::Json& doc);

};

#endif // SERVERXML_H

