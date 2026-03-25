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
#include <boost/algorithm/string.hpp>

#include "services/ogcapi/Exception.h"
#include "services/ogcapi/Service.h"
#include "core/Rok4Server.h"
#include "core/Map.h"


DataStream* OgcApiService::get_map ( Request* req, ServicesConfiguration* services ) {

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

    if (! layer->is_raster()) {
        throw OgcApiException::get_error_message("InvalidParameter", "Vector data " + str_layer + " cannot be requested with WMS GetMap", 400);
    }

    std::vector<Layer*> layers = {layer};

    // le CRS de l'image finale
    CRS* crs;
    std::string str_crs = req->get_query_param("crs");
    if (str_crs != "") {
        if (contain_chars(str_crs, "<>")) {
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS CRS: " << str_crs;
            throw OgcApiException::get_error_message("InvalidParameter", "CRS unknown", 400);
        }
    } else {
        str_crs = layer->get_native_bbox().crs;
    }

    crs = CrsBook::get_crs( str_crs );
    if (! crs->is_define()) {
        throw OgcApiException::get_error_message("InvalidParameter", "CRS " + str_crs + " unknown", 400);
    }

    if (! services->is_map_available_crs(str_crs) ) {
        for ( unsigned int i = 0; i < layers.size() ; i++ ) {
            bool crs_equals = services->are_crs_equals(crs->get_request_code(), layers.at(i)->get_pyramid()->get_tms()->get_crs()->get_request_code());
            if (! crs_equals && (! services->map_reprojection || ! layers.at ( i )->is_available_crs(str_crs)) ) {
                throw OgcApiException::get_error_message("InvalidParameter", "CRS " + str_crs + " is not available for the layer " + layers.at ( i )->get_id(), 400);
            }
        }
    }

    // le CRS de la bbox
    CRS* bbox_crs;
    std::string str_bbox_crs = req->get_query_param("bbox-crs");
    if (str_bbox_crs != "") {
        if (contain_chars(str_bbox_crs, "<>")) {
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMS CRS: " << str_bbox_crs;
            throw OgcApiException::get_error_message("InvalidParameter", "CRS unknown", 400);
        }
    } else {
        str_bbox_crs = "CRS:84";
    }

    bbox_crs = CrsBook::get_crs( str_bbox_crs );
    if (! bbox_crs->is_define()) {
        throw OgcApiException::get_error_message("InvalidParameter", "BBOX CRS " + str_bbox_crs + " unknown", 400);
    }

    if (! services->is_map_available_crs(str_bbox_crs) ) {
        for ( unsigned int i = 0; i < layers.size() ; i++ ) {
            bool crs_equals = services->are_crs_equals(bbox_crs->get_request_code(), layers.at(i)->get_pyramid()->get_tms()->get_crs()->get_request_code());
            if (! crs_equals && (! services->map_reprojection || ! layers.at ( i )->is_available_crs(str_bbox_crs)) ) {
                throw OgcApiException::get_error_message("InvalidParameter", "BBOX CRS " + str_bbox_crs + " is not available for the layer " + layers.at ( i )->get_id(), 400);
            }
        }
    }

    // La bbox
    std::string str_bbox = req->get_query_param("bbox");
    BoundingBox<double> bbox;
    if (str_bbox != "") {

        std::vector<std::string> vector_bbox;
        boost::split(vector_bbox, str_bbox, boost::is_any_of(","));
        if (vector_bbox.size() != 4 && vector_bbox.size() != 6) throw OgcApiException::get_error_message("InvalidParameter", "bbox query parameter invalid (require 4 or 6 comma separated numbers)", 400);
        
        if (vector_bbox.size() == 6) {
            // On retire le 6ème et le 3ème élément
            vector_bbox.erase(vector_bbox.begin() + 5);
            vector_bbox.erase(vector_bbox.begin() + 2);
        }

        double bb[4];
        for ( int i = 0; i < 4; i++ ) {
            if ( sscanf ( vector_bbox[i].c_str(),"%lf",&bb[i] ) !=1 )
                throw OgcApiException::get_error_message("InvalidParameter", "BBOX query parameter invalid (require 4 or 6 comma separated numbers)", 400);
            //Test NaN values
            if (bb[i] != bb[i])
                throw OgcApiException::get_error_message("InvalidParameter", "BBOX query parameter invalid (require 4 or 6 comma separated numbers)", 400);
        }
        if ( bb[0] >= bb[2] || bb[1] >= bb[3] )
            throw OgcApiException::get_error_message("InvalidParameter", "BBOX query parameter invalid (max have to be bigger than min)", 400);

        if (bbox_crs->is_lat_lon()) {
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

        bbox.crs = bbox_crs->get_request_code();
    } else {
        // On met la bbox globale de la donnée, dans le CRS natif des données
        bbox = layer->get_native_bbox();
        bbox_crs = layer->get_pyramid()->get_tms()->get_crs();
    }

    // Reprojection de la bbox dans le CRS de l'image demandée

    if (! services->are_crs_equals(str_bbox_crs, str_crs)) {
        bbox.reproject(bbox_crs, crs, 4);
    }

    // calcul du ratio largeur sur hauteur de la bbox
    double bbox_ratio = (bbox.xmax - bbox.xmin) / (bbox.ymax - bbox.ymin);

    // Dimensions

    int width = -1;
    std::string str_width = req->get_query_param("width");

    if (str_width != "") {
        if (sscanf(str_width.c_str(), "%d", &width) != 1)
            throw OgcApiException::get_error_message("InvalidParameter", "Invalid width value", 400);

        if ( width <= 0 )
            throw OgcApiException::get_error_message("InvalidParameter", "width query parameter have to be a strictly positive integer", 400);
        if ( width > services->map_max_width )
            throw OgcApiException::get_error_message("InvalidParameter", "width query parameter exceed the limit (" + std::to_string(services->map_max_width) + ")", 400);
    }

    int height = -1;
    std::string str_height = req->get_query_param("height");
    if (str_height != "") {
        if (sscanf(str_height.c_str(), "%d", &height) != 1)
            throw OgcApiException::get_error_message("InvalidParameter", "Invalid height value", 400);

        if ( height <= 0 )
            throw OgcApiException::get_error_message("InvalidParameter", "height query parameter have to be a strictly positive integer", 400);
        if ( height > services->map_max_height )
            throw OgcApiException::get_error_message("InvalidParameter", "height query parameter exceed the limit (" + std::to_string(services->map_max_height) + ")", 400);
    }

    if (width == -1 && height == -1) {
        // On a fourni aucune dimension, on met la plus grande à default_size et l'autre en respectant le ratio
        if (bbox.xmax - bbox.xmin > bbox.ymax - bbox.ymin) {
            width = default_size;
        } else {
            height = default_size;
        }
    }

    if (width == -1) {
        // Une dimension est encore manquante, on respecte le ratio pour la définir
        width = (int) round(bbox_ratio * height);
    } 
    else if (height == -1) {
        // Une dimension est encore manquante, on respecte le ratio pour la définir
        height = (int) round((double) width / bbox_ratio);
    }

    // Le format
    std::string str_format = req->get_query_param("f");
    std::string format;
    if (str_format != "") {

        if (contain_chars(str_format, "\"")) {
            // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in OGC API Maps format: " << str_format;
            throw OgcApiException::get_error_message("InvalidParameter", "Format unknown", 400);
        }

        format = ogcapi_format_to_mime_type.at(str_format);

        if (! services->is_map_available_format(format)) {
            throw OgcApiException::get_error_message("InvalidParameter", "Format " + str_format + " unknown", 400);
        }
    } else {
        format = Rok4Format::to_mime_type(layer->get_pyramid()->get_format());
    }

    // Le style
    std::vector<Style*> styles;

    if (req->path_params.size() == 1) {
        // pas de style fourni, on prend celui par défaut
        styles.push_back(layer->get_default_style());
    } else {
        std::string str_style = req->path_params.at(1);
        if (contain_chars(str_style, "\"")) {
            // On a détecté un caractère interdit, on ne met pas le style fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in OGC API Maps style: " << str_style;
            throw OgcApiException::get_error_message("InvalidParameter", "Style unknown", 400);
        }

        Style* style = layer->get_style_by_identifier(str_style);
        if ( style == NULL )
            throw OgcApiException::get_error_message("InvalidParameter", "Style " + str_style + " is not available for the layer " + layer->get_id(), 400);

        styles.push_back(style);
    }

    // Les options de format (non géré dans OGC API Maps)
    std::map<std::string, std::string> format_options ;

    // La résolution
    double res = 0;
    std::string str_res = req->get_query_param("mm-per-pixel");
    if (str_res != "") {
        if (sscanf(str_res.c_str(), "%lf", &res) != 1)
            throw OgcApiException::get_error_message("InvalidParameter", "Invalid mm-per-pixel value", 400);

        if ( res <= 0 )
            throw OgcApiException::get_error_message("InvalidParameter", "mm-per-pixel query parameter have to be a strictly positive number", 400);
    } else {
        res = 0.28;
    }

    // Le dpi
    int dpi = (int) round(25.4 / res);

    // Traitement de la requête
    std::string error;
    DataStream* d = Map::get_map(
        services, services->map_reprojection, services->map_max_tile_x, services->map_max_tile_y, 
        layers, width, height, crs, bbox, styles, format, format_options, dpi,
        &error
    );
    if (d == NULL) {
        throw OgcApiException::get_error_message("InvalidParameter", error, 400);
    }
    return d;
}