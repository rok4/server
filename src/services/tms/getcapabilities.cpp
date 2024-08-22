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
 * \file services/tms/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe TmsService
 ** \~english
 * \brief Implements classe TmsService
 */

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;

#include <rok4/thirdparty/json11.hpp>

#include "services/tms/Exception.h"
#include "services/tms/Service.h"
#include "Rok4Server.h"

DataStream* TmsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    if ( req->path_params.at(0) != "1.0.0" ) {
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);
    }

    ptree tree;

    ptree& root = tree.add("TileMapService", "");
    root.add("<xmlattr>.version", "1.0.0");
    root.add("<xmlattr>.services", endpoint_uri );
    root.add("Title", title );
    root.add("Abstract", abstract );

    for ( unsigned int i=0; i < keywords.size(); i++ ) {
        root.add("KeywordList", keywords.at(i).get_content() );
    }

    serv->get_services_configuration()->contact->add_node_tms(root, serv->get_services_configuration()->service_provider);

    if (metadata) {
        metadata->add_node_tms(root);
    }

    ptree& contents_node = root.add("TileMaps", "");

    std::map<std::string, Layer*>::iterator layers_iterator ( serv->get_server_configuration()->get_layers().begin() ), layers_end ( serv->get_server_configuration()->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        Layer* layer = layers_iterator->second;

        if (layer->is_tms_enabled()) {
            std::string ln = std::regex_replace(layer->get_title(), std::regex("\""), "&quot;");

            ptree& layer_node = contents_node.add("TileMap", "");
            layer_node.add("<xmlattr>.title", ln);
            layer_node.add("<xmlattr>.srs", layer->get_pyramid()->get_tms()->get_crs()->get_request_code());
            layer_node.add("<xmlattr>.profile", "none");
            layer_node.add("<xmlattr>.extension", Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ));
            layer_node.add("<xmlattr>.href", endpoint_uri + "/1.0.0/" + layer->get_id());
        }
    }

    std::stringstream ss;
    write_xml(ss, tree);
    return new MessageDataStream ( ss.str(), "application/xml", 200 );
}

DataStream* TmsService::get_tiles ( Request* req, Rok4Server* serv ) {

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
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    ptree tree;

    ptree& root = tree.add("TileMap", "");
    root.add("<xmlattr>.version", "1.0.0");
    root.add("<xmlattr>.tilemapservice", endpoint_uri + "/1.0.0" );
    root.add("Title", layer->get_title() );

    for ( unsigned int i=0; i < layer->get_keywords()->size(); i++ ) {
        root.add("KeywordList", layer->get_keywords()->at(i).get_content() );
    }

    for ( unsigned int i = 0; i < layer->get_metadata().size(); i++ ) {
        layer->get_metadata().at(i).add_node_tms(root);
    }

    if (layer->get_attribution() != NULL) {
        layer->get_attribution()->add_node_tms(root);
    }

    root.add("SRS", layer->get_pyramid()->get_tms()->get_crs()->get_request_code() );

    root.add("BoundingBox.<xmlattr>.minx", layer->get_native_bbox().xmin );
    root.add("BoundingBox.<xmlattr>.maxx", layer->get_native_bbox().xmax );
    root.add("BoundingBox.<xmlattr>.miny", layer->get_native_bbox().ymin );
    root.add("BoundingBox.<xmlattr>.maxy", layer->get_native_bbox().ymax );

    TileMatrix* tm = layer->get_pyramid()->get_tms()->getTmList()->begin()->second;

    root.add("Origin.<xmlattr>.x", tm->get_x0() );
    root.add("Origin.<xmlattr>.y", tm->get_y0() );

    root.add("TileFormat.<xmlattr>.width", tm->get_tile_width() );
    root.add("TileFormat.<xmlattr>.height", tm->get_tile_height() );
    root.add("TileFormat.<xmlattr>.mime-type", Rok4Format::to_mime_type ( layer->get_pyramid()->get_format() ) );
    root.add("TileFormat.<xmlattr>.extension", Rok4Format::to_extension ( layer->get_pyramid()->get_format() ) );

    ptree& tilesets_node = root.add("TileMap", "");
    tilesets_node.add("<xmlattr>.profile", "none");

    int order = 0;

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->get_pyramid()->get_ordered_levels(false);

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;
        tm = level->get_tm();
        ptree& tileset_node = tilesets_node.add("TileSet", "");

        tileset_node.add("<xmlattr>.href", str(boost::format("%s/1.0.0/%s/%s") % endpoint_uri % layer->get_id() % tm->get_id()) );
        tileset_node.add("<xmlattr>.minrow", level->get_min_tile_row());
        tileset_node.add("<xmlattr>.maxrow", level->get_max_tile_row());
        tileset_node.add("<xmlattr>.mincol", level->get_min_tile_col());
        tileset_node.add("<xmlattr>.maxcol", level->get_max_tile_col());
        tileset_node.add("<xmlattr>.units-per-pixel", tm->get_res());
        tileset_node.add("<xmlattr>.order", order);

        order++;
    }

    std::stringstream ss;
    write_xml(ss, tree);
    return new MessageDataStream ( ss.str(), "application/xml", 200 );
}

DataStream* TmsService::get_metadata ( Request* req, Rok4Server* serv ) {

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
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->get_pyramid()->get_ordered_levels(true);

    int order = 0;
    std::string minzoom, maxzoom;

    std::vector<std::string> tables_names;
    std::map<std::string, Table*> tables_infos;
    std::map<std::string, int> mins;
    std::map<std::string, int> maxs;

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;

        if (order == 0) {
            // Le premier niveau lu est le plus détaillé, on va l'utiliser pour définir plusieurs choses
            maxzoom = level->get_id();
        }
        // Zooms min et max
        minzoom = level->get_id();

        std::vector<Table>* level_tables = level->get_tables();
        for (int i = 0; i < level_tables->size(); i++) {
            std::string t = level_tables->at(i).get_name();
            std::map<std::string, int>::iterator it = mins.find ( t );
            if ( it == mins.end() ) {
                tables_names.push_back(t);
                tables_infos.emplace ( t, &(level_tables->at(i)) );
                maxs.emplace ( t, std::stoi(level->get_id()) );
                mins.emplace ( t, std::stoi(level->get_id()) );
            } else {
                it->second = std::stoi(minzoom);
            }

        }

        order++;
    }

    json11::Json::object res = json11::Json::object {
        { "name", layer->get_id() },
        { "description", layer->get_abstract() },
        { "minzoom", std::stoi(minzoom) },
        { "maxzoom", std::stoi(maxzoom) },
        { "crs", layer->get_pyramid()->get_tms()->get_crs()->get_request_code() },
        { "center", json11::Json::array { 
            ((layer->get_geographical_bbox().xmax + layer->get_geographical_bbox().xmin) / 2.), 
            ((layer->get_geographical_bbox().ymax + layer->get_geographical_bbox().ymin) / 2.)
        } },
        { "bounds", json11::Json::array { 
            layer->get_geographical_bbox().xmin,
            layer->get_geographical_bbox().ymin,
            layer->get_geographical_bbox().xmax,
            layer->get_geographical_bbox().ymax 
        } },
        { "format", Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) },
        { "tiles", json11::Json::array { endpoint_uri + "/1.0.0/" + layer->get_id() + "/{z}/{x}/{y}." + Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) } },
    };


    if (! Rok4Format::is_raster(layer->get_pyramid()->get_format())) {
        std::vector<json11::Json> items;
        for (int i = 0; i < tables_names.size(); i++) {
            items.push_back(tables_infos.at(tables_names.at(i))->to_json(maxs.at(tables_names.at(i)),mins.at(tables_names.at(i))));
        }
        res["vector_layers"] = items;
    }

    return new MessageDataStream ( json11::Json{ res }.dump(), "application/json", 200 );
}

/*
<GDAL_WMS>
    <Service name="TMS">
        <ServerUrl>https://rok4.ign.fr/tms/1.0.0/LAYER/${z}/${x}/${y}.png</ServerUrl>
    </Service>
    <DataWindow>
        <UpperLeftX>-20037508.34</UpperLeftX>
        <UpperLeftY>20037508.34</UpperLeftY>
        <LowerRightX>20037508.34</LowerRightX>
        <LowerRightY>-20037508.34</LowerRightY>
        <TileLevel>18</TileLevel>
        <TileCountX>1</TileCountX>
        <TileCountY>1</TileCountY>
        <YOrigin>top</YOrigin>
    </DataWindow>
    <Projection>EPSG:3857</Projection>
    <BlockSizeX>256</BlockSizeX>
    <BlockSizeY>256</BlockSizeY>
    <BandsCount>4</BandsCount>
    <ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes>
</GDAL_WMS>
*/
DataStream* TmsService::get_gdal ( Request* req, Rok4Server* serv ) {

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
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    if (! Rok4Format::is_raster(layer->get_pyramid()->get_format())) {
        throw TmsException::get_error_message("Layer " +str_layer+" is vector data: cannot describe it with this format", 400);
    }

    if (! layer->get_pyramid()->get_tms()->is_qtree()) {
        throw TmsException::get_error_message("Layer " +str_layer+" data is not a quad tree: cannot describe it with this format", 400);
    }

    Level* best = layer->get_pyramid()->get_lowest_level();


    ptree tree;

    ptree& root = tree.add("GDAL_WMS", "");
    root.add("Service.<xmlattr>.name", "TMS");
    root.add("Service.ServerUrl", endpoint_uri + "/1.0.0/" + layer->get_id() + "/${z}/${x}/${y}." + Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ));

    root.add("DataWindow.UpperLeftX", best->get_tm()->get_x0());
    root.add("DataWindow.UpperLeftY", best->get_tm()->get_y0());
    root.add("DataWindow.LowerRightX", best->get_tm()->get_x0() + best->get_tm()->get_matrix_width() * best->get_tm()->get_tile_width() * best->get_tm()->get_res());
    root.add("DataWindow.LowerRightY", best->get_tm()->get_y0() - best->get_tm()->get_matrix_height() * best->get_tm()->get_tile_height() * best->get_tm()->get_res());
    root.add("DataWindow.TileLevel", best->get_tm()->get_id());
    root.add("DataWindow.TileCountX", 1);
    root.add("DataWindow.TileCountY", 1);
    root.add("DataWindow.YOrigin", "top");

    root.add("Projection", layer->get_pyramid()->get_tms()->get_crs()->get_proj_code());
    root.add("BlockSizeX", best->get_tm()->get_tile_width());
    root.add("BlockSizeY", best->get_tm()->get_tile_height());
    root.add("ZeroBlockHttpCodes", "204,404");
    root.add("BandsCount", layer->get_pyramid()->get_channels());

    std::stringstream ss;
    write_xml(ss, tree);
    return new MessageDataStream ( ss.str(), "application/xml", 200 );
}
