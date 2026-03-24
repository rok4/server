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
 * \file services/ogcapi/gettile.cpp
 ** \~french
 * \brief Implémentation de la classe OgcApiService
 ** \~english
 * \brief Implements classe OgcApiService
 */

#include <iostream>

#include <rok4/datasource/PaletteDataSource.h>
#include <rok4/datastream/AscEncoder.h>
#include <rok4/datastream/BilEncoder.h>
#include <rok4/datastream/JPEGEncoder.h>
#include <rok4/datastream/PNGEncoder.h>
#include <rok4/datastream/TiffEncoder.h>
#include <rok4/datastream/TiffPackBitsEncoder.h>
#include <rok4/datastream/TiffLZWEncoder.h>
#include <rok4/datastream/TiffDeflateEncoder.h>
#include <rok4/datastream/TiffRawEncoder.h>

#include "services/ogcapi/Exception.h"
#include "services/ogcapi/Service.h"
#include "core/Rok4Server.h"
#include "core/Tile.h"


DataStream* OgcApiService::get_tilesets ( Request* req, ServicesConfiguration* services, bool is_map_request ) {

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

    bool is_raster_data = layer->is_raster();

    if (is_map_request) {
        // Map tiles request -> pour les pyramides raster
        if (! is_raster_data) {
            throw OgcApiException::get_error_message("InvalidParameter", "Vector dataset have to be requested without map", 400);
        }

    } else {
        // Tiles request -> pour les pyramides vecteur
        if (is_raster_data) {
            throw OgcApiException::get_error_message("InvalidParameter", "Raster dataset have to be requested with map", 400);
        }
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_tilesets(this) }.dump(), "application/json", 200 );
}


DataStream* OgcApiService::get_tileset ( Request* req, ServicesConfiguration* services, bool is_map_request ) {

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

    bool is_raster_data = layer->is_raster();

    std::string str_tms;

    if (is_map_request) {
        // Map tiles request -> pour les pyramides raster
        if (! is_raster_data) {
            throw OgcApiException::get_error_message("InvalidParameter", "Vector dataset have to be requested without map", 400);
        }

        str_tms = req->path_params.at(1);

    } else {
        // Tiles request -> pour les pyramides vecteur
        if (is_raster_data) {
            throw OgcApiException::get_error_message("InvalidParameter", "Raster dataset have to be requested with map", 400);
        }

        str_tms = req->path_params.at(1);
    }

    // Le tile matrix set
    if (contain_chars(str_tms, "\"")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in TILES tile matrix set: " << str_tms;
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix set unknown", 400);
    }

    TileMatrixSetInfos* tmsi = layer->get_tilematrixset(str_tms);
    if (tmsi == NULL || (! services->tile_reprojection && tmsi->tms->get_id() != layer->get_pyramid()->get_tms()->get_id())) {
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix set " + str_tms + " unknown", 400);
    }

    return new MessageDataStream ( json11::Json{ layer->to_json_tileset(this, tmsi) }.dump(), "application/json", 200 );
}

DataStream* OgcApiService::get_tile ( Request* req, ServicesConfiguration* services, bool is_map_request ) {

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

    bool is_raster_data = layer->is_raster();

    // Le format : mvt, png, jpg ou tiff si fourni
    // Le format doit correspondre à celui natif des données

    std::string format;

    if (! req->has_query_param("f")) {
        format = Rok4Format::to_ogcapi_format(layer->get_pyramid()->get_format());
    } else {
        format = req->get_query_param("f");

        if (contain_chars(format, "\"")) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in format: " << format ;
            throw OgcApiException::get_error_message("InvalidParameter", "Format unknown", 400);
        }

        if (format != Rok4Format::to_ogcapi_format(layer->get_pyramid()->get_format())) {
            throw OgcApiException::get_error_message("InvalidParameter", "Format " + format + " unknown", 400);
        }
    }

    format = ogcapi_format_to_mime_type.at(format);

    // Récupération des paramètre selon le type de route (raster ou vecteur)
    
    std::string str_tms;
    std::string str_tm;
    std::string str_row;
    std::string str_column;

    Style* style = NULL;

    if (is_map_request) {
        // Map tiles request -> pour les pyramides raster
        if (! is_raster_data) {
            throw OgcApiException::get_error_message("InvalidParameter", "Vector dataset have to be requested with tiles request", 400);
        }

        std::string str_style = req->path_params.at(1);
        str_tms = req->path_params.at(2);
        str_tm = req->path_params.at(3);
        str_row = req->path_params.at(4);
        str_column = req->path_params.at(5);

        // Traitement du style
        if ( contain_chars(str_style, "\"")) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in style: " << str_layer ;
            throw OgcApiException::get_error_message("InvalidParameter", "Style unknown", 400);
        }

        style = layer->get_style_by_identifier(str_style);

        if (style == NULL) {
            throw OgcApiException::get_error_message("InvalidParameter", "Style " + str_style + " unknown", 400);
        }

    } else {
        // Tiles request -> pour les pyramides vecteur
        if (is_raster_data) {
            throw OgcApiException::get_error_message("InvalidParameter", "Raster dataset have to be requested with map tiles request", 400);
        }

        str_tms = req->path_params.at(1);
        str_tm = req->path_params.at(2);
        str_row = req->path_params.at(3);
        str_column = req->path_params.at(4);
    }

    // Le tile matrix set
    if (contain_chars(str_tms, "\"")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in tile matrix set: " << str_tms;
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix set unknown", 400);
    }

    TileMatrixSetInfos* tmsi = layer->get_tilematrixset(str_tms);
    if (tmsi == NULL) {
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix set " + str_tms + " unknown", 400);
    }
    if (tmsi->tms->get_id() != layer->get_pyramid()->get_tms()->get_id() && ! services->tile_reprojection) {
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix set " + str_tms + " unknown", 400);
    }

    // Le niveau
    if (contain_chars(str_tm, "\"")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in tile matrix: " << str_tm;
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix unknown", 400);
    }

    TileMatrix* tm = tmsi->tms->get_tm(str_tm);
    if (tm == NULL) {
        throw OgcApiException::get_error_message("InvalidParameter", "Tile matrix " + str_tm + " unknown", 400);
    }

    // La colonne
    int column;
    if (sscanf(str_column.c_str(), "%d", &column) != 1) {
        throw OgcApiException::get_error_message("InvalidParameter", "Invalid column value", 400);
    }

    // La ligne
    int row;
    if (sscanf(str_row.c_str(), "%d", &row) != 1) {
        throw OgcApiException::get_error_message("InvalidParameter", "Invalid row value", 400);
    }

    TileMatrixLimits* tml = layer->get_tilematrix_limits(tmsi->tms, tm);
    if (tml == NULL) {
        // On est hors niveau -> erreur
        throw OgcApiException::get_error_message("ResourceNotFound", "Level out of limits", 404);
    }
    if (!tml->contain_tile(column, row)) {
        // On est hors tuiles -> erreur
        throw OgcApiException::get_error_message("ResourceNotFound", "Tile's indices out of limits", 404);
    }

    // Traitement de la requête
    DataStream* d = Tile::get_tile(services, layer, tmsi->tms, tm, column, row, format, style);
    if (d == NULL) {
        throw OgcApiException::get_error_message("ResourceNotFound", "Not data found", 404);
    }
    return d;
}