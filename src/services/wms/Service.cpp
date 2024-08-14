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

#include <rok4/utils/CRS.h>

#include "services/wms/Exception.h"
#include "services/wms/Service.h"
#include "Rok4Server.h"

WmsService::WmsService (json11::Json& doc, ServicesConfiguration* svc) : Service(doc), metadata(NULL) {

    if (! is_ok()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        error_message = "WMS service: " + error_message;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    if (doc["name"].is_string()) {
        name = doc["name"].string_value();
    } else if (! doc["name"].is_null()) {
        error_message = "WMS service: name have to be a string";
        return;
    } else {
        name = "WMS";
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

    if (doc["formats"].is_array()) {
        for (json11::Json f : doc["formats"].array_items()) {
            if (f.is_string()) {
                std::string format = f.string_value();
                if ( format != "image/jpeg" &&
                    format != "image/png"  &&
                    format != "image/tiff" &&
                    format != "image/geotiff" &&
                    format != "image/x-bil;bits=32" &&
                    format != "image/gif" && 
                    format != "text/asc" ) {
                    error_message = "WMS service: format [" + format + "] is not an handled MIME format";
                    return;
                } else {
                    formats.push_back ( format );
                }
                formats.push_back ( f.string_value() );
            } else {
                error_message = "WMS service: formats have to be a string array";
                return;
            }
        }
    } else if (! doc["formats"].is_null()) {
        error_message = "WMS service: formats have to be a string array";
        return;
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

    if (doc["root_layer"].is_object()) {
        if (doc["root_layer"]["title"].is_string()) {
            root_layer_title = doc["root_layer"]["title"].string_value();
        } else if (! doc["root_layer"]["title"].is_null()) {
            error_message = "WMS service: root_layer.title have to be a string";
            return;
        } else {
            root_layer_title = "WMS layers";
        }

        if (doc["root_layer"]["abstract"].is_string()) {
            root_layer_abstract = doc["root_layer"]["abstract"].string_value();
        } else if (! doc["root_layer"]["abstract"].is_null()) {
            error_message = "WMS service: root_layer.abstract have to be a string";
            return;
        } else {
            root_layer_abstract = "WMS layers";
        }
    } else if (! doc["root_layer"].is_null()) {
        error_message = "WMS service: root_layer have to be an object";
        return;
    } else {
        root_layer_title = "WMS layers";
        root_layer_abstract = "WMS layers";
    }

    if (doc["limits"].is_object()) {
        if (doc["limits"]["layers_count"].is_number() && doc["limits"]["layers_count"].number_value() >= 1) {
            max_layers_count = doc["limits"]["layers_count"].number_value();
        } else if (! doc["limits"]["layers_count"].is_null()) {
            error_message = "WMS service: limits.layers_count have to be an integer >= 1";
            return;
        } else {
            max_layers_count = 1;
        }

        if (doc["limits"]["width"].is_number() && doc["limits"]["width"].number_value() >= 1) {
            max_width = doc["limits"]["width"].number_value();
        } else if (! doc["limits"]["width"].is_null()) {
            error_message = "WMS service: limits.width have to be an integer >= 1";
            return;
        } else {
            max_width = 5000;
        }

        if (doc["limits"]["height"].is_number() && doc["limits"]["height"].number_value() >= 1) {
            max_height = doc["limits"]["height"].number_value();
        } else if (! doc["limits"]["height"].is_null()) {
            error_message = "WMS service: limits.height have to be an integer >= 1";
            return;
        } else {
            max_height = 5000;
        }

        if (doc["limits"]["tile_x"].is_number() && doc["limits"]["tile_x"].number_value() >= 1) {
            max_tile_x = doc["limits"]["tile_x"].number_value();
        } else if (! doc["limits"]["tile_x"].is_null()) {
            error_message = "WMS service: limits.tile_x have to be an integer >= 1";
            return;
        } else {
            max_tile_x = 32;
        }

        if (doc["limits"]["tile_y"].is_number() && doc["limits"]["tile_y"].number_value() >= 1) {
            max_tile_y = doc["limits"]["tile_y"].number_value();
        } else if (! doc["limits"]["tile_y"].is_null()) {
            error_message = "WMS service: limits.tile_y have to be an integer >= 1";
            return;
        } else {
            max_tile_y = 32;
        }

    } else if (! doc["limits"].is_null()) {
        error_message = "WMS service: limits have to be an object";
        return;
    } else {
        max_layers_count = 1;
        max_width = 5000;
        max_height = 5000;
        max_tile_x = 32;
        max_tile_y = 32;
    }

    bool crs84_present = false;
    if (doc["crs"].is_array()) {
        for (json11::Json c : doc["crs"].array_items()) {
            if (c.is_string()) {
                std::string crs_string = c.string_value();

                CRS* crs = new CRS( crs_string );
                if ( ! crs->is_define() ) {
                    BOOST_LOG_TRIVIAL(warning) << "The (WMS) CRS [" << crs_string <<"] is not present in PROJ"  ;
                    continue;
                }

                BOOST_LOG_TRIVIAL(info) <<  "Adding global CRS " << crs->get_request_code()   ;
                crss.push_back(crs);
                if (crs->get_request_code() == "CRS:84") {
                    crs84_present = true;
                }

                if (svc->handle_crs_equivalences()) {
                    std::vector<CRS*> eqs = svc->get_equals_crs(crs->get_request_code());
                    size_t init_size = crss.size();
                    for (unsigned int e = 0; e < eqs.size(); e++) {
                        bool already_in = false;
                        for ( int i = 0; i < init_size ; i++ ) {
                            if (crss.at( i )->cmp_request_code(eqs.at(e)->get_request_code() ) ){
                                already_in = true;
                                break;
                            }
                        }
                        if (! already_in) {
                            BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqs.at(e)->get_request_code() <<"] of [" << crs->get_request_code() << "]"  ;
                            // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
                            crss.push_back(new CRS(eqs.at(e)));
                            if (eqs.at(e)->get_request_code() == "CRS:84") {
                                crs84_present = true;
                            }
                        }
                    }
                }
            } else {
                error_message = "WMS service: crs have to be a string array";
                return;
            }
        }
    } else if (! doc["crs"].is_null()) {
        error_message = "WMS service: crs have to be a string array";
        return;
    }

    if (! crs84_present) {
        BOOST_LOG_TRIVIAL(info) <<  "CRS:84 not found -> adding global CRS CRS:84"   ;
        CRS* crs = new CRS( "CRS:84" );

        if ( ! crs->is_define() ) {
            error_message = "WMS service: The CRS [CRS:84] is not present in PROJ"  ;
            return;
        }

        crss.push_back ( crs );

        if (svc->handle_crs_equivalences()) {
            std::vector<CRS*> eqs = svc->get_equals_crs(crs->get_request_code());
            size_t init_size = crss.size();
            for (unsigned int e = 0; e < eqs.size(); e++) {
                bool already_in = false;
                for ( int i = 0; i < init_size ; i++ ) {
                    if (crss.at( i )->cmp_request_code(eqs.at(e)->get_request_code() ) ){
                        already_in = true;
                    }
                }
                if (! already_in) {
                    BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqs.at(e)->get_request_code() <<"] of [CRS:84]"  ;
                    // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
                    crss.push_back(new CRS(eqs.at(e)));
                }
            }
        }
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
