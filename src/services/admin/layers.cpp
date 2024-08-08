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
 * \file services/admin/layers.cpp
 ** \~french
 * \brief Implémentation de la classe AdminService
 ** \~english
 * \brief Implements classe AdminService
 */

#include "services/admin/Service.h"
#include "services/admin/Exception.h"

#include "Rok4Server.h"

DataStream* AdminService::add_layer ( Request* req, Rok4Server* serv ) {


    std::string str_layer = req->path_params.at(0);

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);

    if ( layer != NULL ) throw AdminException::get_error_message("Layer " + str_layer +" already exists.", "Configuration conflict", 409);

    layer = new Layer( str_layer, req->body );
    if ( ! layer->is_ok() ) {
        std::string msg = layer->get_error_message();
        delete layer;
        throw AdminException::get_error_message(msg, "Configuration issue", 400);
    }

    serv->get_server_configuration()->add_layer ( layer );

    return new EmptyResponseDataStream ();
}

DataStream* AdminService::update_layer ( Request* req, Rok4Server* serv ) {

    std::string str_layer = req->path_params.at(0);

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);

    if ( layer == NULL ) throw AdminException::get_error_message("Layer " + str_layer + " does not exists.", "Not found", 404);

    Layer* new_layer = new Layer( str_layer, req->body );
    if ( ! new_layer->is_ok() ) {
        std::string msg = new_layer->get_error_message();
        delete new_layer;
        throw AdminException::get_error_message(msg, "Configuration issue", 400);
    }

    serv->get_server_configuration()->delete_layer ( layer->get_id() );
    serv->get_server_configuration()->add_layer ( new_layer );

    return new EmptyResponseDataStream ();

}

DataStream* AdminService::delete_layer ( Request* req, Rok4Server* serv ) {

    std::string str_layer = req->path_params.at(0);

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);

    if ( layer == NULL ) throw AdminException::get_error_message("Layer " + str_layer + " does not exists.", "Not found", 404);

    serv->get_server_configuration()->delete_layer ( layer->get_id() );

    return new EmptyResponseDataStream ();

}