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
 * \file services/wmts/gettile.cpp
 ** \~french
 * \brief Implémentation de la classe WmtsService
 ** \~english
 * \brief Implements classe WmtsService
 */

#include <rok4/datasource/PaletteDataSource.h>
#include <rok4/datastream/AscEncoder.h>
#include <rok4/datastream/BilEncoder.h>
#include <rok4/datastream/JPEGEncoder.h>
#include <rok4/datastream/PNGEncoder.h>
#include <rok4/datastream/TiffEncoder.h>
#include <rok4/image/PaletteImage.h>

#include <iostream>

#include "Rok4Server.h"
#include "services/wmts/Exception.h"
#include "services/wmts/Service.h"

DataStream* format_image(Image* image, std::string format, Rok4Format::eFormat pyr_type,
                         std::map<std::string, std::string> format_option,
                         int size, ) {
    if (format == "image/png") {
        if (size == 1) {
            return new PNGEncoder(image, style->get_palette());
        } else {
            return new PNGEncoder(image, NULL);
        }

    } else if (format == "image/tiff" || format == "image/geotiff") {
        switch (pyr_type) {
            case Rok4Format::TIFF_RAW_FLOAT32:
            case Rok4Format::TIFF_ZIP_FLOAT32:
            case Rok4Format::TIFF_LZW_FLOAT32:
            case Rok4Format::TIFF_PKB_FLOAT32:

                if (opt.compare("lzw") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_LZW_FLOAT32, is_geotiff);
                }
                if (opt.compare("deflate") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_ZIP_FLOAT32, is_geotiff);
                }
                if (opt.compare("raw") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_RAW_FLOAT32, is_geotiff);
                }
                if (opt.compare("packbits") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_PKB_FLOAT32, is_geotiff);
                }
                return TiffEncoder::get_tiff_encoder(image, pyr_type, is_geotiff);
            case Rok4Format::TIFF_RAW_UINT8:
            case Rok4Format::TIFF_ZIP_UINT8:
            case Rok4Format::TIFF_LZW_UINT8:
            case Rok4Format::TIFF_PKB_UINT8:
                if (opt.compare("lzw") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_LZW_UINT8, is_geotiff);
                }
                if (opt.compare("deflate") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_ZIP_UINT8, is_geotiff);
                }
                if (opt.compare("raw") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_RAW_UINT8, is_geotiff);
                }
                if (opt.compare("packbits") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_PKB_UINT8, is_geotiff);
                }
                return TiffEncoder::get_tiff_encoder(image, pyr_type, is_geotiff);
            default:
                if (opt.compare("lzw") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_LZW_UINT8, is_geotiff);
                }
                if (opt.compare("deflate") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_ZIP_UINT8, is_geotiff);
                }
                if (opt.compare("packbits") == 0) {
                    return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_PKB_UINT8, is_geotiff);
                }
                return TiffEncoder::get_tiff_encoder(image, Rok4Format::TIFF_RAW_UINT8, is_geotiff);
        }
    } else if (format == "image/jpeg") {
        int quality = 75;
        if (pyr_type == Rok4Format::TIFF_JPG90_UINT8 || getParam(format_option, "quality").compare("90") == 0) {
            quality = 90;
        }

        return new JPEGEncoder(image, quality);
    } else if (format == "image/x-bil;bits=32") {
        return new BilEncoder(image);
    } else if (format == "text/asc") {
        // On ne traite le format asc que sur les image à un seul channel
        if (image->get_channels() != 1) {
            BOOST_LOG_TRIVIAL(error) << "Le format " << format << " ne concerne que les images à 1 canal";
        } else {
            return new AscEncoder(image);
        }
    }

    BOOST_LOG_TRIVIAL(error) << "Le format " << format << " ne peut etre traite";

    return new SERDataStream(new ServiceException("", WMS_INVALID_FORMAT, "Le format " + format + " ne peut etre traite", "wms"));
}

DataStream* WmtsService::get_tile(Request* req, Rok4Server* serv) {
    // La couche
    std::string str_layer = req->get_query_param("layer");
    if (str_layer == "") throw WmtsException::get_error_message("LAYER query parameter missing", "MissingParameterValue", 400);

    if (containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS layer: " << str_layer;
        throw WmtsException::get_error_message("Layer unknown", "InvalidParameterValue", 400);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if (layer == NULL || !layer->is_wmts_enabled()) {
        throw WmtsException::get_error_message("Layer " + str_layer + " unknown", "InvalidParameterValue", 400);
    }

    // Le tile matrix set
    std::string str_tms = req->get_query_param("tilematrixset");
    if (str_tms == "") throw WmtsException::get_error_message("TILEMATRIXSET query parameter missing", "MissingParameterValue", 400);

    if (containForbiddenChars(str_tms)) {
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

    if (containForbiddenChars(str_tm)) {
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS tile matrix: " << str_tm;
        throw WmtsException::get_error_message("Tile matrix unknown", "InvalidParameterValue", 400);
    }

    TileMatrix* tm = tms->getTm(str_tm);
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
    if (!tml->containTile(column, row)) {
        // On est hors tuiles -> erreur
        throw WmtsException::get_error_message("No data found", "Not Found", 404);
    }

    // Le format
    std::string format = req->get_query_param("format");
    if (format == "") throw WmtsException::get_error_message("FORMAT query parameter missing", "MissingParameterValue", 400);

    if (containForbiddenChars(format)) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) << "Forbidden char detected in WMTS format: " << format;
        throw WmtsException::get_error_message("Format unknown", "InvalidParameterValue", 400);
    }

    if (format.compare(Rok4Format::to_mime_type((layer->get_pyramid()->getFormat()))) != 0)
        throw WmtsException::get_error_message("Format " + format + " unknown", "InvalidParameterValue", 400);

    std::string str_style = req->get_query_param("style");
    if (str_style == "") throw WmtsException::get_error_message("STYLE query parameter missing", "MissingParameterValue", 400);

    Style* style = NULL;
    if (Rok4Format::is_raster(layer->get_pyramid()->getFormat())) {
        style = layer->get_style_by_identifier(str_style);

        if (style == NULL) throw WmtsException::get_error_message("Style " + str_style + " unknown", "InvalidParameterValue", 400);
    }

    if (tms->getId() == layer->get_pyramid()->getTms()->getId()) {
        // TMS d'interrogation natif
        Level* level = layer->get_pyramid()->getLevel(tm->getId());

        DataSource* d = level->getTile(column, row);
        if (d == NULL) {
            throw WmtsException::get_error_message("No data found", "Not Found", 404);
        }

        if (format == "image/png" && style->get_palette() && !style->get_palette()->is_empty()) {
            return new DataStreamFromDataSource(new PaletteDataSource(d, style->get_palette()));
        } else {
            return new DataStreamFromDataSource(d);
        }
    } else {
        // TMS d'interrogation à la demande

        BoundingBox<double> bbox = tm->tileIndicesToBbox(column, row);
        int height = tm->getTileH();
        int width = tm->getTileW();
        CRS* crs = tms->getCrs();
        bbox.crs = crs->getRequestCode();

        bool crs_equals = serv->get_services_configuration()->are_crs_equals(layer->get_pyramid()->getTms()->getCrs()->getProjCode(), crs->getProjCode());

        int error;
        // On se donne maxium 3 tuiles sur 3 dans la pyramide source pour calculer cette tuile
        Image* image = layer->get_pyramid()->getbbox(3, 3, bbox, width, height, crs, crs_equals, layer->get_resampling(), 0, error);

        if (image == NULL) {
            BOOST_LOG_TRIVIAL(warning) << "Cannot process the tile in a non native TMS";
            throw WmtsException::get_error_message("No data found", "Not Found", 404);
        }

        image->set_bbox(bbox);
        image->set_crs(crs);

        // La classe PaletteImage appliquera la palette que si elle peut (donnée 1 canal et palette non vide)
        Image* styled_image = new PaletteImage(image, style->get_palette());

        if (styled_image == 0) {
            BOOST_LOG_TRIVIAL(warning) << "Cannot apply palette to the tile in a non native TMS";
            throw WmtsException::get_error_message("No data found", "Not Found", 404);
        }

        std::map<std::string, std::string> format_options = Utils::string_to_map(req->get_query_param("format_options"), ";", ":");

        if (format == "image/png") {
            return new PNGEncoder(image, style->get_palette());
        } else if (format == "image/tiff" || format == "image/geotiff") {
            bool is_geotiff = (format == "image/geotiff");

            std::map<std::string, std::string>::iterator it = format_options.find("compression");
            std::string opt = "";
            if (it != format_options.end()) {
                opt = it->second;
            }

            // La donnée ne peut être retournée que dans le format de la pyramide source utilisée

            switch (layer->get_pyramid()->getFormat()) {
                case Rok4Format::TIFF_RAW_UINT8:
                case Rok4Format::TIFF_LZW_UINT8:
                case Rok4Format::TIFF_ZIP_UINT8:
                case Rok4Format::TIFF_PKB_UINT8:
                    if (opt.compare("lzw") == 0) { 
                        return new TiffLZWEncoder<uint8_t>(image, is_geotiff);
                    }
                    if (opt.compare("deflate") == 0) {
                        return new TiffRawEncoder<uint8_t>(image, is_geotiff);
                    }
                    if (opt.compare("raw") == 0) {
                        return new TiffDeflateEncoder<uint8_t>(image, is_geotiff);
                    }
                    if (opt.compare("packbits") == 0) {
                        return new TiffPackBitsEncoder<uint8_t>(image, is_geotiff);
                    }

                case Rok4Format::TIFF_RAW_FLOAT32:
                    return new TiffRawEncoder<float>(image, is_geotiff);
                case Rok4Format::TIFF_LZW_FLOAT32:
                    return new TiffLZWEncoder<float>(image, is_geotiff);
                case Rok4Format::TIFF_ZIP_FLOAT32:
                    return new TiffDeflateEncoder<float>(image, is_geotiff);
                case Rok4Format::TIFF_PKB_FLOAT32:
                    return new TiffPackBitsEncoder<float>(image, is_geotiff);
                default:
                    throw WmtsException::get_error_message("No data found", "Not Found", 404);
            }
        }
        else if (format == "image/jpeg") {

            std::map<std::string, std::string>::iterator it = format_options.find("quality");
            int quality = 75;
            if (it != format_options.end()) {
                // si on n'arrive pas à paser en entier l'option, on remet 75
                if (sscanf(it->second.c_str(), "%d", &quality) != 1) quality = 75;
            }

            return new JPEGEncoder(image, quality);
        }
        else if (format == "image/x-bil;bits=32") {
            return new BilEncoder(image);
        }
        else if (format == "text/asc") {
            // On ne traite le format asc que sur les image à un seul channel
            if (image->get_channels() != 1) {
                BOOST_LOG_TRIVIAL(error) << "Le format " << format << " ne concerne que les images à 1 canal";
            } else {
                return new AscEncoder(image);
            }
        }
    }

    return TiffEncoder::get_tiff_encoder(image, pyr_type, is_geotiff);

    switch (format) {
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
            return NULL;
    }

    return stream;
}
}