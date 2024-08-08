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
 * \file services/tiles/Service.cpp
 ** \~french
 * \brief Implémentation de la classe TilesService
 ** \~english
 * \brief Implements classe TilesService
 */

#include <iostream>

#include "services/tiles/Exception.h"
#include "services/tiles/Service.h"
#include "Rok4Server.h"

TilesService::TilesService (json11::Json& doc) : Service(doc), metadata(NULL) {

    if (! is_ok()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        error_message = "TILES service: " + error_message;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else if (! doc["title"].is_null()) {
        error_message = "TILES service: title have to be a string";
        return;
    } else {
        title = "TILES service";
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else if (! doc["abstract"].is_null()) {
        error_message = "TILES service: abstract have to be a string";
        return;
    } else {
        abstract = "TILES service";
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                error_message = "TILES service: keywords have to be a string array";
                return;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        error_message = "TILES service: keywords have to be a string array";
        return;
    }

    if (doc["endpoint_uri"].is_string()) {
        endpoint_uri = doc["endpoint_uri"].string_value();
    } else if (! doc["endpoint_uri"].is_null()) {
        error_message = "TILES service: endpoint_uri have to be a string";
        return;
    } else {
        endpoint_uri = "http://localhost/tiles";
    }

    if (doc["root_path"].is_string()) {
        root_path = doc["root_path"].string_value();
    } else if (! doc["root_path"].is_null()) {
        error_message = "TILES service: root_path have to be a string";
        return;
    } else {
        root_path = "/tiles";
    }

    if (doc["metadata"].is_object()) {
        metadata = new Metadata ( doc["metadata"] );
        if (metadata->get_missing_field() != "") {
            error_message = "TILES service: invalid metadata: have to own a field " + metadata->get_missing_field();
            return ;
        }
    }
}

DataStream* TilesService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "TILES service";

    if ( match_route( "/tiles/collections", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETCAPABILITIES request";
        return get_capabilities(req, serv);
    }
    else if ( match_route( "/collections/([^/]+)/styles/([^/]+)/map/tiles/([^/]+)/([^/]+)/([^/]+)/([^/]+)/info", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETFEATUREINFO request";
        return get_feature_info(req, serv);
    }
    else if ( match_route( "/collections/([^/]+)/styles/([^/]+)/map/tiles/([^/]+)/([^/]+)/([^/]+)/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETTILE request";
        return new DataStreamFromDataSource(get_tile(req, serv));
    } else {
        throw TilesException::get_error_message("Unknown tms request path", 400);
    }
};
