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
 * \file services/tms/gettile.cpp
 ** \~french
 * \brief Implémentation de la classe TmsService
 ** \~english
 * \brief Implements classe TmsService
 */

#include <iostream>

#include <rok4/datasource/PaletteDataSource.h>

#include "services/tms/Exception.h"
#include "services/tms/Service.h"
#include "Rok4Server.h"

DataStream* TmsService::get_tile ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->path_params.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->path_params.at(1);
    if ( contain_chars(str_layer, "<>")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " + str_layer + " unknown", 400);
    }

    // Le niveau
    TileMatrixSet* tms = layer->get_pyramid()->get_tms();

    std::string str_tm = req->path_params.at(2);

    if ( contain_chars(str_tm, "<>")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS tileMatrix: " << str_tm ;
        throw TmsException::get_error_message("Level unknown", 400);
    }

    TileMatrix* tm = tms->get_tm(str_tm);
    if ( tm == NULL )
        throw TmsException::get_error_message("Level " + str_tm + " unknown", 400);


    // La colonne
    int column;
    std::string str_column = req->path_params.at(3);
    if ( sscanf ( str_column.c_str(),"%d",&column ) != 1 )
        throw TmsException::get_error_message("Invalid column value", 400);

    // La ligne     
    int row;
    std::string str_row = req->path_params.at(4);
    if ( sscanf ( str_row.c_str(),"%d",&row ) != 1 )
        throw TmsException::get_error_message("Invalid row value", 400);

    std::string extension = req->path_params.at(5);

    TileMatrixLimits* tml = layer->get_tilematrix_limits(tms, tm);
    if (tml == NULL) {
        // On est hors niveau -> erreur
        throw TmsException::get_error_message("No data found", 404);
    }

    if (! tml->contain_tile(column, row)) {
        // On est hors tuiles -> erreur
        throw TmsException::get_error_message("No data found", 404);
    }

    // Le style
    Style* style;
    if (Rok4Format::is_raster(layer->get_pyramid()->get_format())) {
        style = layer->get_default_style();
    }

    // Le format : on vérifie la cohérence de l'extension avec le format des données

    if ( extension == "" )
        throw TmsException::get_error_message("Empty extension", 400);

    if ( contain_chars(extension, "<>")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS extension: " << extension ;
        throw TmsException::get_error_message("Invalid extension", 400);
    }

    if ( extension.compare ( Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) ) != 0 ) {
        throw TmsException::get_error_message("Invalid extension " + extension, 400);
    }

    std::string format = Rok4Format::to_mime_type ( ( layer->get_pyramid()->get_format() ) );

    DataSource* d = layer->get_pyramid()->get_level(tm->get_id())->get_tile(column, row);
    if (d == NULL) {
        throw TmsException::get_error_message("No data found", 404);
    }

    if (format == "image/png" && style->get_palette() && ! style->get_palette()->is_empty()) {
        return new DataStreamFromDataSource(new PaletteDataSource(d, style->get_palette()));
    } else {
        return new DataStreamFromDataSource(d);
    }
}