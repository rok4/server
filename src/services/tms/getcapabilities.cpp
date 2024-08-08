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

#include "services/tms/Exception.h"
#include "services/tms/Service.h"
#include "Rok4Server.h"

DataStream* TmsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    if ( req->path_params.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    std::string res = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res += "<TileMapService version=\"1.0.0\" services=\"" + endpoint_uri + "\">\n";
    res += "  <Title>" + title + "</Title>\n";
    res += "  <Abstract>" + abstract + "</Abstract>\n";

    if ( metadata ) {
        res += "  <Metadata type=\"" + metadata->get_type() + "\" mime-type=\"text/xml\" href=\"" + metadata->get_href() + "\" />\n";
    }

    res += "  <TileMaps>\n";

    std::map<std::string, Layer*>::iterator itLay ( serv->get_server_configuration()->get_layers().begin() ), itLayEnd ( serv->get_server_configuration()->get_layers().end() );
    for ( ; itLay != itLayEnd; ++itLay ) {
        Layer* lay = itLay->second;

        if (lay->is_tms_enabled()) {
            std::string ln = std::regex_replace(lay->get_title(), std::regex("\""), "&quot;");
            res += "    <TileMap\n";
            res += "      title=\"" + ln + "\" \n";
            res += "      srs=\"" + lay->get_pyramid()->get_tms()->get_crs()->get_request_code() + "\" \n";
            res += "      profile=\"none\" \n";
            res += "      extension=\"" + Rok4Format::to_extension ( ( lay->get_pyramid()->get_format() ) ) + "\" \n";
            res += "      href=\"" + endpoint_uri + "/1.0.0/" + lay->get_id() + "\" />\n";
        }
    }

    res += "  </TileMaps>\n";
    res += "</TileMapService>\n";

    return new MessageDataStream ( res, "application/xml", 200 );
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

    std::ostringstream res;
    res << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res << "<TileMap version=\"1.0.0\" tilemapservice=\"" + endpoint_uri + "/1.0.0\">\n";
    res << "  <Title>" << layer->get_title() << "</Title>\n";

    for ( unsigned int i=0; i < layer->get_keywords()->size(); i++ ) {
        res << "  <KeywordList>" << layer->get_keywords()->at(i).get_content() << "</KeywordList>\n";
    }

    for ( unsigned int i = 0; i < layer->get_metadata().size(); i++ ) {
        res << "  " << layer->get_metadata().at(i).get_tms_xml();
    }

    if (layer->get_attribution() != NULL) {
        res << "  " << layer->get_attribution()->get_tms_xml();
    }

    res << "  <SRS>" << layer->get_pyramid()->get_tms()->get_crs()->get_request_code() << "</SRS>\n";
    res << "  <BoundingBox minx=\"" <<
        layer->get_native_bbox().xmin << "\" miny=\"" <<
        layer->get_native_bbox().ymin << "\" maxx=\"" <<
        layer->get_native_bbox().xmax << "\" maxy=\"" <<
        layer->get_native_bbox().ymax << "\" />\n";

    TileMatrix* tm = layer->get_pyramid()->get_tms()->getTmList()->begin()->second;

    res << "  <Origin x=\"" << tm->get_x0() << "\" y=\"" << tm->get_y0() << "\" />\n";

    res << "  <TileFormat width=\"" << tm->get_tile_width() << 
        "\" height=\"" << tm->get_tile_height() <<
        "\" mime-type=\"" << Rok4Format::to_mime_type ( ( layer->get_pyramid()->get_format() ) ) << 
        "\" extension=\"" << Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) << "\" />\n";

    res << "  <TileSets profile=\"none\">\n";

    int order = 0;

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->get_pyramid()->get_ordered_levels(false);

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;
        tm = level->get_tm();
        res << "    <TileSet href=\"" << endpoint_uri << "/1.0.0/" << layer->get_id() << "/" << tm->getId() << 
            "\" minrow=\"" << level->get_min_tile_row() << "\" maxrow=\"" << level->get_max_tile_row() << 
            "\" mincol=\"" << level->get_min_tile_col() << "\" maxcol=\"" << level->get_max_tile_col() <<
            "\" units-per-pixel=\"" << tm->get_res() << "\" order=\"" << order << "\" />\n";
        order++;
    }

    res << "  </TileSets>\n";
    res << "</TileMap>\n";

    return new MessageDataStream ( res.str(), "application/xml", 200 );
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

    std::vector<std::string> tablesNames;
    std::map<std::string, Table*> tablesInfos;
    std::map<std::string, std::string> mins;
    std::map<std::string, std::string> maxs;

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;

        if (order == 0) {
            // Le premier niveau lu est le plus détaillé, on va l'utiliser pour définir plusieurs choses
            maxzoom = level->get_id();
        }
        // Zooms min et max
        minzoom = level->get_id();

        std::vector<Table>* levelTables = level->get_tables();
        for (int i = 0; i < levelTables->size(); i++) {
            std::string t = levelTables->at(i).get_name();
            std::map<std::string, std::string>::iterator it = mins.find ( t );
            if ( it == mins.end() ) {
                tablesNames.push_back(t);
                tablesInfos.insert ( std::pair<std::string, Table*> ( t, &(levelTables->at(i)) ) );
                maxs.insert ( std::pair<std::string, std::string> ( t, level->get_id() ) );
                mins.insert ( std::pair<std::string, std::string> ( t, level->get_id() ) );
            } else {
                it->second = minzoom;
            }

        }

        order++;
    }

    std::ostringstream res;
    std::string jsondesc;
    res << "{\n";
    res << "  \"name\": \"" << layer->get_id() << "\",\n";
    res << "  \"description\": \"" << layer->get_abstract() << "\",\n";
    res << "  \"minzoom\": " << minzoom << ",\n";
    res << "  \"maxzoom\": " << maxzoom << ",\n";
    res << "  \"crs\": \"" << layer->get_pyramid()->get_tms()->get_crs()->get_request_code() << "\",\n";

    res << std::fixed << "  \"center\": [" <<
        ((layer->get_geographical_bbox().xmax + layer->get_geographical_bbox().xmin) / 2.) << "," << 
        ((layer->get_geographical_bbox().ymax + layer->get_geographical_bbox().ymin) / 2.) << "],\n";

    res << std::fixed << "  \"bounds\": [" << 
        layer->get_geographical_bbox().xmin << "," << 
        layer->get_geographical_bbox().ymin << "," << 
        layer->get_geographical_bbox().xmax << "," << 
        layer->get_geographical_bbox().ymax << 
    "],\n";

    res << "  \"format\": \"" << Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) << "\",\n";
    res << "  \"tiles\":[\"" << endpoint_uri << "/1.0.0/" << layer->get_id() << "/{z}/{x}/{y}." << Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) << "\"]";


    if (! Rok4Format::is_raster(layer->get_pyramid()->get_format())) {
        res << ",\n  \"vector_layers\": [\n";
        for (int i = 0; i < tablesNames.size(); i++) {
            if (i != 0) res << ",\n";
            res << "      " << tablesInfos.at(tablesNames.at(i))->get_metadata_json(maxs.at(tablesNames.at(i)),mins.at(tablesNames.at(i)));
        }
        res << "\n  ]";
    }
    res << "\n}\n";

    return new MessageDataStream ( res.str(),"application/json", 200 );
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

    std::ostringstream res;
    res << "<GDAL_WMS>\n";

    res << "  <Service name=\"TMS\">\n";
    res << "    <ServerUrl>" << endpoint_uri << "/1.0.0/" << layer->get_id() << "/${z}/${x}/${y}." << Rok4Format::to_extension ( ( layer->get_pyramid()->get_format() ) ) << "</ServerUrl>\n";
    res << "  </Service>\n";

    res << "  <DataWindow>\n";
    res << "    <UpperLeftX>" << Utils::double_to_string(best->get_tm()->get_x0()) << "</UpperLeftX>\n";
    res << "    <UpperLeftY>" << Utils::double_to_string(best->get_tm()->get_y0()) << "</UpperLeftY>\n";
    res << "    <LowerRightX>" << Utils::double_to_string(best->get_tm()->get_x0() + best->get_tm()->get_matrix_width() * best->get_tm()->get_tile_width() * best->get_tm()->get_res()) << "</LowerRightX>\n";
    res << "    <LowerRightY>" << Utils::double_to_string(best->get_tm()->get_y0() - best->get_tm()->get_matrix_height() * best->get_tm()->get_tile_height() * best->get_tm()->get_res()) << "</LowerRightY>\n";
    res << "    <TileLevel>" << best->get_tm()->getId() << "</TileLevel>\n";
    res << "    <TileCountX>1</TileCountX>\n";
    res << "    <TileCountY>1</TileCountY>\n";
    res << "    <YOrigin>top</YOrigin>\n";
    res << "  </DataWindow>\n";

    res << "  <Projection>" << layer->get_pyramid()->get_tms()->get_crs()->get_proj_code() << "</Projection>\n";
    res << "  <BlockSizeX>" << best->get_tm()->get_tile_width() << "</BlockSizeX>\n";
    res << "  <BlockSizeY>" << best->get_tm()->get_tile_height() << "</BlockSizeY>\n";
    res << "  <ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes>\n";
    res << "  <BandsCount>" << layer->get_pyramid()->get_channels() << "</BandsCount>\n";

    res << "</GDAL_WMS>\n";

    return new MessageDataStream ( res.str(), "application/xml", 200 );
}
