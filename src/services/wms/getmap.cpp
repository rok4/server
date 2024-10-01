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
 * \file services/wms/getmap.cpp
 ** \~french
 * \brief Implémentation de la classe WmsService
 ** \~english
 * \brief Implements classe WmsService
 */

#include <iostream>
#include <boost/algorithm/string.hpp>

#include <rok4/utils/Cache.h>
#include <rok4/image/PaletteImage.h>
#include <rok4/image/MergeImage.h>
#include <rok4/datastream/AscEncoder.h>
#include <rok4/datastream/BilEncoder.h>
#include <rok4/datastream/JPEGEncoder.h>
#include <rok4/datastream/PNGEncoder.h>
#include <rok4/datastream/TiffEncoder.h>
#include <rok4/datastream/TiffPackBitsEncoder.h>
#include <rok4/datastream/TiffLZWEncoder.h>
#include <rok4/datastream/TiffDeflateEncoder.h>
#include <rok4/datastream/TiffRawEncoder.h>

#include "services/wms/Exception.h"
#include "services/wms/Service.h"
#include "Rok4Server.h"

DataStream* WmsService::get_map ( Request* req, Rok4Server* serv ) {

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
            if (! crs_equals && (! reprojection || ! layers.at ( i )->is_available_crs(str_crs)) ) {
                throw WmsException::get_error_message("CRS is not available for the layer " + layers.at ( i )->get_id(), "InvalidParameterValue", 400);
            }
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

    if (! is_available_format(format)) {
        throw WmsException::get_error_message("Format " + format + " unknown", "InvalidParameterValue", 400);
    }

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

    // Le dpi
    int dpi = 0;
    std::string str_dpi = req->get_query_param("str_dpi");
    if (str_dpi != "") {
        if (sscanf(str_dpi.c_str(), "%d", &dpi) != 1)
            throw WmsException::get_error_message("Invalid DPI value", "InvalidParameterValue", 400);

        if ( width < 0 )
            throw WmsException::get_error_message("DPI query parameter have to be a positive integer", "InvalidParameterValue", 400);
    }

    // Traitement de la requête

    std::vector<Image*> images;

    // Le format des canaux sera identifié à partir des données en entrée, en prenant en compte le style
    SampleFormat::eSampleFormat sample_format = SampleFormat::UNKNOWN;

    // Le nombre de canaux dans l'image finale sera égale au nombre maximum dans les données en entrée (en prenant en compte le style)
    int bands = 0;

    for (int i = 0; i < layers.size(); i++) {
        bool crs_equals = serv->get_services_configuration()->are_crs_equals(crs->get_request_code(), layers.at(i)->get_pyramid()->get_tms()->get_crs()->get_request_code());

        if (! crs_equals && ! reprojection) {
            throw WmsException::get_error_message("Reprojection is not available", "InvalidParameterValue", 400);
        }

        Style* style = styles.at(i);

        // On vérifie que toutes les images ont bien le même format de canal, après application du style
        if (sample_format != SampleFormat::UNKNOWN && sample_format != style->get_sample_format(layers.at(i)->get_pyramid()->get_sample_format())) {
            throw WmsException::get_error_message("All layers (with their style) have to own the same sample format (int or float)", "InvalidParameterValue", 400);
        } else {
            sample_format = style->get_sample_format(layers.at(i)->get_pyramid()->get_sample_format());
        }


        Image* image = layers.at(i)->get_pyramid()->getbbox(max_tile_x, max_tile_y, bbox, width, height, crs, crs_equals, layers.at(i)->get_resampling(), dpi);

        if (image == NULL) {
            throw WmsException::get_error_message("BBOX too big", "InvalidParameterValue", 400);
        }

        image = new PaletteImage(image, style->get_palette());

        images.push_back(image);

        // Le nombre final de canaux est celui maxiumum parmis les couches, c'est à dire celui de la donnée en prenant en compte le style
        bands = std::max(bands, style->get_channels(layers.at(i)->get_pyramid()->get_channels()));
    }

    // On construit la réponse finale, en superposant les couches
   
    Image* final_image;

    if (images.size() >= 2) {
        int background_value[bands];
        memset(background_value, 0, bands * sizeof(int));
        final_image = MergeImage::create(images, bands, background_value, NULL, Merge::ALPHATOP);
    } else {
        final_image = images.at(0);
    }

    final_image->set_crs(crs);

    if (format == "image/png" && sample_format == SampleFormat::UINT8) {
        return new PNGEncoder(final_image, NULL);
    }
    else if (format == "image/tiff" || format == "image/geotiff") {
        bool is_geotiff = (format == "image/geotiff");

        std::map<std::string, std::string>::iterator it = format_options.find("compression");
        std::string opt = "";
        if (it != format_options.end()) {
            opt = it->second;
        }

        if (sample_format == SampleFormat::UINT8) {
            if (opt.compare("lzw") == 0) { 
                return new TiffLZWEncoder<uint8_t>(final_image, is_geotiff);
            }
            if (opt.compare("deflate") == 0) {
                return new TiffDeflateEncoder<uint8_t>(final_image, is_geotiff);
            }
            if (opt.compare("raw") == 0 || opt == "") {
                return new TiffRawEncoder<uint8_t>(final_image, is_geotiff);
            }
            if (opt.compare("packbits") == 0) {
                return new TiffPackBitsEncoder<uint8_t>(final_image, is_geotiff);
            }
        }
        else if (sample_format == SampleFormat::FLOAT32) {
            if (opt.compare("lzw") == 0) { 
                return new TiffLZWEncoder<float>(final_image, is_geotiff);
            }
            if (opt.compare("deflate") == 0) {
                return new TiffDeflateEncoder<float>(final_image, is_geotiff);
            }
            if (opt.compare("raw") == 0 || opt == "") {
                return new TiffRawEncoder<float>(final_image, is_geotiff);
            }
            if (opt.compare("packbits") == 0) {
                return new TiffPackBitsEncoder<float>(final_image, is_geotiff);
            }
        }

        delete final_image;
        throw WmsException::get_error_message("Used data and expected format are not consistent", "InvalidParameterValue", 400);
    }
    else if (format == "image/jpeg" && sample_format == SampleFormat::UINT8) {

        std::map<std::string, std::string>::iterator it = format_options.find("quality");
        int quality = 75;
        if (it != format_options.end()) {
            // si on n'arrive pas à paser en entier l'option, on remet 75
            if (sscanf(it->second.c_str(), "%d", &quality) != 1) quality = 75;
        }

        return new JPEGEncoder(final_image, quality);
    }
    else if (format == "image/x-bil;bits=32" && sample_format == SampleFormat::FLOAT32) {
        return new BilEncoder(final_image);
    }
    else if (format == "text/asc" && sample_format == SampleFormat::FLOAT32 && final_image->get_channels() == 1) {
        return new AscEncoder(final_image);
    } else {

        delete final_image;
        throw WmsException::get_error_message("Used data format (" + std::to_string(bands) + " band(s) " + SampleFormat::to_string(sample_format) + ") and expected output format (" + format + ") are not consistent", "InvalidParameterValue", 400);
    }

    return NULL;
}