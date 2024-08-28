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
 * \file services/wmts/getfeatureinfo.cpp
 ** \~french
 * \brief Implémentation de la classe WmtsService
 ** \~english
 * \brief Implements classe WmtsService
 */

#include <iostream>

#include "services/wmts/Exception.h"
#include "services/wmts/Service.h"
#include "Rok4Server.h"

DataStream* WmtsService::get_feature_info ( Request* req, Rok4Server* serv ) {

    // La couche
    std::string str_layer = req->get_query_param("layer");
    if (str_layer == "") throw WmtsException::get_error_message("LAYER query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(str_layer, "<>")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS layer: " << str_layer;
        throw WmtsException::get_error_message("Layer unknown", "InvalidParameterValue", 400);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if (layer == NULL || ! layer->is_wmts_enabled()) {
        throw WmtsException::get_error_message("Layer " + str_layer + " unknown", "InvalidParameterValue", 400);
    }
    if (! layer->is_gfi_enabled()) {
        throw WmtsException::get_error_message("Layer " + str_layer + " not queryable", "InvalidParameterValue", 400);
    }

    // Le tile matrix set
    std::string str_tms = req->get_query_param("tilematrixset");
    if (str_tms == "") throw WmtsException::get_error_message("TILEMATRIXSET query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(str_tms, "<>")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS tile matrix set: " << str_tms;
        throw WmtsException::get_error_message("Tile matrix set unknown", "InvalidParameterValue", 400);
    }

    TileMatrixSet* tms = layer->get_tilematrixset(str_tms);
    if (tms == NULL) {
        throw WmtsException::get_error_message("Tile matrix set " + str_tms + " unknown", "InvalidParameterValue", 400);
    }

    // Le niveau
    std::string str_tm = req->get_query_param("tilematrix");
    if (str_tm == "") throw WmtsException::get_error_message("TILEMATRIX query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(str_tm, "<>")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS tile matrix: " << str_tm;
        throw WmtsException::get_error_message("Tile matrix unknown", "InvalidParameterValue", 400);
    }

    TileMatrix* tm = tms->get_tm(str_tm);
    if (tm == NULL) throw WmtsException::get_error_message("Tile matrix " + str_tm + " unknown", "InvalidParameterValue", 400);

    // La colonne
    int column;
    std::string str_column = req->get_query_param("tilecol");
    if (str_column == "") throw WmtsException::get_error_message("TILECOL query parameter missing", "MissingParameterValue", 400);
    if (sscanf(str_column.c_str(), "%d", &column) != 1)
        throw WmtsException::get_error_message("Invalid column value", "InvalidParameterValue", 400);

    // La ligne
    int row;
    std::string str_row = req->get_query_param("tilerow");
    if (str_row == "") throw WmtsException::get_error_message("TILEROW query parameter missing", "MissingParameterValue", 400);
    if (sscanf(str_row.c_str(), "%d", &row) != 1)
        throw WmtsException::get_error_message("Invalid row value", "InvalidParameterValue", 400);

    TileMatrixLimits* tml = layer->get_tilematrix_limits(tms, tm);
    if (tml == NULL) {
        // On est hors niveau -> erreur
        throw WmtsException::get_error_message("No data found", "Not Found", 404);
    }
    if (!tml->contain_tile(column, row)) {
        // On est hors tuiles -> erreur
        throw WmtsException::get_error_message("No data found", "Not Found", 404);
    }
    
    // infoformat

    std::string info_format = req->get_query_param("infoformat");
    if (info_format == "") throw WmtsException::get_error_message("INFOFORMAT query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(info_format, "<>")) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS info format: " << info_format;
        throw WmtsException::get_error_message("InfoFormat unknown", "InvalidParameterValue", 400);
    }

    if ( ! is_available_infoformat(info_format) )
        throw WmtsException::get_error_message("InfoFormat " + info_format + " unknown", "InvalidParameterValue", 400);

    // i
    int i;
    std::string str_i = req->get_query_param ( "i" );
    if ( str_i == "" ) throw WmtsException::get_error_message("I query parameter missing", "MissingParameterValue", 400);

    char c;
    if (sscanf(str_i.c_str(), "%d%c", &i, &c) != 1) {
        throw WmtsException::get_error_message("I query parameter have to be a positive integer", "MissingParameterValue", 400);
    }
    if ( i < 0 )
        throw WmtsException::get_error_message("I query parameter have to be a positive integer", "MissingParameterValue", 400);
    if ( i > tm->get_tile_width()-1 )
        throw WmtsException::get_error_message("I query parameter have to be smaller than the tile width", "MissingParameterValue", 400);


    // j
    std::string str_j = req->get_query_param ( "j" );
    if ( str_j == "" ) throw WmtsException::get_error_message("J query parameter missing", "MissingParameterValue", 400);

    int j;
    if (sscanf(str_j.c_str(), "%d%c", &j, &c) != 1) {
        throw WmtsException::get_error_message("J query parameter have to be a positive integer", "MissingParameterValue", 400);
    }
    if ( j < 0 )
        throw WmtsException::get_error_message("J query parameter have to be a positive integer", "MissingParameterValue", 400);
    if ( j > tm->get_tile_width()-1 )
        throw WmtsException::get_error_message("J query parameter have to be smaller than the tile height", "MissingParameterValue", 400);

    Level* level = layer->get_pyramid()->get_level(tm->get_id());

    std::string gfi_type = layer->get_gfi_type();
    if (gfi_type.compare("PYRAMID") == 0 && tms->get_id() == layer->get_pyramid()->get_tms()->get_id() ) {
        BOOST_LOG_TRIVIAL(debug) << "GFI sur pyramide dans le TMS natif";

        Image* image = level->get_tile(column, row, 0, 0, 0, 0, true);
        if (image == NULL) {
            throw WmtsException::get_error_message("No data found", "Not Found", 404);
        }

        std::vector<std::string> gfi_data;

        int bands = image->get_channels();
        switch (layer->get_pyramid()->get_format()) {
            case Rok4Format::TIFF_RAW_UINT8:
            case Rok4Format::TIFF_JPG_UINT8:
            case Rok4Format::TIFF_PNG_UINT8:
            case Rok4Format::TIFF_LZW_UINT8:
            case Rok4Format::TIFF_ZIP_UINT8:
            case Rok4Format::TIFF_PKB_UINT8: {
                uint8_t* data = new uint8_t[image->get_width() * bands * sizeof(uint8_t)];
                image->get_line(data, j);
                for (int b = 0; b < bands; b++) {
                    std::stringstream ss;
                    ss << (int)data[bands * i + b];
                    gfi_data.push_back(ss.str());
                }
                delete[] data;
                break;
            }
            case Rok4Format::TIFF_RAW_FLOAT32:
            case Rok4Format::TIFF_LZW_FLOAT32:
            case Rok4Format::TIFF_ZIP_FLOAT32:
            case Rok4Format::TIFF_PKB_FLOAT32: {
                float* data = new float[image->get_width() * bands * sizeof(float)];
                image->get_line(data, j);
                for (int b = 0; b < bands; b++) {
                    std::stringstream ss;
                    ss << (float)data[bands * i + b];
                    gfi_data.push_back(ss.str());
                }
                delete[] data;
                break;
            }
            default:
                throw WmtsException::get_error_message("No data found", "Not Found", 404);
        }

        delete image;

        return Utils::format_get_feature_info(gfi_data, info_format);

    } else if (gfi_type.compare("EXTERNALWMS") == 0) {
        BOOST_LOG_TRIVIAL(debug) << "GFI sur WMS externe, en TMS natif ou non";

        BoundingBox<double> bbox = tm->tile_indices_to_bbox(column, row);
        int height = tm->get_tile_height();
        int width = tm->get_tile_width();

        std::map<std::string, std::string> query_params;
        query_params.emplace("VERSION", "1.3.0");
        query_params.emplace("SERVICE", "WMS");
        query_params.emplace("REQUEST", "GetFeatureInfo");
        query_params.emplace("STYLES", "");
        query_params.emplace("LAYERS", layer->get_gfi_layers());
        query_params.emplace("QUERY_LAYERS", layer->get_gfi_query_layers());
        query_params.emplace("INFO_FORMAT", info_format);
        query_params.emplace("FEATURE_COUNT", "1");
        query_params.emplace("CRS", tms->get_crs()->get_request_code());
        query_params.emplace("WIDTH", std::to_string(width));
        query_params.emplace("HEIGHT", std::to_string(height));
        query_params.emplace("I", std::to_string(i));
        query_params.emplace("J", std::to_string(j));
        query_params.emplace("BBOX", bbox.to_string(tms->get_crs()->is_lat_lon()));

        std::map<std::string, std::string> extra_query_params = layer->get_gfi_extra_params();
        query_params.insert(extra_query_params.begin(), extra_query_params.end());

        Request* gfi_request = new Request("GET", layer->get_gfi_url(), query_params);

        RawDataStream* response = gfi_request->send();
        delete gfi_request;

        if (response == NULL) {
            throw WmtsException::get_error_message("No data found", "Not Found", 404);
        }

        return response;
    }

    throw WmtsException::get_error_message("Get Feature Info badly configured", "Bad configuration", 503);
}