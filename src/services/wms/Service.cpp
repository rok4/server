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
 * \file services/wms/Service.cpp
 ** \~french
 * \brief Implémentation de la classe WmsService
 ** \~english
 * \brief Implements classe WmsService
 */

#include <iostream>

#include "services/wms/Exception.h"
#include "services/wms/Service.h"
#include "Rok4Server.h"

WmsService::WmsService (json11::Json& doc) : Service(doc), metadata(NULL) {

    if (! is_ok()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        error_message = "WMTS service: " + error_message;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else if (! doc["title"].is_null()) {
        error_message = "WMS service: title have to be a string";
        return;
    } else {
        title = "WMS service";
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else if (! doc["abstract"].is_null()) {
        error_message = "WMS service: abstract have to be a string";
        return;
    } else {
        abstract = "WMS service";
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                error_message = "WMS service: keywords have to be a string array";
                return;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        error_message = "WMS service: keywords have to be a string array";
        return;
    }

    if (doc["endpoint_uri"].is_string()) {
        endpoint_uri = doc["endpoint_uri"].string_value();
    } else if (! doc["endpoint_uri"].is_null()) {
        error_message = "WMS service: endpoint_uri have to be a string";
        return;
    } else {
        endpoint_uri = "http://localhost/wms";
    }

    if (doc["root_path"].is_string()) {
        root_path = doc["root_path"].string_value();
    } else if (! doc["root_path"].is_null()) {
        error_message = "WMS service: root_path have to be a string";
        return;
    } else {
        root_path = "/wms";
    }

    if (doc["metadata"].is_object()) {
        metadata = new Metadata ( doc["metadata"] );
        if (metadata->get_missing_field() != "") {
            error_message = "WMS service: invalid metadata: have to own a field " + metadata->get_missing_field();
            return ;
        }
    }

    if (doc["reprojection"].is_bool()) {
        reprojection = doc["reprojection"].bool_value();
    } else if (! doc["reprojection"].is_null()) {
        error_message = "WMS service: reprojection have to be a boolean";
        return;
    } else {
        reprojection = false;
    }

    if (doc["info_formats"].is_array()) {
        for (json11::Json info : doc["info_formats"].array_items()) {
            if (info.is_string()) {
                info_formats.push_back ( info.string_value() );
            } else {
                error_message = "WMS service: info_formats have to be a string array";
                return;
            }
        }
    } else if (! doc["info_formats"].is_null()) {
        error_message = "WMS service: info_formats have to be a string array";
        return;
    }
}

DataStream* WmsService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "WMS service";

    // On contrôle le service précisé en paramètre de requête
    std::string param_service = req->get_query_param("service");
    std::transform(param_service.begin(), param_service.end(), param_service.begin(), ::tolower);

    if (param_service != "wms") {
        throw WmsException::get_error_message("SERVICE query parameter have to be WMS", "InvalidParameterValue", 400);
    }

    // On ne gère que la version 1.0.0
    std::string param_version = req->get_query_param("version");
    if (param_version != "1.0.0" && param_version != "") {
        throw WmsException::get_error_message("VERSION query parameter have to be 1.3.0 or empty", "InvalidParameterValue", 400);
    }

    // On récupère le type de requête précisé en paramètre de requête
    std::string param_request = req->get_query_param("request");
    std::transform(param_request.begin(), param_request.end(), param_request.begin(), ::tolower);
    
    if (param_request == "getcapabilities") {
        BOOST_LOG_TRIVIAL(debug) << "GETCAPABILITIES request";
        return get_capabilities(req, serv);
    } else if (param_request == "getfeatureinfo") {
        BOOST_LOG_TRIVIAL(debug) << "GETFEATUREINFO request";
        return get_feature_info(req, serv);
    } else if (param_request == "getmap") {
        BOOST_LOG_TRIVIAL(debug) << "GETMAP request";
        return new DataStreamFromDataSource(get_map(req, serv));
    } else {
        throw WmsException::get_error_message("REQUEST query parameter unknown", "OperationNotSupported", 400);
    }

};
