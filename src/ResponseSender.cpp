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
 * \file ResponseSender.cpp
 * \~french
 * \brief Implémentation des fonctions d'envoie de réponse sur le flux FCGI
 * \~english
 * \brief Implement response sender function for the FCGI link
 */

#include "ResponseSender.h"
#include "ServiceException.h"
#include "Message.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include <stdio.h>
#include <string.h> // pour strlen
#include <sstream> // pour les stringstream
#include "config.h"
/**
 * \~french
 * \brief Méthode commune pour générer l'en-tête HTTP en fonction du status code HTTP
 * \param[in] statusCode Code de status HTTP
 * \return élément status de l'en-tête HTTP
 * \~english
 * \brief Common function to generate HTTP headers using the HTTP status code
 * \param[in] statusCode HTTP status code
 * \return HTTP header status element
 */
std::string genStatusHeader ( int statusCode ) {
    // Creation de l'en-tete
    std::stringstream out;
    out << statusCode;
    std::string statusHeader= "Status: "+out.str() +" "+ServiceException::getStatusCodeAsReasonPhrase ( statusCode ) +"\r\n" ;
    return statusHeader ;
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
std::string genFileName ( std::string mime, Request* request ) {

    if (request->hasParam("filename")) {
        return request->getParam("filename");
    }

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
void displayFCGIError ( int error ) {
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

int ResponseSender::sendresponse ( DataSource* source, FCGX_Request* fcgx_request, Request* request ) {
    // Creation de l'en-tete
    std::string statusHeader = genStatusHeader ( source->getHttpStatus() );
    FCGX_PutStr ( statusHeader.data(),statusHeader.size(),fcgx_request->out );

    if (source->getType() != "") {
        FCGX_PutStr ( "Content-Type: ",14,fcgx_request->out );
        FCGX_PutStr ( source->getType().c_str(), strlen ( source->getType().c_str() ),fcgx_request->out );
    }

    if ( ! source->getEncoding().empty() ){
        FCGX_PutStr ( "\r\nContent-Encoding: ",20,fcgx_request->out );
        FCGX_PutStr ( source->getEncoding().c_str(), strlen ( source->getEncoding().c_str() ),fcgx_request->out );
    }
    if ( source->getLength() != 0 ){
        std::stringstream ss;
        ss << source->getLength();
        std::string lengthStr = ss.str();
        FCGX_PutStr ( "\r\nContent-Length: ",18,fcgx_request->out );
        FCGX_PutStr ( lengthStr.c_str(), strlen ( lengthStr.c_str() ),fcgx_request->out );
    }

    if (source->getType() != "") {
        std::string filename = genFileName ( source->getType(), request );
        BOOST_LOG_TRIVIAL(debug) <<  filename ;
        
        FCGX_PutStr ( "\r\nContent-Disposition: ",23,fcgx_request->out );
        if (request->hasParam("filename")) {
            FCGX_PutStr ( "attachment; ",12,fcgx_request->out );
        }
        FCGX_PutStr ( "filename=\"",10,fcgx_request->out );
        FCGX_PutStr ( filename.data(),filename.size(), fcgx_request->out );
        FCGX_PutStr ( "\"",1,fcgx_request->out );
    }

    if (source->getType() != "" || source->getLength() != 0) {
        FCGX_PutStr ( "\r\n\r\n",4,fcgx_request->out );
    } else {
        FCGX_PutStr ( "\r\n",2,fcgx_request->out );
    }

    // Copie dans le flux de sortie
    size_t buffer_size;
    const uint8_t *buffer = source->getData ( buffer_size );
    int wr = 0;
    // Ecriture iterative de la source de donnees dans le flux de sortie
    while ( wr < buffer_size ) {
        // Taille ecrite dans le flux de sortie
        int w = FCGX_PutStr ( ( char* ) ( buffer + wr ), buffer_size,fcgx_request->out );
        if ( w < 0 ) {
            BOOST_LOG_TRIVIAL(error) <<   "Echec d'ecriture dans le flux de sortie de la requete FCGI " << fcgx_request->requestId ;
            displayFCGIError ( FCGX_GetError ( fcgx_request->out ) );
            delete source;
            //delete[] buffer;
            return -1;
        }
        wr += w;
    }
    delete source;
    BOOST_LOG_TRIVIAL(debug) <<   "End of Response" ;
    return 0;
}

int ResponseSender::sendresponse ( DataStream* stream, FCGX_Request* fcgx_request, Request* request ) {

    // Creation de l'en-tete
    std::string statusHeader= genStatusHeader ( stream->getHttpStatus() );
    FCGX_PutStr ( statusHeader.data(),statusHeader.size(),fcgx_request->out );

    if (stream->getType() != "") {
        FCGX_PutStr ( "Content-Type: ",14,fcgx_request->out );
        FCGX_PutStr ( stream->getType().c_str(), strlen ( stream->getType().c_str() ),fcgx_request->out );
    }

    if ( stream->getLength() != 0 ) {
        std::stringstream ss;
        ss << stream->getLength();
        std::string lengthStr = ss.str();
        FCGX_PutStr ( "\r\nContent-Length: ",18,fcgx_request->out );
        FCGX_PutStr ( lengthStr.c_str(), strlen ( lengthStr.c_str() ),fcgx_request->out );
    }

    if (stream->getType() != "") {
        std::string filename = genFileName ( stream->getType(), request );
        BOOST_LOG_TRIVIAL(debug) <<  filename ;

        FCGX_PutStr ( "\r\nContent-Disposition: ",23,fcgx_request->out );
        if (request->hasParam("filename")) {
            FCGX_PutStr ( "attachment; ",12,fcgx_request->out );
        }
        FCGX_PutStr ( "filename=\"",10,fcgx_request->out );
        FCGX_PutStr ( filename.data(),filename.size(), fcgx_request->out );
        FCGX_PutStr ( "\"",1,fcgx_request->out );
    }
    
    if (stream->getType() != "" || stream->getLength() != 0) {
        FCGX_PutStr ( "\r\n\r\n",4,fcgx_request->out );
    } else {
        FCGX_PutStr ( "\r\n",2,fcgx_request->out );
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
            int w = FCGX_PutStr ( ( char* ) ( buffer + wr ), read_size,fcgx_request->out );
            if ( w < 0 ) {
                BOOST_LOG_TRIVIAL(error) <<   "Echec d'ecriture dans le flux de sortie de la requete FCGI " << fcgx_request->requestId ;
                displayFCGIError ( FCGX_GetError ( fcgx_request->out ) );
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
