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
 * \file services/tiles/gettile.cpp
 ** \~french
 * \brief Implémentation de la classe TilesService
 ** \~english
 * \brief Implements classe TilesService
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
#include <rok4/image/PaletteImage.h>

#include "services/tiles/Exception.h"
#include "services/tiles/Service.h"
#include "Rok4Server.h"

DataStream* TilesService::get_tile ( Request* req, Rok4Server* serv, bool is_map_request ) {

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

    // Le format : mvt, png, jpg ou tiff si fourni
    // Le format doit correspondre à celui natif des données

    std::string format;

    if (! req->has_query_param("f")) {
        format = Rok4Format::to_tiles_format(layer->get_pyramid()->get_format());
    } else {
        format = req->get_query_param("f");

        if (contain_chars(format, "\"")) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES format: " << format ;
            throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
        }

        if (format != Rok4Format::to_tiles_format(layer->get_pyramid()->get_format())) {
            throw TilesException::get_error_message("InvalidParameter", "Format " + format + " unknown", 400);
        }
    }


    // Récupération des paramètre selon le type de route (raster ou vecteur)
    
    std::string str_tms;
    std::string str_tm;
    std::string str_row;
    std::string str_column;

    Style* style = NULL;

    if (is_map_request) {
        // Map tiles request -> pour les pyramides raster
        if (! is_raster_data) {
            throw TilesException::get_error_message("InvalidParameter", "Vector dataset have to be requested with tiles request", 400);
        }

        std::string str_style = req->path_params.at(1);
        str_tms = req->path_params.at(2);
        str_tm = req->path_params.at(3);
        str_row = req->path_params.at(4);
        str_column = req->path_params.at(5);

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
            throw TilesException::get_error_message("InvalidParameter", "Raster dataset have to be requested with map tiles request", 400);
        }

        str_tms = req->path_params.at(1);
        str_tm = req->path_params.at(2);
        str_row = req->path_params.at(3);
        str_column = req->path_params.at(4);
    }

    // Le tile matrix set
    if (contain_chars(str_tms, "\"")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in TILES tile matrix set: " << str_tms;
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix set unknown", 400);
    }

    TileMatrixSetInfos* tmsi = layer->get_tilematrixset(str_tms);
    if (tmsi == NULL) {
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix set " + str_tms + " unknown", 400);
    }

    // Le niveau
    if (contain_chars(str_tm, "\"")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in TILES tile matrix: " << str_tm;
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix unknown", 400);
    }

    TileMatrix* tm = tmsi->tms->get_tm(str_tm);
    if (tm == NULL) {
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix " + str_tm + " unknown", 400);
    }

    // La colonne
    int column;
    if (sscanf(str_column.c_str(), "%d", &column) != 1) {
        throw TilesException::get_error_message("InvalidParameter", "Invalid column value", 400);
    }

    // La ligne
    int row;
    if (sscanf(str_row.c_str(), "%d", &row) != 1) {
        throw TilesException::get_error_message("InvalidParameter", "Invalid row value", 400);
    }

    TileMatrixLimits* tml = layer->get_tilematrix_limits(tmsi->tms, tm);
    if (tml == NULL) {
        // On est hors niveau -> erreur
        throw TilesException::get_error_message("ResourceNotFound", "Level out of limits", 404);
    }
    if (!tml->contain_tile(column, row)) {
        // On est hors tuiles -> erreur
        throw TilesException::get_error_message("ResourceNotFound", "Tile's indices out of limits", 404);
    }


    // Traitement de la requête

    if (tmsi->tms->get_id() == layer->get_pyramid()->get_tms()->get_id()) {
        // TMS d'interrogation natif
        Level* level = layer->get_pyramid()->get_level(tm->get_id());

        DataSource* d = level->get_tile(column, row);
        if (d == NULL) {
            throw TilesException::get_error_message("ResourceNotFound", "Not data found", 404);
        }

        if (layer->get_pyramid()->get_channels() == 1 && format == "png" && style->get_palette() && ! style->get_palette()->is_empty()) {
            return new DataStreamFromDataSource(new PaletteDataSource(d, style->get_palette()));
        } else {
            return new DataStreamFromDataSource(d);
        }
    } else if (reprojection) {
        // TMS d'interrogation à la demande

        BoundingBox<double> bbox = tm->tile_indices_to_bbox(column, row);
        int height = tm->get_tile_height();
        int width = tm->get_tile_width();
        CRS* crs = tmsi->tms->get_crs();
        bbox.crs = crs->get_request_code();

        bool crs_equals = serv->get_services_configuration()->are_crs_equals(layer->get_pyramid()->get_tms()->get_crs()->get_proj_code(), crs->get_proj_code());

        // On se donne maxium 3 tuiles sur 3 dans la pyramide source pour calculer cette tuile
        Image* image = layer->get_pyramid()->getbbox(3, 3, bbox, width, height, crs, crs_equals, layer->get_resampling(), 0);

        if (image == NULL) {
            BOOST_LOG_TRIVIAL(warning) << "Cannot process the tile in a non native TMS";
            throw TilesException::get_error_message("ResourceNotFound", "Not data found", 404);
        }

        image->set_bbox(bbox);
        image->set_crs(crs);

        if (format == "png") {
            return new PNGEncoder(image, style->get_palette());
        }
        else if (format == "tiff") {
            bool is_geotiff = true;

            // La donnée ne peut être retournée que dans le format de la pyramide source utilisée

            switch (layer->get_pyramid()->get_format()) {
                case Rok4Format::TIFF_RAW_UINT8:
                    return new TiffRawEncoder<uint8_t>(image, is_geotiff);
                case Rok4Format::TIFF_LZW_UINT8:
                    return new TiffLZWEncoder<uint8_t>(image, is_geotiff);
                case Rok4Format::TIFF_ZIP_UINT8:
                    return new TiffDeflateEncoder<uint8_t>(image, is_geotiff);
                case Rok4Format::TIFF_PKB_UINT8:
                    return new TiffPackBitsEncoder<uint8_t>(image, is_geotiff);
                case Rok4Format::TIFF_RAW_FLOAT32:
                    return new TiffRawEncoder<float>(image, is_geotiff);
                case Rok4Format::TIFF_LZW_FLOAT32:
                    return new TiffLZWEncoder<float>(image, is_geotiff);
                case Rok4Format::TIFF_ZIP_FLOAT32:
                    return new TiffDeflateEncoder<float>(image, is_geotiff);
                case Rok4Format::TIFF_PKB_FLOAT32:
                    return new TiffPackBitsEncoder<float>(image, is_geotiff);
                default:
                    delete image;
                    throw TilesException::get_error_message("ResourceNotFound", "Not data found", 404);
            }
        }
        else if (format == "jpg") {
            if (layer->get_pyramid()->get_format() == Rok4Format::TIFF_JPG_UINT8) {
                return new JPEGEncoder(image, 75);
            }
            else if (layer->get_pyramid()->get_format() == Rok4Format::TIFF_JPG90_UINT8) {
                return new JPEGEncoder(image, 90);
            }
        }
    } else {
        // TMS d'interrogation à la demande mais reprojection non activée
        throw TilesException::get_error_message("InvalidParameter", "Tile matrix set " + str_tms + " unknown", 400);
    }

    BOOST_LOG_TRIVIAL(error) << "On ne devrait pas passer par là";
    BOOST_LOG_TRIVIAL(error) << req->to_string();
    throw TilesException::get_error_message("ResourceNotFound", "Not data found", 404);
}