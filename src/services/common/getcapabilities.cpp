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
 * \file services/common/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe CommonService
 ** \~english
 * \brief Implements classe CommonService
 */

#include <iostream>

#include "services/common/Exception.h"
#include "services/common/Service.h"
#include "Rok4Server.h"

DataStream* CommonService::get_landing_page ( Request* req, Rok4Server* serv ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw CommonException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    ServicesConfiguration* services = serv->get_services_configuration();

    if ( ! cache_getlandingpage.empty()) {
        return new MessageDataStream ( cache_getlandingpage, "application/json", 200 );
    }

    std::vector<json11::Json> links;

    links.push_back(json11::Json::object {
        { "href", endpoint_uri},
        { "rel", "self"},
        { "type", "application/json"},
        { "title", "this document"}
    });

    if (services->get_wms_service()->is_enabled()) {
        links.push_back(json11::Json::object {
            { "href", services->get_wms_service()->get_endpoint_uri() + "?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.3.0"},
            { "rel", "data"},
            { "type", "application/xml"},
            { "title", "OGC Web Map Service capabilities"}
        });
    }

    if (services->get_wmts_service()->is_enabled()) {
        links.push_back(json11::Json::object {
            { "href", services->get_wmts_service()->get_endpoint_uri() + "?SERVICE=WMTS&REQUEST=GetCapabilities&VERSION=1.0.0"},
            { "rel", "data"},
            { "type", "application/xml"},
            { "title", "OGC Web Map Tile Service capabilities"}
        });
    }

    if (services->get_tms_service()->is_enabled()) {
        links.push_back(json11::Json::object {
            { "href", services->get_tms_service()->get_endpoint_uri() + "/1.0.0"},
            { "rel", "data"},
            { "type", "application/xml"},
            { "title", "Tile Map Service capabilities"}
        });
    }

    if (services->get_tiles_service()->is_enabled()) {
        links.push_back(json11::Json::object {
            { "href", services->get_tiles_service()->get_endpoint_uri() + "/collections?f=json"},
            { "rel", "data"},
            { "type", "application/json"},
            { "title", "OGC API Tiles capabilities"}
        });
    }

    json11::Json::object res = json11::Json::object {
        { "title", title },
        { "description", abstract },
        { "links", links }
    };

    cache_mtx.lock();
    cache_getlandingpage = json11::Json{ res }.dump();
    cache_mtx.unlock();
    return new MessageDataStream ( cache_getlandingpage, "application/json", 200 );
}