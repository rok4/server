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
 * \file services/wms/getfeatureinfo.cpp
 ** \~french
 * \brief Implémentation de la classe WmsService
 ** \~english
 * \brief Implements classe WmsService
 */

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "services/wms/Exception.h"
#include "services/wms/Service.h"
#include "Rok4Server.h"

DataStream* WmsService::get_feature_info ( Request* req, Rok4Server* serv ) {

    // Les couches
    std::vector<Layer*> layers;

    std::string str_layers = req->get_query_param("layers");
    if (str_layers == "") throw WmsException::get_error_message("LAYERS query parameter missing", "MissingParameterValue", 400);

    std::vector<std::string> vector_layers;
    boost::split(vector_layers, str_layers, boost::is_any_of(","));

    if ( vector_layers.size() > max_layers_count ) {
        throw WmsException::get_error_message("Number of layers exceed the limit (" + std::to_string(max_layers_count) + ")", "InvalidParameterValue", 400);
    }

    for (unsigned int i = 0 ; i < vector_layers.size(); i++ ) {

        if (contain_chars(vector_layers.at(i), "<>")) {
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS layer: " << vector_layers.at(i);
            throw WmsException::get_error_message("Layer unknown", "InvalidParameterValue", 400);
        }

        Layer* layer = serv->get_server_configuration()->get_layer(vector_layers.at(i));
        if (layer == NULL || ! layer->is_wms_enabled()) {
            throw WmsException::get_error_message("Layer " + vector_layers.at(i) + " unknown", "InvalidParameterValue", 400);
        }
       
        layers.push_back ( layer );
    }

    // Les couches interrogées
    std::vector<Layer*> query_layers;

    std::string str_query_layers = req->get_query_param("query_layers");
    if (str_query_layers == "") throw WmsException::get_error_message("QUERY_LAYERS query parameter missing", "MissingParameterValue", 400);

    std::vector<std::string> vector_query_layers;
    boost::split(vector_query_layers, str_query_layers, boost::is_any_of(","));

    if ( vector_query_layers.size() > 1 ) {
        throw WmsException::get_error_message("Only one query layer is allowed", "InvalidParameterValue", 400);
    }

    for (unsigned int i = 0 ; i < vector_query_layers.size(); i++ ) {

        if (contain_chars(vector_query_layers.at(i), "<>")) {
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS layer: " << vector_query_layers.at(i);
            throw WmsException::get_error_message("Layer unknown", "LayerNotDefined", 400);
        }

        Layer* layer = serv->get_server_configuration()->get_layer(vector_query_layers.at(i));
        if (layer == NULL || ! layer->is_wms_enabled()) {
            throw WmsException::get_error_message("Layer " + vector_query_layers.at(i) + " unknown", "LayerNotDefined", 400);
        }

        if (! layer->is_gfi_enabled()) {
            throw WmsException::get_error_message("Layer " + vector_query_layers.at(i) + " not queryable", "LayerNotQueryable", 400);
        }

        if (std::find(vector_layers.begin(), vector_layers.end(), vector_query_layers.at(i)) == vector_layers.end()) {
            // La couche interrogée n'est pas dans les couches de base de la requête
            throw WmsException::get_error_message("Layer " + vector_query_layers.at(i) + " unknown", "LayerNotDefined", 400);
        }
       
        query_layers.push_back ( layer );
    }

    // La largeur
    int width;
    std::string str_width = req->get_query_param("width");
    if (str_width == "") throw WmsException::get_error_message("WIDTH query parameter missing", "MissingParameterValue", 400);
    if (sscanf(str_width.c_str(), "%d", &width) != 1)
        throw WmsException::get_error_message("Invalid WIDTH value", "InvalidParameterValue", 400);

    if ( width < 0 )
        throw WmsException::get_error_message("WIDTH query parameter have to be a positive integer", "InvalidParameterValue", 400);
    if ( width > max_width )
        throw WmsException::get_error_message("WIDTH query parameter exceed the limit (" + std::to_string(max_width) + ")", "InvalidParameterValue", 400);

    // La hauteur
    int height;
    std::string str_height = req->get_query_param("height");
    if (str_height == "") throw WmsException::get_error_message("HEIGHT query parameter missing", "MissingParameterValue", 400);
    if (sscanf(str_height.c_str(), "%d", &height) != 1)
        throw WmsException::get_error_message("Invalid HEIGHT value", "InvalidParameterValue", 400);

    if ( height < 0 )
        throw WmsException::get_error_message("HEIGHT query parameter have to be a positive integer", "InvalidParameterValue", 400);
    if ( height > max_height )
        throw WmsException::get_error_message("HEIGHT query parameter exceed the limit (" + std::to_string(max_width) + ")", "InvalidParameterValue", 400);

    // le CRS
    CRS* crs;
    std::string str_crs = req->get_query_param("crs");
    if (str_crs == "") throw WmsException::get_error_message("CRS query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(str_crs, "<>")) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS CRS: " << str_crs;
        throw WmsException::get_error_message("CRS unknown", "InvalidParameterValue", 400);
    }

    crs = CrsBook::get_crs( str_crs );
    if (! crs->is_define()) {
        throw WmsException::get_error_message("CRS " + str_crs + " unknown", "InvalidParameterValue", 400);
    }

    if (! is_available_crs(str_crs) ) {
        for ( unsigned int i = 0; i < layers.size() ; i++ ) {
            bool crs_equals = serv->get_services_configuration()->are_crs_equals(crs->get_request_code(), layers.at(i)->get_pyramid()->get_tms()->get_crs()->get_request_code());
            if (! crs_equals && ! layers.at ( i )->is_available_crs(str_crs) )
                throw WmsException::get_error_message("CRS is not available for the layer " + layers.at ( i )->get_id(), "InvalidParameterValue", 400);
        }
    }

    // Le format
    std::string format = req->get_query_param("format");
    if (format == "") throw WmsException::get_error_message("FORMAT query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(format, "<>")) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS format: " << format;
        throw WmsException::get_error_message("Format unknown", "InvalidParameterValue", 400);
    }

    if (! is_available_format(format)) throw WmsException::get_error_message("Format " + format + " unknown", "InvalidParameterValue", 400);

    // La bbox
    std::string str_bbox = req->get_query_param("bbox");
    if (str_bbox == "") throw WmsException::get_error_message("BBOX query parameter missing", "MissingParameterValue", 400);

    std::vector<std::string> vector_bbox;
    boost::split(vector_bbox, str_bbox, boost::is_any_of(","));
    if (vector_bbox.size() != 4) throw WmsException::get_error_message("BBOX query parameter invalid (require 4 comma separated numbers)", "InvalidParameterValue", 400);
    
    double bb[4];
    for ( int i = 0; i < 4; i++ ) {
        if ( sscanf ( vector_bbox[i].c_str(),"%lf",&bb[i] ) !=1 )
            throw WmsException::get_error_message("BBOX query parameter invalid (require 4 comma separated numbers)", "InvalidParameterValue", 400);
        //Test NaN values
        if (bb[i] != bb[i])
            throw WmsException::get_error_message("BBOX query parameter invalid (require 4 comma separated numbers)", "InvalidParameterValue", 400);
    }
    if ( bb[0] >= bb[2] || bb[1] >= bb[3] )
        throw WmsException::get_error_message("BBOX query parameter invalid (max have to be bigger than min)", "InvalidParameterValue", 400);

    BoundingBox<double> bbox;
    if (crs->is_lat_lon()) {
        bbox.xmin=bb[1];
        bbox.ymin=bb[0];
        bbox.xmax=bb[3];
        bbox.ymax=bb[2];
    } else {
        bbox.xmin=bb[0];
        bbox.ymin=bb[1];
        bbox.xmax=bb[2];
        bbox.ymax=bb[3];
    }

    bbox.crs = crs->get_request_code();

    // Exception
    std::string exception = req->get_query_param ( "exception" );
    if ( exception != "" && exception != "XML" ) {
        throw WmsException::get_error_message("EXCEPTION query parameter invalid (only XML accepted)", "InvalidParameterValue", 400);
    }

    // Les styles
    std::vector<Style*> styles;
    if (! req->has_query_param("styles")) throw WmsException::get_error_message("STYLES query parameter missing", "MissingParameterValue", 400);

    std::string str_styles = req->get_query_param ( "styles" );
    if (str_styles == "") {
        // Cas où l'on veut les styles par défaut pour toutes les couches
        // On met la version avec virgules pour gérer le cas de manière générique après
        str_styles = std::string(layers.size() - 1, ',');
    }

    std::vector<std::string> vector_styles;
    boost::split(vector_styles, str_styles, boost::is_any_of(","));
    if ( vector_styles.size() != layers.size() ) {
        throw WmsException::get_error_message("STYLES query parameter invalid (comma separated styles identifier, one by requested layer)", "InvalidParameterValue", 400);
    }
    for ( int i = 0 ; i  < vector_styles.size(); i++ ) {
        if ( vector_styles.at(i) == "" ) {
            styles.push_back(layers.at(i)->get_default_style());
            continue;
        }

        if (contain_chars(vector_styles.at(i), "<>")) {
            // On a détecté un caractère interdit, on ne met pas le style fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS style: " << vector_styles.at(i);
            throw WmsException::get_error_message("Style unknown", "InvalidParameterValue", 400);
        }

        Style* style = layers.at(i)->get_style_by_identifier(vector_styles.at(i));
        if ( style == NULL )
            throw WmsException::get_error_message("Style " + vector_styles.at(i) + " is not available for the layer " + layers.at(i)->get_id(), "InvalidParameterValue", 400);

        styles.push_back(style);
    }

    // Les options de format
    std::map<std::string, std::string> format_options ;
    if (req->get_query_param("format_options") != "") {
        format_options = Utils::string_to_map(req->get_query_param("format_options"), ";", ":");
    }

    // dpi
    int dpi = 0;
    std::string str_dpi = req->get_query_param("str_dpi");
    if (str_dpi != "") {
        if (sscanf(str_dpi.c_str(), "%d", &dpi) != 1)
            throw WmsException::get_error_message("Invalid DPI value", "InvalidParameterValue", 400);

        if ( width < 0 )
            throw WmsException::get_error_message("DPI query parameter have to be a positive integer", "InvalidParameterValue", 400);
    }

    // info_format

    std::string info_format = req->get_query_param("info_format");
    if (info_format == "") throw WmsException::get_error_message("INFO_FORMAT query parameter missing", "MissingParameterValue", 400);

    if (contain_chars(info_format, "<>")) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS info format: " << info_format;
        throw WmsException::get_error_message("InfoFormat unknown", "InvalidParameterValue", 400);
    }

    if ( ! is_available_infoformat(info_format) )
        throw WmsException::get_error_message("INFO_FORMAT " + info_format + " unknown", "InvalidParameterValue", 400);

    // i
    int i;
    std::string str_i = req->get_query_param ( "i" );
    if ( str_i == "" ) throw WmsException::get_error_message("I query parameter missing", "MissingParameterValue", 400);

    char c;
    if (sscanf(str_i.c_str(), "%d%c", &i, &c) != 1) {
        throw WmsException::get_error_message("I query parameter have to be a positive integer", "InvalidPoint", 400);
    }
    if ( i < 0 )
        throw WmsException::get_error_message("I query parameter have to be a positive integer", "InvalidPoint", 400);
    if ( i > width )
        throw WmsException::get_error_message("I query parameter have to be smaller than width", "InvalidPoint", 400);


    // j
    std::string str_j = req->get_query_param ( "j" );
    if ( str_j == "" ) throw WmsException::get_error_message("J query parameter missing", "MissingParameterValue", 400);

    int j;
    if (sscanf(str_j.c_str(), "%d%c", &j, &c) != 1) {
        throw WmsException::get_error_message("J query parameter have to be a positive integer", "InvalidPoint", 400);
    }
    if ( j < 0 )
        throw WmsException::get_error_message("J query parameter have to be a positive integer", "InvalidPoint", 400);
    if ( j > height )
        throw WmsException::get_error_message("J query parameter have to be smaller than height", "InvalidPoint", 400);

    // feature_count
    int feature_count = 1;
    std::string str_feature_count = req->get_query_param ( "feature_count" );
    if ( str_feature_count != "" ) {
        char c;
        if (sscanf(str_feature_count.c_str(), "%d%c", &feature_count, &c) != 1) {
            feature_count = 1;
        }
        if ( feature_count < 0 ) {
            feature_count = 1;
        }
    }

    // Traitement de la requête

    Layer* layer = query_layers.at(0);

    std::string gfi_type = layer->get_gfi_type();
    if (gfi_type.compare("PYRAMID") == 0) {

        // On cherche la valeur du pixel source sous le clique
        bool crs_equals = serv->get_services_configuration()->are_crs_equals(crs->get_request_code(), layer->get_pyramid()->get_tms()->get_crs()->get_request_code());

        if (! crs_equals && ! reprojection) {
            throw WmsException::get_error_message("Reprojection is not available", "InvalidParameterValue", 400);
        }

        Image* image = layer->get_pyramid()->getbbox(max_tile_x, max_tile_y, bbox, width, height, crs, crs_equals, layer->get_resampling(), dpi);

        if (image == NULL) {
            throw WmsException::get_error_message("BBOX too big", "InvalidParameterValue", 400);
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
                    ss.setf(std::ios::fixed, std::ios::floatfield);
                    ss.precision(2);
                    ss << data[bands * i + b];
                    gfi_data.push_back(ss.str());
                }
                delete[] data;
                break;
            }
            default:
                throw WmsException::get_error_message("No readable data found", "Not Found", 404);
        }

        delete image;

        return Utils::format_get_feature_info(gfi_data, info_format);
    } else if (gfi_type.compare("EXTERNALWMS") == 0) {
        BOOST_LOG_TRIVIAL(debug) << "GFI sur WMS externe, en projection native ou non";

        std::map<std::string, std::string> query_params;
        query_params.emplace("VERSION", "1.3.0");
        query_params.emplace("SERVICE", "WMS");
        query_params.emplace("REQUEST", "GetFeatureInfo");
        query_params.emplace("STYLES", "");
        query_params.emplace("LAYERS", layer->get_gfi_layers());
        query_params.emplace("QUERY_LAYERS", layer->get_gfi_query_layers());
        query_params.emplace("INFO_FORMAT", info_format);
        query_params.emplace("FORMAT", "image/tiff");
        query_params.emplace("FEATURE_COUNT", std::to_string(feature_count));
        query_params.emplace("CRS", crs->get_request_code());
        query_params.emplace("WIDTH", std::to_string(width));
        query_params.emplace("HEIGHT", std::to_string(height));
        query_params.emplace("I", std::to_string(i));
        query_params.emplace("J", std::to_string(j));
        query_params.emplace("BBOX", str_bbox);

        std::map<std::string, std::string> extra_query_params = layer->get_gfi_extra_params();
        query_params.insert(extra_query_params.begin(), extra_query_params.end());

        Request* gfi_request = new Request("GET", layer->get_gfi_url(), query_params);

        RawDataStream* response = gfi_request->send();
        delete gfi_request;

        if (response == NULL) {
            throw WmsException::get_error_message("No readable data found", "Not Found", 404);
        }

        return response;
    }

    throw WmsException::get_error_message("Get Feature Info badly configured", "Bad configuration", 503);

}