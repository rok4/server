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
 * \file Request.cpp
 * \~french
 * \brief Implémentation de la classe Request, analysant les requêtes HTTP
 * \~english
 * \brief Implement the Request Class analysing HTTP requests
 */

#include <cstdlib>
#include <climits>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <boost/log/trivial.hpp>

#include "Request.h"
#include "UtilsXML.h"

#include "config.h"

/**
 * \~french
 * \brief Convertit un caractère héxadécimal (0-9, A-Z, a-z) en décimal
 * \param[in] hex caractère
 * \return 0xFF sur une entrée invalide
 * \~english
 * \brief Converts hex char (0-9, A-Z, a-z) to decimal.
 * \param[in] hex character
 * \return 0xFF on invalid input.
 */
char hex2int ( unsigned char hex ) {
    hex = hex - '0';
    // Si hex <= 9 on a le résultat
    //   Sinon
    if ( hex > 9 ) {
        hex = ( hex + '0' - 1 ) | 0x20; // Pour le passage des majuscules aux minuscules dans la table ASCII
        hex = hex - 'a' + 11;
    }
    if ( hex > 15 ) // En cas d'erreur
        hex = 0xFF;

    return hex;
}

void Request::url_decode ( char *src ) {
    unsigned char high, low;
    char* dst = src;

    while ( ( *src ) != '\0' ) {
        if ( *src == '+' ) {
            *dst = ' ';
        } else if ( *src == '%' ) {
            *dst = '%';

            high = hex2int ( * ( src + 1 ) );
            if ( high != 0xFF ) {
                low = hex2int ( * ( src + 2 ) );
                if ( low != 0xFF ) {
                    high = ( high << 4 ) | low;

                    /* map control-characters out */
                    if ( high < 32 || high == 127 ) high = '_';

                    *dst = high;
                    src += 2;
                }
            }
        } else {
            *dst = *src;
        }

        dst++;
        src++;
    }

    *dst = '\0';
}

Request::Request ( FCGX_Request* fcgxRequest ) : fcgx_request(fcgxRequest) {
    
    // Méthode
    method = std::string(FCGX_GetParam ( "REQUEST_METHOD",fcgxRequest->envp ));

    // Body
    if (method == "POST" || method == "PUT") {
        char* contentBuffer = ( char* ) malloc ( sizeof ( char ) *200 );
        while ( FCGX_GetLine ( contentBuffer, 200, fcgxRequest->in ) ) {
            body.append ( contentBuffer );
        }
        free ( contentBuffer );
        contentBuffer = NULL;
        body.append("\n");
        BOOST_LOG_TRIVIAL(debug) <<  "Request Content :" << std::endl << body ;
    }

    // Chemin
    path = std::string(FCGX_GetParam ( "SCRIPT_NAME",fcgxRequest->envp ));
    // Suppression du slash final
    if (path.compare ( path.size()-1,1,"/" ) == 0) {
        path.pop_back();
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Request: " << method << " " << path ;

    // Query parameters
    char* query = FCGX_GetParam ( "QUERY_STRING",fcgxRequest->envp );
    url_decode ( query );
    BOOST_LOG_TRIVIAL(debug) <<  "Query parameters: " << query ;
    for ( int pos = 0; query[pos]; ) {
        char* key = query + pos;
        for ( ; query[pos] && query[pos] != '=' && query[pos] != '&'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = query + pos;
        for ( ; query[pos] && query[pos] != '&'; pos++ ); // on trouve le suivant "&" ou 0
        if ( *value == '=' ) *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if ( query[pos] ) query[pos++] = 0; // on met un 0 à la fin du char* value

        UtilsXML::toLowerCase ( key );
        queryParams.insert ( std::pair<std::string, std::string> ( key, value ) );
    }
}


Request::~Request() {}

bool Request::hasParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = queryParams.find ( paramName );
    if ( it == queryParams.end() ) {
        it = bodyParams.find ( paramName );
        if ( it == bodyParams.end() ) {
            return false;
        }
    }
    return true;
}

std::string Request::getParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = queryParams.find ( paramName );
    if ( it == queryParams.end() ) {
        it = bodyParams.find ( paramName );
        if ( it == bodyParams.end() ) {
            return "";
        }
    }
    return it->second;
}
    
void Request::print() {
    BOOST_LOG_TRIVIAL(info) << "path = " << path;
    BOOST_LOG_TRIVIAL(info) << "method = " << method;
}
