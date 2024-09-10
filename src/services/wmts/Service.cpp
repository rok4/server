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
 * \file services/wmts/Service.cpp
 ** \~french
 * \brief Implémentation de la classe WmtsService
 ** \~english
 * \brief Implements classe WmtsService
 */

#include <iostream>

#include <rok4/datasource/PaletteDataSource.h>

#include "services/wmts/Exception.h"
#include "services/wmts/Service.h"
#include "Rok4Server.h"

WmtsService::WmtsService (json11::Json& doc) : Service(doc), metadata(NULL) {

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
        error_message = "WMTS service: title have to be a string";
        return;
    } else {
        title = "WMTS service";
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else if (! doc["abstract"].is_null()) {
        error_message = "WMTS service: abstract have to be a string";
        return;
    } else {
        abstract = "WMTS service";
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                error_message = "WMTS service: keywords have to be a string array";
                return;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        error_message = "WMTS service: keywords have to be a string array";
        return;
    }

    if (doc["endpoint_uri"].is_string()) {
        endpoint_uri = doc["endpoint_uri"].string_value();
    } else if (! doc["endpoint_uri"].is_null()) {
        error_message = "WMTS service: endpoint_uri have to be a string";
        return;
    } else {
        endpoint_uri = "http://localhost/wmts";
    }

    if (doc["root_path"].is_string()) {
        root_path = doc["root_path"].string_value();
    } else if (! doc["root_path"].is_null()) {
        error_message = "WMTS service: root_path have to be a string";
        return;
    } else {
        root_path = "/wmts";
    }

    if (doc["metadata"].is_object()) {
        metadata = new Metadata ( doc["metadata"] );
        if (metadata->get_missing_field() != "") {
            error_message = "WMTS service: invalid metadata: have to own a field " + metadata->get_missing_field();
            return ;
        }
    }

    if (doc["reprojection"].is_bool()) {
        reprojection = doc["reprojection"].bool_value();
    } else if (! doc["reprojection"].is_null()) {
        error_message = "WMTS service: reprojection have to be a boolean";
        return;
    } else {
        reprojection = false;
    }

    if (doc["inspire"].is_bool()) {
        default_inspire = doc["inspire"].bool_value();
    } else if (! doc["inspire"].is_null()) {
        error_message = "WMTS service: inspire have to be a boolean";
        return;
    } else {
        default_inspire = false;
    }

    info_formats.push_back("text/plain");
    info_formats.push_back("text/xml");
    info_formats.push_back("text/html");
    info_formats.push_back("application/json");
}

DataStream* WmtsService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "WMTS service";

    // On contrôle le service précisé en paramètre de requête
    std::string param_service = req->get_query_param("service");

    std::transform(param_service.begin(), param_service.end(), param_service.begin(), ::tolower);

    if (param_service == "") {
        throw WmtsException::get_error_message("SERVICE query parameter missing", "MissingParameterValue", 400);
    }

    if (param_service != "wmts") {
        throw WmtsException::get_error_message("SERVICE query parameter have to be WMTS", "InvalidParameterValue", 400);
    }

    // On ne gère que la version 1.0.0
    std::string param_version = req->get_query_param("version");
    if (param_version != "1.0.0" && param_version != "") {
        throw WmtsException::get_error_message("VERSION query parameter have to be 1.0.0 or empty", "InvalidParameterValue", 400);
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
    } else if (param_request == "gettile") {
        BOOST_LOG_TRIVIAL(debug) << "GETTILE request";
        return get_tile(req, serv);
    } else {
        throw WmtsException::get_error_message("REQUEST query parameter unknown", "OperationNotSupported", 400);
    }

};

bool WmtsService::is_available_infoformat(std::string f) {
    return (std::find(info_formats.begin(), info_formats.end(), f) != info_formats.end());
}

std::vector<std::string>* WmtsService::get_available_infoformats() {
    return &info_formats;
}