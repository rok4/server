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

#pragma once

#include <rok4/datasource/PaletteDataSource.h>
#include <rok4/datastream/BilEncoder.h>
#include <rok4/datastream/JPEGEncoder.h>
#include <rok4/datastream/PNGEncoder.h>
#include <rok4/datastream/TiffDeflateEncoder.h>
#include <rok4/datastream/TiffEncoder.h>
#include <rok4/datastream/TiffLZWEncoder.h>
#include <rok4/datastream/TiffPackBitsEncoder.h>
#include <rok4/datastream/TiffRawEncoder.h>

#include <iomanip>
#include <string>
#include <vector>

#include "configurations/Layer.h"

namespace Tile {
    
/**
 * \~french
 * \brief Retourne la tuile demandée
 * \return Flux de donnée
 * \~english
 * \brief Give the asked tile
 * \return Data stream
 */
static DataStream* get_tile(ServicesConfiguration* services, Layer* layer, TileMatrixSet* tms, TileMatrix* tm, int column, int row, std::string format, Style* style) {
    // Traitement de la requête

    if (tms->get_id() == layer->get_pyramid()->get_tms()->get_id()) {
        // TMS d'interrogation natif
        Level* level = layer->get_pyramid()->get_level(tm->get_id());

        DataSource* d = level->get_tile(column, row);
        if (d == NULL) {
            return NULL;
        }

        if (layer->get_pyramid()->get_channels() == 1 && format == "image/png" && style->get_palette() && !style->get_palette()->is_empty()) {
            return new DataStreamFromDataSource(new PaletteDataSource(d, style->get_palette()));
        } else {
            return new DataStreamFromDataSource(d);
        }
    } else {
        // TMS d'interrogation à la demande, forcément du raster

        BoundingBox<double> bbox = tm->tile_indices_to_bbox(column, row);
        int height = tm->get_tile_height();
        int width = tm->get_tile_width();
        CRS* crs = tms->get_crs();
        bbox.crs = crs->get_request_code();

        bool crs_equals = services->are_crs_equals(layer->get_pyramid()->get_tms()->get_crs()->get_proj_code(), crs->get_proj_code());

        // On se donne maxium 3 tuiles sur 3 dans la pyramide source pour calculer cette tuile
        Image* image = layer->get_pyramid()->getbbox(3, 3, bbox, width, height, crs, crs_equals, layer->get_resampling(), 0);

        if (image == NULL) {
            BOOST_LOG_TRIVIAL(warning) << "Cannot process the tile in a non native TMS";
            return NULL;
        }

        image->set_bbox(bbox);
        image->set_crs(crs);

        if (format == "image/png" || format == "png") {
            return new PNGEncoder(image, style->get_palette());
        } else if (format == "image/tiff" || format == "image/geotiff") {
            bool is_geotiff = (format == "image/geotiff");

            // Dans le cas d'un geotiff, on renseigne la valeur de nodata
            // on ne peut mettre qu'une valeur, ce sera celle du premier canal
            int nodata = *(layer->get_pyramid()->get_nodata_value());

            // La donnée ne peut être retournée que dans le format de la pyramide source utilisée

            switch (layer->get_pyramid()->get_format()) {
                case Rok4Format::TIFF_RAW_UINT8:
                    return new TiffRawEncoder<uint8_t>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_LZW_UINT8:
                    return new TiffLZWEncoder<uint8_t>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_ZIP_UINT8:
                    return new TiffDeflateEncoder<uint8_t>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_PKB_UINT8:
                    return new TiffPackBitsEncoder<uint8_t>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_RAW_FLOAT32:
                    return new TiffRawEncoder<float>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_LZW_FLOAT32:
                    return new TiffLZWEncoder<float>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_ZIP_FLOAT32:
                    return new TiffDeflateEncoder<float>(image, is_geotiff, nodata);
                case Rok4Format::TIFF_PKB_FLOAT32:
                    return new TiffPackBitsEncoder<float>(image, is_geotiff, nodata);
                default:
                    delete image;
                    return NULL;
            }
        } else if (format == "image/jpeg") {
            switch (layer->get_pyramid()->get_format()) {
                case Rok4Format::TIFF_JPG_UINT8:
                    return new JPEGEncoder(image, 75);
                case Rok4Format::TIFF_JPG90_UINT8:
                    return new JPEGEncoder(image, 90);
                default:
                    delete image;
                    return NULL;
            }

        } else if (format == "image/x-bil;bits=32") {
            return new BilEncoder(image);
        }
    }

    BOOST_LOG_TRIVIAL(error) << "On ne devrait pas passer par là";
    return NULL;
}
};  // namespace Tile
