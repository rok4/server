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
 * \file services/tiles/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe TilesService
 ** \~english
 * \brief Implements classe TilesService
 */

#include <iostream>

#include "services/tiles/Exception.h"
#include "services/tiles/Service.h"
#include "Rok4Server.h"

DataStream* TilesService::get_tilematrixsets ( Request* req, Rok4Server* serv ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    json11::Json::object res = json11::Json::object {};

    std::vector<json11::Json::object> tmss;
    
    for(auto const& t: TmsBook::get_book()) {
        tmss.push_back(json11::Json::object {
            { "id", t.second->get_id()},
            { "title", t.second->get_title()},
            { "crs", t.second->get_crs()->get_url()},
            { "links", json11::Json::array {
                json11::Json::object {
                    { "href", endpoint_uri + "/tileMatrixSets/" + t.second->get_id() + "?f=json"},
                    { "rel", "describedby"},
                    { "type", "application/json"},
                    { "title", t.second->get_id() + " definition as application/json"}
                }
            }}
        });
    }

    res["tileMatrixSets"] = tmss;

    return new MessageDataStream ( json11::Json{ res }.dump(), "application/json", 200 );
}


DataStream* TilesService::get_tilematrixset ( Request* req, Rok4Server* serv ) {

    std::string f = req->get_query_param("f");
    if (f != "" && f != "application/json" && f != "json") {
        throw TilesException::get_error_message("InvalidParameter", "Format unknown", 400);
    }

    // Le TMS
    std::string str_tms = req->path_params.at(0);
    if ( contain_chars(str_tms, "\"")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TILES tms: " << str_tms ;
        throw TilesException::get_error_message("ResourceNotFound", "Tile matrix set unknown", 404);
    }

    TileMatrixSet* tms = TmsBook::get_tms(str_tms);
    if ( tms == NULL) {
        throw TilesException::get_error_message("ResourceNotFound", "Tile matrix set "+str_tms+" unknown", 404);
    }

    json11::Json::object res = json11::Json::object {
        { "id", tms->get_id()},
        { "title", tms->get_title()},
        { "description", tms->get_abstract()},
        { "keywords", *(tms->get_keywords())},
        { "crs", tms->get_crs()->get_url()},
    };

    std::vector<json11::Json> tile_matrices;
    
    for(TileMatrix* tm: tms->get_ordered_tm(false)) {
        tile_matrices.push_back(json11::Json::object {
            { "id", tm->get_id()},
            { "cellSize", tm->get_res()},
            { "cornerOfOrigin", "topLeft"},
            { "pointOfOrigin", json11::Json::array{ tm->get_x0(), tm->get_y0()}},
            { "tileWidth", tm->get_tile_width()},
            { "tileHeight", tm->get_tile_height()},
            { "matrixHeight", int(tm->get_matrix_width())},
            { "matrixWidth", int(tm->get_matrix_height())}
        });
    }

    res["tileMatrices"] = tile_matrices;

    return new MessageDataStream ( json11::Json{ res }.dump(), "application/json", 200 );
}

