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

#pragma once

#include <map>
#include <vector>
#include <regex>
#include "fcgiapp.h"

#include <rok4/datastream/DataStream.h>

// struct Route;

/**
 * \file Request.h
 * \~french
 * \brief Définition de la classe Request, analysant les requêtes HTTP
 * \details Définition de la classe Request
 * \~english
 * \brief Define the Request Class analysing HTTP requests
 * \details Define class Request
 */

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Classe décodant les requêtes HTTP envoyé au serveur.
 * Elle supporte les types de requête suivant :
 *  - HTTP GET de type KVP
 *  - HTTP POST de type XML (OGC SLD)
 * \brief Gestion des requêtes HTTP
 * \~english
 * HTTP request decoder class.
 * It support the following request type
 *  - HTTP GET, KVP style
 *  - HTTP POST , XML style (OGC SLD)
 * \brief HTTP requests handler
 */
class Request {
    friend class CppUnitRequest;

public:

    FCGX_Request* fcgx_request;

    /**
     * \~french
     * \brief Test de la présence d'un paramètre dans la requête
     * \param[in] paramName nom du paramètre à tester
     * \return true si présent
     * \~english
     * \brief Test if the request contain a specific parameter
     * \param[in] paramName parameter to test
     * \return true if present
     */
    bool has_query_param ( std::string paramName );

    /**
     * \~french
     * \brief Récupération de la valeur d'un paramètre dans la requête
     * \param[in] paramName nom du paramètre
     * \return valeur du parametre ou "" si non présent
     * \~english
     * \brief Fetch a specific parameter value in the request
     * \param[in] paramName parameter name
     * \return parameter value or "" if not availlable
     */
    std::string get_query_param ( std::string paramName );

    /**
     * \~french \brief Protocole, hôte, port et chemin
     * \~english \brief Protocol, host, port and path
     */
    std::string url;

    /**
     * \~french \brief Méthode de la requête (GET, POST, PUT, DELETE)
     * \~english \brief Request method (GET, POST, PUT, DELETE)
     */
    std::string method;
    /**
     * \~french \brief Chemin du serveur web pour accèder au service
     * \~english \brief Web Server path of the service
     */
    std::string path;

    /**
     * \~french \brief Liste des paramètres de la requête
     * \~english \brief Request parameters list
     */
    std::map<std::string, std::string> query_params;

    /**
     * \~french \brief Liste des paramètres extraits du chemin de la requête
     * \~english \brief Parameters list from request path
     */
    std::vector<std::string> path_params;

    /**
     * \~french \brief Corps de la requête
     * \~english \brief Request body
     */
    std::string body;

    /**
     * \~french \brief Export textuel
     * \~english \brief Text export
     */
    std::string to_string();

    /**
     * \~french \brief Est une requête INSPIRE ?
     * \param[in] inspire_default Comportement par défaut du service appelé
     * \~english \brief Is a INSPIRE request ?
     * \param[in] inspire_default Default behaviour of requested service
     */
    bool is_inspire(bool inspire_default = false);

    /**
     * \~french \brief Joue la requête
     * \details La requête ne doit pas être une requête reçue
     * \~english \brief Send the request
     * \details Request cannot be an input one
     */
    RawDataStream* send();
    
    /**
     * \~french
     * \brief Constructeur d'une requête reçue
     * \param fcgx Requête fcgi
     * \~english
     * \brief Input request Constructor
     * \param fcgx Fcgi request
     */
    Request ( FCGX_Request* fcgx);
    
    /**
     * \~french
     * \brief Constructeur d'une requête à jouer
     * \param m Méthode
     * \param u URL
     * \param qp Paramètres de requête
     * \~english
     * \brief Request to process constructor
     * \param m Method
     * \param u URL
     * \param qp Query parameters
     */
    Request (std::string m, std::string u, std::map<std::string, std::string> qp);

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Request();
};


