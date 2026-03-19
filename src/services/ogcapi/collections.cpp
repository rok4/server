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
 * \file services/ogcapi/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe OgcApiService
 ** \~english
 * \brief Implements classe OgcApiService
 */

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "services/ogcapi/Exception.h"
#include "services/ogcapi/Service.h"
#include "core/Rok4Server.h"


DataStream* OgcApiService::get_collections ( Request* req, ServicesConfiguration* services ) {

    // format
    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw OgcApiException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    // bbox
    std::string str_bbox = req->get_query_param("bbox");
    bool bbox_provided = false;
    BoundingBox<double> bbox;
    if (str_bbox != "") {
        bbox_provided = true;

        std::vector<std::string> vector_bbox;
        boost::split(vector_bbox, str_bbox, boost::is_any_of(","));
        if (vector_bbox.size() != 4) throw OgcApiException::get_error_message("InvalidParameter", "Bbox invalid", 400);
        
        double bb[4];
        for ( int i = 0; i < 4; i++ ) {
            if ( sscanf ( vector_bbox[i].c_str(),"%lf",&bb[i] ) !=1 )
                throw OgcApiException::get_error_message("InvalidParameter", "Bbox invalid", 400);
            //Test NaN values
            if (bb[i] != bb[i])
                throw OgcApiException::get_error_message("InvalidParameter", "Bbox invalid", 400);
        }
        if ( bb[0] >= bb[2] || bb[1] >= bb[3] )
            throw OgcApiException::get_error_message("InvalidParameter", "Bbox invalid", 400);

        bbox.xmin=bb[0];
        bbox.ymin=bb[1];
        bbox.xmax=bb[2];
        bbox.ymax=bb[3];
    }

    if ( ! cache_getcapabilities.empty() && ! bbox_provided) {
        return new MessageDataStream ( cache_getcapabilities, "application/json", 200 );
    }

    std::vector<json11::Json> links;
    if (bbox_provided) {
        links.push_back(json11::Json::object {
            { "href", endpoint_uri + "/collections?f=json&bbox=" + str_bbox},
            { "rel", "self"},
            { "type", "application/json"},
            { "title", "this document"}
        });
    } else {
        links.push_back(json11::Json::object {
            { "href", endpoint_uri + "/collections?f=json"},
            { "rel", "self"},
            { "type", "application/json"},
            { "title", "this document"}
        });
    }

    if (metadata) {
        links.push_back(metadata->to_json_ogcapi("Service metadata", "describedby"));
    }

    std::vector<json11::Json> collections;

    std::map<std::string, Layer*>::iterator layers_iterator ( services->get_layers().begin() ), layers_end ( services->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        if (layers_iterator->second->is_ogcapi_enabled() && (! bbox_provided || layers_iterator->second->get_geographical_bbox().intersects(bbox))) {
            collections.push_back(layers_iterator->second->to_json_ogcapi(this));
        }
    }

    json11::Json::object res = json11::Json::object {
        { "links", links },
        { "numberMatched", (int) collections.size() },
        { "numberReturned", (int) collections.size() }
    };

    res["collections"] = collections;

    if (! bbox_provided) {
        cache_mtx.lock();
        cache_getcapabilities = json11::Json{ res }.dump();
        cache_mtx.unlock();
        return new MessageDataStream ( cache_getcapabilities, "application/json", 200 );
    }
    return new MessageDataStream ( json11::Json{ res }.dump(), "application/json", 200 ); 
}


DataStream* OgcApiService::get_collection ( Request* req, ServicesConfiguration* services ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw OgcApiException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    // La couche
    std::string str_layer = req->path_params.at(0);
    if ( contain_chars(str_layer, "\"")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in collection: " << str_layer ;
        throw OgcApiException::get_error_message("ResourceNotFound", "Layer unknown", 404);
    }

    Layer* layer = services->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_ogcapi_enabled() ) {
        throw OgcApiException::get_error_message("ResourceNotFound", "Layer "+str_layer+" unknown", 404);
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_ogcapi(this) }.dump(), "application/json", 200 );
}
