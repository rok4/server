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
 * \file services/ogcapi/Service.cpp
 ** \~french
 * \brief Implémentation de la classe OgcApiService
 ** \~english
 * \brief Implements classe OgcApiService
 */

#include <iostream>

#include "services/ogcapi/Exception.h"
#include "services/ogcapi/Service.h"
#include "core/Rok4Server.h"

OgcApiService::OgcApiService (json11::Json& doc) : Service(doc, "OGC API service", "OGC API service", "http://localhost/ogcapi", "/ogcapi") {

    if (! is_ok()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        error_message = "OGCAPI service: " + error_message;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    if (doc["tiles"].is_bool()) {
        tiles = doc["tiles"].bool_value();
    } else if (! doc["tiles"].is_null()) {
        error_message = "OGC API service: tiles have to be a boolean";
        return;
    } else {
        tiles = true;
    }

    if (doc["maps"].is_bool()) {
        maps = doc["maps"].bool_value();
    } else if (! doc["maps"].is_null()) {
        error_message = "OGC API service: maps have to be a boolean";
        return;
    } else {
        maps = false;
    }
}

DataStream* OgcApiService::process_request(Request* req, ServicesConfiguration* services) {
    BOOST_LOG_TRIVIAL(debug) << "OGC API service";

    if ( match_route( "", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET LANDING PAGE request";
        return get_landing_page(req, services);
    }
    else if ( match_route( "/conformance", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET CONFORMANCE request";
        return get_conformance(req, services);
    }
    // API
    else if ( match_route( "/api/all-collections", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET API COLLECTION request";
        return get_api_collections(req, services);
    }
    else if ( match_route( "/api/vectorTiles-collections", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET API VECTOR COLLECTIONS request";
        return get_api_vector_collections(req, services);
    }
    else if ( match_route( "/api/tileMatrixSets", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET API TILE MATRIX SETS request";
        return get_api_tilematrixsets(req, services);
    }
    else if ( match_route( "/api/styles", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET API STYLES request";
        return get_api_styles(req, services);
    }
    // TMS
    else if ( match_route( "/tileMatrixSets", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILE MATRIX SETS request";
        return get_tilematrixsets(req, services);
    }
    else if ( match_route( "/tileMatrixSets/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILE MATRIX SET request";
        return get_tilematrixset(req, services);
    }
    // Collections
    else if ( match_route( "/collections", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET COLLECTIONS request";
        return get_collections(req, services);
    }
    else if ( match_route( "/collections/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET COLLECTION request";
        return get_collection(req, services);
    }

    // TILES
    // Données vecteur
    else if ( tiles && match_route( "/collections/([^/]+)/tiles", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILESETS vector request";
        return get_tilesets(req, services, false);
    }
    else if ( tiles && match_route( "/collections/([^/]+)/tiles/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILESET vector request";
        return get_tileset(req, services, false);
    }
    else if ( tiles && match_route( "/collections/([^/]+)/tiles/([^/]+)/([^/]+)/([^/]+)/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILE vector request";
        return get_tile(req, services, false);
    }
    // Données raster
    else if ( tiles && match_route( "/collections/([^/]+)/map/tiles", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILE SETS map request";
        return get_tilesets(req, services, true);
    }
    else if ( tiles && match_route( "/collections/([^/]+)/map/tiles/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILE SET map request";
        return get_tileset(req, services, true);
    }
    else if ( tiles && match_route( "/collections/([^/]+)/styles/([^/]+)/map/tiles/([^/]+)/([^/]+)/([^/]+)/([^/]+)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GET TILE map request";
        return get_tile(req, services, true);
    }

    // MAPS
    // Données raster
    // à venir
    
    else {
        throw OgcApiException::get_error_message("ResourceNotFound", "Unknown OGC API request path", 404);
    }
};
