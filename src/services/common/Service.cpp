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
 * \file services/common/Service.cpp
 ** \~french
 * \brief Implémentation de la classe CommonService
 ** \~english
 * \brief Implements classe CommonService
 */

#include <iostream>

#include "services/common/Service.h"
#include "services/common/Exception.h"

#include "Rok4Server.h"

CommonService::CommonService (json11::Json& doc) : Service(doc) {

    if (! isOk()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        errorMessage = "COMMON service: " + errorMessage;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else if (! doc["title"].is_null()) {
        errorMessage = "COMMON service: title have to be a string";
        return;
    } else {
        title = "COMMON service";
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else if (! doc["abstract"].is_null()) {
        errorMessage = "COMMON service: abstract have to be a string";
        return;
    } else {
        abstract = "COMMON service";
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                errorMessage = "COMMON service: keywords have to be a string array";
                return;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        errorMessage = "COMMON service: keywords have to be a string array";
        return;
    }

    if (doc["endpoint_uri"].is_string()) {
        endpoint_uri = doc["endpoint_uri"].string_value();
    } else if (! doc["endpoint_uri"].is_null()) {
        errorMessage = "COMMON service: endpoint_uri have to be a string";
        return;
    } else {
        endpoint_uri = "http://localhost/common";
    }

    if (doc["root_path"].is_string()) {
        root_path = doc["root_path"].string_value();
    } else if (! doc["root_path"].is_null()) {
        errorMessage = "COMMON service: root_path have to be a string";
        return;
    } else {
        root_path = "/common";
    }
}

DataStream* CommonService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "COMMON service";

    if ( match_route( "", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETSERVICES request";
        return get_services(req, serv);
    } else {
        throw CommonException::get_error_message("Unknown common request path", 400);
    }
};


DataStream* CommonService::get_services ( Request* req, Rok4Server* serv ) {

    return CommonException::get_error_message("Coming soon !", 501);
    // std::ostringstream res;
    // res << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    // res << "<Services>";
    // if (serv->) {
    //     res << "<TileMapService title=\"" << serv->getServicesConf()->title << "\" version=\"1.0.0\" href=\"" << serv->getServicesConf()->tmsPublicUrl << "/1.0.0/\" />";
    // }
    // if (serv->supportWms()) {
    //     res << "<WebMapService title=\"" << serv->getServicesConf()->title << "\" version=\"1.3.0\" href=\"" << serv->getServicesConf()->wmsPublicUrl << "?SERVICE=WMS&amp;VERSION=1.3.0&amp;REQUEST=GetCapabilities\" />";
    // }
    // if (serv->supportWmts()) {
    //     res << "<WebMapTileService title=\"" << serv->getServicesConf()->title << "\" version=\"1.0.0\" href=\"" << serv->getServicesConf()->wmtsPublicUrl << "?SERVICE=WMTS&amp;VERSION=1.0.0&amp;REQUEST=GetCapabilities\" />";
    // }
    // res << "</Services>";

    // return new MessageDataStream ( res.str(), "application/xml", 200 );
}