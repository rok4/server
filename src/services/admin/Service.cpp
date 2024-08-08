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
 * \file services/admin/Service.cpp
 ** \~french
 * \brief Implémentation de la classe AdminService
 ** \~english
 * \brief Implements classe AdminService
 */

#include "services/admin/Service.h"
#include "services/admin/Exception.h"

#include "Rok4Server.h"

AdminService::AdminService (json11::Json& doc) : Service(doc) {

    if (! is_ok()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        error_message = "ADMIN service: " + error_message;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    title = "ADMIN service";
    abstract = "ADMIN service";
    keywords.push_back(Keyword ( "administration" ));
    root_path = "/admin";
}

DataStream* AdminService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "ADMIN service";

    if ( match_route( "/admin/layers/([^/]+)", {"POST"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "ADDLAYER request";
        return add_layer(req, serv);
    }
    else if ( match_route( "/admin/layers/([^/]+)", {"PUT"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "UPDATELAYER request";
        return update_layer(req, serv);
    }
    else if ( match_route( "/admin/layers/([^/]+)", {"DELETE"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "DELETELAYER request";
        return delete_layer(req, serv);
    }
    else if ( match_route( "/depends", {"PUT"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "TURNON request";
        return turn_on(req, serv);
    }
    else if ( match_route( "/depends", {"PUT"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "TURNOFF request";
        return turn_off(req, serv);
    } else {
        throw AdminException::get_error_message("Unknown admin request path", "Operation not supported", 400);
    }
};
