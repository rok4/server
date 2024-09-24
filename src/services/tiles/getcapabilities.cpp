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
 * \file services/tiles/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe TilesService
 ** \~english
 * \brief Implements classe TilesService
 */

#include <iostream>

#include "services/tiles/Exception.h"
#include "services/tiles/Service.h"
#include "Rok4Server.h"

DataStream* TilesService::get_capabilities ( Request* req, Rok4Server* serv ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    if ( ! cache_getcapabilities.empty()) {
        return new MessageDataStream ( cache_getcapabilities, "application/json", 200 );
    }

    std::vector<json11::Json> links;
    links.push_back(json11::Json::object {
        { "href", endpoint_uri + "/collections?f=application/json"},
        { "rel", "self"},
        { "type", "application/json"},
        { "title", "this document"}
    });

    if (metadata) {
        links.push_back(metadata->to_json_tiles("Service metadata", "describedby"));
    }

    std::vector<json11::Json> collections;

    std::map<std::string, Layer*>::iterator layers_iterator ( serv->get_server_configuration()->get_layers().begin() ), layers_end ( serv->get_server_configuration()->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        if (layers_iterator->second->is_tiles_enabled()) {
            collections.push_back(layers_iterator->second->to_json_tiles(this));
        }
    }

    json11::Json::object res = json11::Json::object {
        { "links", links },
        { "numberMatched", (int) collections.size() },
        { "numberReturned", (int) collections.size() }
    };

    res["collections"] = collections;

    cache_mtx.lock();
    cache_getcapabilities = json11::Json{ res }.dump();
    cache_mtx.unlock();
    return new MessageDataStream ( cache_getcapabilities, "application/json", 200 );
}

DataStream* TilesService::get_tiles ( Request* req, Rok4Server* serv ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    // La couche
    std::string str_layer = req->path_params.at(0);
    if ( contain_chars(str_layer, "\"")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES layer: " << str_layer ;
        throw TilesException::get_error_message("ResourceNotFound", "Layer unknown", 404);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tiles_enabled() ) {
        throw TilesException::get_error_message("ResourceNotFound", "Layer "+str_layer+" unknown", 404);
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_tiles(this) }.dump(), "application/json", 200 );
}

DataStream* TilesService::get_styles ( Request* req, Rok4Server* serv ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    // La couche
    std::string str_layer = req->path_params.at(0);
    if ( contain_chars(str_layer, "\"")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES layer: " << str_layer ;
        throw TilesException::get_error_message("ResourceNotFound", "Layer unknown", 404);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tiles_enabled() ) {
        throw TilesException::get_error_message("ResourceNotFound", "Layer "+str_layer+" unknown", 404);
    }

    if (! Rok4Format::is_raster(layer->get_pyramid()->get_format())) {
        throw TilesException::get_error_message("InvalidParameter", "Vector dataset have to be requested without style", 400);
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_styles(this) }.dump(), "application/json", 200 );
}


DataStream* TilesService::get_tilesets ( Request* req, Rok4Server* serv, bool is_map_request ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }
    
    // La couche
    std::string str_layer = req->path_params.at(0);
    if ( contain_chars(str_layer, "\"")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES layer: " << str_layer ;
        throw TilesException::get_error_message("ResourceNotFound", "Layer unknown", 404);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tiles_enabled() ) {
        throw TilesException::get_error_message("ResourceNotFound", "Layer "+str_layer+" unknown", 404);
    }

    bool is_raster_data = Rok4Format::is_raster(layer->get_pyramid()->get_format());

    Style* style = NULL;

    if (is_map_request) {
        // Map tiles request -> pour les pyramides raster
        if (! is_raster_data) {
            throw TilesException::get_error_message("InvalidParameter", "Vector dataset have to be requested without style", 400);
        }

        std::string str_style = req->path_params.at(1);

        // Traitement du style
        if ( contain_chars(str_style, "\"")) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES style: " << str_layer ;
            throw TilesException::get_error_message("InvalidParameter", "Style unknown", 400);
        }

        style = layer->get_style_by_identifier(str_style);

        if (style == NULL) {
            throw TilesException::get_error_message("InvalidParameter", "Style " + str_style + " unknown", 400);
        }

    } else {
        // Tiles request -> pour les pyramides vecteur
        if (is_raster_data) {
            throw TilesException::get_error_message("InvalidParameter", "Raster dataset have to be requested with style", 400);
        }
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_tilesets(this, style) }.dump(), "application/json", 200 );
}


DataStream* TilesService::get_tileset ( Request* req, Rok4Server* serv, bool is_map_request ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    // La couche
    std::string str_layer = req->path_params.at(0);
    if ( contain_chars(str_layer, "\"")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES layer: " << str_layer ;
        throw TilesException::get_error_message("ResourceNotFound", "Layer unknown", 404);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tiles_enabled() ) {
        throw TilesException::get_error_message("ResourceNotFound", "Layer "+str_layer+" unknown", 404);
    }

    bool is_raster_data = Rok4Format::is_raster(layer->get_pyramid()->get_format());

    std::string str_tms;
    Style* style = NULL;

    if (is_map_request) {
        // Map tiles request -> pour les pyramides raster
        if (! is_raster_data) {
            throw TilesException::get_error_message("InvalidParameter", "Vector dataset have to be requested without style", 400);
        }

        str_tms = req->path_params.at(2);

        std::string str_style = req->path_params.at(1);

        // Traitement du style
        if ( contain_chars(str_style, "\"")) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES style: " << str_layer ;
            throw TilesException::get_error_message("InvalidParameter", "Style unknown", 400);
        }

        style = layer->get_style_by_identifier(str_style);

        if (style == NULL) {
            throw TilesException::get_error_message("InvalidParameter", "Style " + str_style + " unknown", 400);
        }

    } else {
        // Tiles request -> pour les pyramides vecteur
        if (is_raster_data) {
            throw TilesException::get_error_message("InvalidParameter", "Raster dataset have to be requested with style", 400);
        }

        str_tms = req->path_params.at(1);
    }

    // Le tile matrix set
    if (contain_chars(str_tms, "\"")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in TILES tile matrix set: " << str_tms;
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix set unknown", 400);
    }

    TileMatrixSetInfos* tmsi = layer->get_tilematrixset(str_tms);
    if (tmsi == NULL || (! reprojection && tmsi->tms->get_id() != layer->get_pyramid()->get_tms()->get_id())) {
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix set " + str_tms + " unknown", 400);
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_tileset(this, style, tmsi) }.dump(), "application/json", 200 );
}