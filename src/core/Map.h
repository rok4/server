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

#include <rok4/datastream/AscEncoder.h>
#include <rok4/datastream/BilEncoder.h>
#include <rok4/datastream/JPEGEncoder.h>
#include <rok4/datastream/PNGEncoder.h>
#include <rok4/datastream/TiffDeflateEncoder.h>
#include <rok4/datastream/TiffEncoder.h>
#include <rok4/datastream/TiffLZWEncoder.h>
#include <rok4/datastream/TiffPackBitsEncoder.h>
#include <rok4/datastream/TiffRawEncoder.h>
#include <rok4/image/MergeImage.h>
#include <rok4/image/StyledImage.h>

#include <iomanip>
#include <string>
#include <vector>

#include "configurations/Layer.h"

namespace Map {

/**
 * \~french
 * \brief Retourne l'image demandée
 * \return Flux de donnée
 * \~english
 * \brief Give the asked image
 * \return Data stream
 */
static DataStream* get_map(
    ServicesConfiguration* services, bool reprojection, int max_tile_x, int max_tile_y,
    std::vector<Layer*> layers, int width, int height, CRS* crs, BoundingBox<double> bbox, std::vector<Style*> styles,
    std::string format, std::map<std::string, std::string> format_options, int dpi, std::string* error
) {
    // Traitement de la requête

    std::vector<Image*> images;

    // Le format des canaux sera identifié à partir des données en entrée, en prenant en compte le style
    SampleFormat::eSampleFormat sample_format = SampleFormat::UNKNOWN;
    int nodata;

    // Le nombre de canaux dans l'image finale sera égale au nombre maximum dans les données en entrée (en prenant en compte le style)
    int bands = 0;

    for (int i = 0; i < layers.size(); i++) {
        bool crs_equals = services->are_crs_equals(crs->get_request_code(), layers.at(i)->get_pyramid()->get_tms()->get_crs()->get_request_code());

        if (!crs_equals && !reprojection) {
            *error = "Reprojection is not available";
            return NULL;
        }

        Style* style = styles.at(i);

        // On vérifie que toutes les images ont bien le même format de canal, après application du style
        if (sample_format != SampleFormat::UNKNOWN && sample_format != style->get_sample_format(layers.at(i)->get_pyramid()->get_sample_format())) {
            *error = "All layers (with their style) have to own the same sample format (int or float)";
            return NULL;
        } else {
            sample_format = style->get_sample_format(layers.at(i)->get_pyramid()->get_sample_format());

            // Dans le cas d'un geotiff, on renseigne la valeur de nodata
            // on ne peut mettre qu'une valeur, ce sera celle du premier canal du nodata de la première couche (après style)
            nodata = *(style->get_output_nodata_value(layers.at(i)->get_pyramid()->get_nodata_value()));
        }

        Image* image = layers.at(i)->get_pyramid()->getbbox(max_tile_x, max_tile_y, bbox, width, height, crs, crs_equals, layers.at(i)->get_resampling(), dpi);

        if (image == NULL) {
            *error = "BBOX too big";
            return NULL;
        }

        StyledImage* s_image = StyledImage::create(image, style);
        image = s_image;
        images.push_back(image);

        // Le nombre final de canaux est celui maxiumum parmis les couches, c'est à dire celui de la donnée en prenant en compte le style
        bands = std::max(bands, image->get_channels());
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

    final_image->set_bbox(bbox);
    final_image->set_crs(crs);

    if (format == "image/png" && sample_format == SampleFormat::UINT8) {
        return new PNGEncoder(final_image, NULL);
    } else if (format == "image/tiff" || format == "image/geotiff") {
        bool is_geotiff = (format == "image/geotiff");

        std::map<std::string, std::string>::iterator it = format_options.find("compression");
        std::string opt = "";
        if (it != format_options.end()) {
            opt = it->second;
        }

        if (sample_format == SampleFormat::UINT8) {
            if (opt.compare("lzw") == 0) {
                return new TiffLZWEncoder<uint8_t>(final_image, is_geotiff, nodata);
            }
            if (opt.compare("deflate") == 0) {
                return new TiffDeflateEncoder<uint8_t>(final_image, is_geotiff, nodata);
            }
            if (opt.compare("raw") == 0 || opt == "") {
                return new TiffRawEncoder<uint8_t>(final_image, is_geotiff, nodata);
            }
            if (opt.compare("packbits") == 0) {
                return new TiffPackBitsEncoder<uint8_t>(final_image, is_geotiff, nodata);
            }
        } else if (sample_format == SampleFormat::FLOAT32) {
            if (opt.compare("lzw") == 0) {
                return new TiffLZWEncoder<float>(final_image, is_geotiff, nodata);
            }
            if (opt.compare("deflate") == 0) {
                return new TiffDeflateEncoder<float>(final_image, is_geotiff, nodata);
            }
            if (opt.compare("raw") == 0 || opt == "") {
                return new TiffRawEncoder<float>(final_image, is_geotiff, nodata);
            }
            if (opt.compare("packbits") == 0) {
                return new TiffPackBitsEncoder<float>(final_image, is_geotiff, nodata);
            }
        }

        delete final_image;
        *error = "Used data and expected format are not consistent";
        return NULL;
    } else if (format == "image/jpeg" && sample_format == SampleFormat::UINT8 && bands == 3) {
        std::map<std::string, std::string>::iterator it = format_options.find("quality");
        int quality = 75;
        if (it != format_options.end()) {
            // si on n'arrive pas à paser en entier l'option, on remet 75
            if (sscanf(it->second.c_str(), "%d", &quality) != 1) quality = 75;
        }

        return new JPEGEncoder(final_image, quality);
    } else if (format == "image/x-bil;bits=32" && sample_format == SampleFormat::FLOAT32) {
        return new BilEncoder(final_image);
    } else if (format == "text/asc" && sample_format == SampleFormat::FLOAT32 && final_image->get_channels() == 1) {
        return new AscEncoder(final_image);
    } else {
        delete final_image;
        *error = "Used data format (" + std::to_string(bands) + " band(s) " + SampleFormat::to_string(sample_format) + ") and expected output format (" + format + ") are not consistent";
        return NULL;
    }
    return NULL;
}
};  // namespace Map
