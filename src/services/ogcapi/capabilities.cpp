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

#include "services/ogcapi/Exception.h"
#include "services/ogcapi/Service.h"
#include "core/Rok4Server.h"


DataStream* OgcApiService::get_landing_page ( Request* req, Rok4Server* serv ) {
    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw OgcApiException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    std::vector<json11::Json> links;

    links.push_back(json11::Json::object {
        { "href", endpoint_uri},
        { "rel", "self"},
        { "type", "application/json"},
        { "title", "this document"}
    });

    links.push_back(json11::Json::object {
        { "href", endpoint_uri + "/collections?f=json"},
        { "rel", "data"},
        { "type", "application/json"},
        { "title", "Information about the collections"}
    });

    links.push_back(json11::Json::object {
        { "href", endpoint_uri + "/conformance?f=json"},
        { "rel", "data"},
        { "type", "application/json"},
        { "title", "OGC API conformance classes implemented by this service"}
    });

    json11::Json::object res = json11::Json::object {
        { "title", title },
        { "description", abstract },
        { "links", links }
    };

    return new MessageDataStream ( json11::Json{ res }.dump(), "application/json", 200 );
}

DataStream* OgcApiService::get_conformance ( Request* req, Rok4Server* serv ) {
    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw OgcApiException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    json11::Json::object res = json11::Json::object {
        { "conformsTo", json11::Json::array{
            "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
            "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/json",
            "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/oas30",
            "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/core",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/tileset",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/tilesets-list",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/geodata-tilesets",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/dataset-tilesets",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/jpeg",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/png",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/mvt",
            "http://www.opengis.net/spec/ogcapi-tiles-1/1.0/conf/tiff"
        } }
    };

    return new MessageDataStream ( json11::Json{ res }.dump(), "application/json", 200 );
}
