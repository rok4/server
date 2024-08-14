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
 * \file services/Router.cpp
 ** \~french
 * \brief Implémentation de la classe Router
 ** \~english
 * \brief Implements classe Router
 */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <boost/log/trivial.hpp>
#include <stdexcept>

#include "services/Router.h"
#include "services/common/Service.h"
#include "services/health/Service.h"
#include "services/tms/Service.h"
#include "services/wmts/Service.h"
#include "services/admin/Service.h"
#include "services/tiles/Service.h"
#include "services/wms/Service.h"

std::string get_message_from_http_status ( int http_status ) {
    switch ( http_status ) {
    case 200 :
        return "OK" ;
    case 204 :
        return "No Content" ;
    case 400 :
        return "Bad Request" ;
    case 404 :
        return "Not Found" ;
    case 409 :
        return "Conflict" ;
    case 500 :
        return "Internal server error" ;
    case 501 :
        return "Not implemented" ;
    case 503 :
        return "Service unavailable" ;
    default : 
        return "No reason";
    }
}

/**
 * \~french
 * \brief Méthode commune pour générer l'en-tête HTTP en fonction du status code HTTP
 * \param[in] http_status Code de status HTTP
 * \return élément status de l'en-tête HTTP
 * \~english
 * \brief Common function to generate HTTP headers using the HTTP status code
 * \param[in] http_status HTTP status code
 * \return HTTP header status element
 */
std::string get_status_header ( int http_status ) {
    // Creation de l'en-tete
    std::stringstream out;
    out << http_status;
    return "Status: "+out.str() +" "+get_message_from_http_status ( http_status ) +"\r\n" ;
}

/**
 * \~french
 * \brief Méthode commune pour générer le nom du fichier en fonction du type mime
 * \param[in] mime type mime
 * \return nom du fichier
 * \~english
 * \brief Common function to generate file name using the mime type
 * \param[in] mime mime type
 * \return filename
 */
std::string get_default_filename ( std::string mime ) {
    if ( mime.compare ( "image/tiff" ) ==0 )
        return "image.tif";
    else if ( mime.compare ( "image/geotiff" ) ==0 )
        return "image.tif";
    else if ( mime.compare ( "image/jpeg" ) ==0 )
        return "image.jpg";
    else if ( mime.compare ( "image/png" ) ==0 )
        return "image.png";
    else if ( mime.compare ( "image/x-bil;bits=32" ) ==0 )
        return "image.bil";
    else if ( mime.compare ( "text/plain" ) ==0 )
        return "message.txt";
    else if ( mime.compare ( "text/xml" ) ==0 )
        return "message.xml";
    else if ( mime.compare ( "text/asc" ) ==0 )
        return "file.asc";
    else if ( mime.compare ( "application/xml" ) ==0 )
        return "file.xml";
    else if ( mime.compare ( "application/json" ) ==0 )
        return "file.json";
    else if ( mime.compare ( "application/x-protobuf" ) ==0 )
        return "file.pbf";

    return "file";
}

/**
 * \~french
 * \brief Méthode commune pour afficher les codes d'erreur FCGI
 * \param[in] error code d'erreur
 * \~english
 * \brief Common function to display FCGI error code
 * \param[in] error error code
 */
void print_fcgi_error ( int error ) {
    if ( error>0 )
        BOOST_LOG_TRIVIAL(error) <<   "Code erreur : " << error; // Erreur errno(2) (Cf. manpage 
    else if ( error==FCGX_UNSUPPORTED_VERSION )
        BOOST_LOG_TRIVIAL(error) <<   "Version FCGI non supportee" ;
    else if ( error==FCGX_UNSUPPORTED_VERSION )
        BOOST_LOG_TRIVIAL(error) <<   "Erreur de protocole" ;
    else if ( error==FCGX_CALL_SEQ_ERROR )
        BOOST_LOG_TRIVIAL(error) <<   "Erreur de parametre" ;
    else if ( error==FCGX_UNSUPPORTED_VERSION )
        BOOST_LOG_TRIVIAL(error) <<   "Preconditions non remplies" ;
    else
        BOOST_LOG_TRIVIAL(error) <<   "Erreur inconnue" ;
}

/**
 * \~french
 * \brief Copie d'un flux d'entree dans le flux de sortie de l'objet request de type FCGX_Request
 * \return -1 en cas de problème, 0 sinon
 * \~english
 * \brief Copy a data stream in the FCGX_Request output stream
 * \return -1 if error, else 0
 */
int sendresponse ( DataStream* stream, Request* request ) {

    // Creation de l'en-tete
    std::string statusHeader= get_status_header ( stream->get_http_status() );
    FCGX_PutStr ( statusHeader.data(),statusHeader.size(),request->fcgx_request->out );

    if (stream->get_type() != "") {
        FCGX_PutStr ( "Content-Type: ",14,request->fcgx_request->out );
        FCGX_PutStr ( stream->get_type().c_str(), strlen ( stream->get_type().c_str() ),request->fcgx_request->out );
    }

    if ( stream->get_length() != 0 ) {
        std::stringstream ss;
        ss << stream->get_length();
        std::string lengthStr = ss.str();
        FCGX_PutStr ( "\r\nContent-Length: ",18,request->fcgx_request->out );
        FCGX_PutStr ( lengthStr.c_str(), strlen ( lengthStr.c_str() ),request->fcgx_request->out );
    }

    if (stream->get_type() != "") {
        std::string filename = get_default_filename ( stream->get_type() );
        BOOST_LOG_TRIVIAL(debug) <<  filename ;

        FCGX_PutStr ( "\r\nContent-Disposition: filename=\"",33,request->fcgx_request->out );
        FCGX_PutStr ( filename.data(),filename.size(), request->fcgx_request->out );
        FCGX_PutStr ( "\"",1,request->fcgx_request->out );
    }
    
    if (stream->get_type() != "" || stream->get_length() != 0) {
        FCGX_PutStr ( "\r\n\r\n",4,request->fcgx_request->out );
    } else {
        FCGX_PutStr ( "\r\n",2,request->fcgx_request->out );
    }

    // Copie dans le flux de sortie
    uint8_t *buffer = new uint8_t[2 << 20];
    size_t size_to_read = 2 << 20;
    int pos = 0;

    // Ecriture progressive du flux d'entree dans le flux de sortie
    while ( true ) {
        // Recuperation d'une portion du flux d'entree

        size_t read_size = stream->read ( buffer, size_to_read );
        if ( read_size==0 )
            break;
        int wr = 0;
        // Ecriture iterative de la portion du flux d'entree dans le flux de sortie
        while ( wr < read_size ) {
            // Taille ecrite dans le flux de sortie
            int w = FCGX_PutStr ( ( char* ) ( buffer + wr ), read_size,request->fcgx_request->out );
            if ( w < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<   "Echec d'ecriture dans le flux de sortie de la requete FCGI " << request->fcgx_request->requestId ;
                print_fcgi_error ( FCGX_GetError ( request->fcgx_request->out ) );
                delete stream;
                delete[] buffer;
                return -1;
            }
            wr += w;
        }
        if ( wr != read_size ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Nombre incorrect d'octets ecrits dans le flux de sortie" ;
            delete stream;
            stream = 0;
            delete[] buffer;
            buffer = 0;
            break;
        }
        pos += read_size;
    }
    if ( stream ) {
        delete stream;
    }
    if ( buffer ) {
        delete[] buffer;
    }
    BOOST_LOG_TRIVIAL(debug) <<   "End of Response" ;
    return 0;
}

void Router::process_request(Request* req, Rok4Server* serv) {

    ServicesConfiguration* services = serv->get_services_configuration();
    bool enabled = serv->get_server_configuration()->is_enabled();

    try {

        if (services->get_health_service()->match_request(req)) {
            sendresponse(services->get_health_service()->process_request(req, serv), req);
        }
        else if (services->get_admin_service()->match_request(req)) {
            sendresponse(services->get_admin_service()->process_request(req, serv), req);
        }
        else if (enabled && services->get_common_service()->match_request(req)) {
            sendresponse(services->get_common_service()->process_request(req, serv), req);
        }
        else if (enabled && services->get_tms_service()->match_request(req)) {
            sendresponse(services->get_tms_service()->process_request(req, serv), req);
        }
        else if (enabled && services->get_wmts_service()->match_request(req)) {
            sendresponse(services->get_wmts_service()->process_request(req, serv), req);
        }
        else if (enabled && services->get_tiles_service()->match_request(req)) {
            sendresponse(services->get_tiles_service()->process_request(req, serv), req);
        }
        else if (enabled && services->get_wms_service()->match_request(req)) {
            sendresponse(services->get_wms_service()->process_request(req, serv), req);
        }
        else {
            throw new MessageDataStream("{\"error\": \"Bad Request\", \"error_description\": \"Unknown request path\"}", "application/json", 400);
        }
    }
    catch (MessageDataStream* m) {
        // On attrape un message d'erreur
        BOOST_LOG_TRIVIAL(debug) << "Request failure";
        BOOST_LOG_TRIVIAL(debug) << req->method << " " << req->path;
        sendresponse(m, req);
    }
    catch (...) {
        // On ne devrait pas passer là, c'est un bug du serveur
        BOOST_LOG_TRIVIAL(error) << "Routing issue";
        BOOST_LOG_TRIVIAL(error) << req->method << " " << req->path;
        sendresponse(new MessageDataStream("{\"error\": \"Internal issue\", \"error_description\": \"Routing error\"}", "application/json", 500), req);
    }
}