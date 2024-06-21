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
 * \file DataStreams.h
 * \~french
 * \brief Définition des classes EmptyResponseDataStream et MessageDataStream
 * \~english
 * \brief Define classes EmptyResponseDataStream and MessageDataStream
 */

#ifndef _MESSAGE_
#define _MESSAGE_

#include <string.h>
#include <vector>

#include <rok4/datastream/DataStream.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance de EmptyResponseDataStream définit une réponse vide renvoyée à l'utilisateur.
 * Le message est renvoyé sous forme de flux
 * \brief Gestion des messages sous forme de flux
 * \~english
 * A EmptyResponseDataStream defines an empty response sent to the user.
 * \brief Streamed messages handler
 * \~ \see MessageDataSource
 */
class EmptyResponseDataStream : public DataStream {

public:
    EmptyResponseDataStream() {}

    size_t read ( uint8_t *buffer, size_t size ) {
        return 0;
    }
    bool eof() {
        return true;
    }
    std::string get_type() {
        return "";
    }
    std::string get_encoding() {
        return "";
    }
    int get_http_status() {
        return 204;
    }
    unsigned int get_length(){
        return 0;
    }
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance de MessageDataStream définit un message renvoyé à l'utilisateur.
 * Le message est renvoyé sous forme de flux
 * \brief Gestion des messages sous forme de flux
 * \~english
 * A MessageDataStream defines a message sent to the user.
 * \brief Streamed messages handler
 * \~ \see MessageDataSource
 */
class MessageDataStream : public DataStream {
private:
    /**
     * \~french Type MIME du message
     * \~english MIME type
     */
    std::string type;
    
    /**
     * \~french Position courante dans le flux
     * \~english Current stream position
     */
    uint32_t pos;
    
    /**
     * \~french Statut HTTP
     * \~english HTTP status
     */
    int http_status;

protected:
    /**
     * \~french Message à envoyer
     * \~english Sended message
     */
    std::string message;

public:
    MessageDataStream ( std::string m, std::string t, int s ) : message ( m ), type ( t ), http_status(s), pos ( 0 ) {}

    size_t read ( uint8_t *buffer, size_t size ) {
        if ( size > message.length() - pos ) size = message.length() - pos;
        memcpy ( buffer, ( uint8_t* ) ( message.data() +pos ),size );
        pos+=size;
        return size;
    }
    bool eof() {
        return ( pos==message.length() );
    }
    std::string get_type() {
        return type;
    }
    std::string get_encoding() {
        return "";
    }
    int get_http_status() {
        return http_status;
    }
    unsigned int get_length(){
        return message.length();
    }
};

#endif
