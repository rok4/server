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
 * \file services/Service.h
 ** \~french
 * \brief Définition de la classe Service
 ** \~english
 * \brief Define classe Service
 */

#ifndef SERVICE_H_
#define SERVICE_H_

#include <regex>

#include <utils/Configuration.h>
#include <utils/Keyword.h>
#include <rok4/datastream/DataStream.h>

class Rok4Server;
class Request;

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Abstraction d'un service du serveur
 */
class Service : public Configuration {

protected:

    std::string title;
    std::string abstract;
    std::vector<Keyword> keywords;
    std::string endpoint_uri;
    std::string root_path;
    bool enabled;

    /**
     * \~french
     * \brief Teste la requête vis à vis de la route
     * \details La méthode est comparée. Si la requête correspond à la route, les paramètres du chemin sont extraits
     * \param[in] route Route de validation de la requête
     * \param[in] req Requête à valider
     * \~english
     * \brief Test if request match the route
     * \details Method is tested. If request matches the route, path params are extracted
     * \param[in] route Route to test request
     * \param[in] req Request to test
     */
    bool match_route(std::string path, std::vector<std::string> methods, Request* req);

public:
    /**
     * \~french
     * \brief Constructeur d'un service
     * \~english
     * \brief Service constructor
     */
    Service (json11::Json& doc);

    virtual DataStream* process_request(Request* req, Rok4Server* serv) = 0;

    std::string get_root_path() {return root_path;};
    bool is_enabled() {return enabled;};
    bool match_request(Request* req);
};

#endif /* SERVICE_H_ */


