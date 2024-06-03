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
 * \file services/common/Service.cpp
 ** \~french
 * \brief Implémentation de la classe CommonService
 ** \~english
 * \brief Implements classe CommonService
 */

#include <iostream>

#include <rok4/datasource/PaletteDataSource.h>

#include "services/tms/Exception.h"
#include "services/tms/Service.h"
#include "Rok4Server.h"

TmsService::TmsService (json11::Json& doc) : Service(doc) {

    if (! isOk()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        errorMessage = "TMS service: " + errorMessage;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else if (! doc["title"].is_null()) {
        errorMessage = "TMS service: title have to be a string";
        return;
    } else {
        title = "TMS service";
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else if (! doc["abstract"].is_null()) {
        errorMessage = "TMS service: abstract have to be a string";
        return;
    } else {
        abstract = "TMS service";
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                errorMessage = "TMS service: keywords have to be a string array";
                return;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        errorMessage = "TMS service: keywords have to be a string array";
        return;
    }

    if (doc["endpoint_uri"].is_string()) {
        endpoint_uri = doc["endpoint_uri"].string_value();
    } else if (! doc["endpoint_uri"].is_null()) {
        errorMessage = "TMS service: endpoint_uri have to be a string";
        return;
    } else {
        endpoint_uri = "http://localhost/tms";
    }

    if (doc["root_path"].is_string()) {
        root_path = doc["root_path"].string_value();
    } else if (! doc["root_path"].is_null()) {
        errorMessage = "TMS service: root_path have to be a string";
        return;
    } else {
        root_path = "/tms";
    }

    if (doc["metadata"].is_object()) {
        metadata = new MetadataURL ( doc["metadata"] );
        if (metadata->getMissingField() != "") {
            errorMessage = "TMS service: invalid metadata: have to own a field " + metadata->getMissingField();
            return ;
        }
    }
}

DataStream* TmsService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "HEALTH service";

    if ( match_route( "/([^/]+)/?", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETCAPABILITIES request";
        return get_capabilities(req, serv);
    }
    else if ( match_route( "/([^/]+)/([^/]+)/?", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETTILES request";
        return get_tiles(req, serv);
    }
    else if ( match_route( "/([^/]+)/([^/]+)/metadata\\.json", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETMETADATA request";
        return get_metadata(req, serv);
    }
    else if ( match_route( "/([^/]+)/([^/]+)/gdal\\.xml", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETGDAL request";
        return get_gdal(req, serv);
    }
    else if ( match_route( "/([^/]+)/([^/]+)/([^/]+)/([^/]+)/([^/]+)\\.(.*)", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETTILE request";
        return new DataStreamFromDataSource(get_tile(req, serv));
    } else {
        throw TmsException::get_error_message("Unknown tms request path", 400);
    }
};

DataStream* TmsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    if ( req->pathParams.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    std::string res = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res += "<TileMapService version=\"1.0.0\" services=\"" + endpoint_uri + "\">\n";
    res += "  <Title>" + title + "</Title>\n";
    res += "  <Abstract>" + abstract + "</Abstract>\n";

    if ( metadata ) {
        res += "  <Metadata type=\"" + metadata->getType() + "\" mime-type=\"text/xml\" href=\"" + metadata->getHRef() + "\" />\n";
    }

    res += "  <TileMaps>\n";

    std::map<std::string, Layer*>::iterator itLay ( serv->getServerConf()->get_layers().begin() ), itLayEnd ( serv->getServerConf()->get_layers().end() );
    for ( ; itLay != itLayEnd; ++itLay ) {
        Layer* lay = itLay->second;

        if (lay->is_tms_enabled()) {
            std::string ln = std::regex_replace(lay->getTitle(), std::regex("\""), "&quot;");
            res += "    <TileMap\n";
            res += "      title=\"" + ln + "\" \n";
            res += "      srs=\"" + lay->get_pyramid()->getTms()->getCrs()->getRequestCode() + "\" \n";
            res += "      profile=\"none\" \n";
            res += "      extension=\"" + Rok4Format::toExtension ( ( lay->get_pyramid()->getFormat() ) ) + "\" \n";
            res += "      href=\"" + endpoint_uri + "/1.0.0/" + lay->getId() + "\" />\n";
        }
    }

    res += "  </TileMaps>\n";
    res += "</TileMapService>\n";

    return new MessageDataStream ( res, "application/xml", 200 );
}

DataStream* TmsService::get_tiles ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->pathParams.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->pathParams.at(1);
    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->getServerConf()->getLayer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    std::ostringstream res;
    res << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res << "<TileMap version=\"1.0.0\" tilemapservice=\"" + endpoint_uri + "/1.0.0\">\n";
    res << "  <Title>" << layer->getTitle() << "</Title>\n";

    for ( unsigned int i=0; i < layer->getKeyWords()->size(); i++ ) {
        res << "  <KeywordList>" << layer->getKeyWords()->at(i).getContent() << "</KeywordList>\n";
    }

    for ( unsigned int i = 0; i < layer->getMetadataURLs().size(); i++ ) {
        res << "  " << layer->getMetadataURLs().at(i).getTmsXml();
    }

    if (layer->getAttribution() != NULL) {
        res << "  " << layer->getAttribution()->getTmsXml();
    }

    res << "  <SRS>" << layer->get_pyramid()->getTms()->getCrs()->getRequestCode() << "</SRS>\n";
    res << "  <BoundingBox minx=\"" <<
        layer->getBoundingBox().xmin << "\" miny=\"" <<
        layer->getBoundingBox().ymin << "\" maxx=\"" <<
        layer->getBoundingBox().xmax << "\" maxy=\"" <<
        layer->getBoundingBox().ymax << "\" />\n";

    TileMatrix* tm = layer->get_pyramid()->getTms()->getTmList()->begin()->second;

    res << "  <Origin x=\"" << tm->getX0() << "\" y=\"" << tm->getY0() << "\" />\n";

    res << "  <TileFormat width=\"" << tm->getTileW() << 
        "\" height=\"" << tm->getTileH() <<
        "\" mime-type=\"" << Rok4Format::toMimeType ( ( layer->get_pyramid()->getFormat() ) ) << 
        "\" extension=\"" << Rok4Format::toExtension ( ( layer->get_pyramid()->getFormat() ) ) << "\" />\n";

    res << "  <TileSets profile=\"none\">\n";

    int order = 0;

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->get_pyramid()->getOrderedLevels(false);

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;
        tm = level->getTm();
        res << "    <TileSet href=\"" << endpoint_uri << "/1.0.0/" << layer->getId() << "/" << tm->getId() << 
            "\" minrow=\"" << level->getMinTileRow() << "\" maxrow=\"" << level->getMaxTileRow() << 
            "\" mincol=\"" << level->getMinTileCol() << "\" maxcol=\"" << level->getMaxTileCol() <<
            "\" units-per-pixel=\"" << tm->getRes() << "\" order=\"" << order << "\" />\n";
        order++;
    }

    res << "  </TileSets>\n";
    res << "</TileMap>\n";

    return new MessageDataStream ( res.str(), "application/xml", 200 );
}

DataStream* TmsService::get_metadata ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->pathParams.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->pathParams.at(1);
    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->getServerConf()->getLayer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->get_pyramid()->getOrderedLevels(true);

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
            maxzoom = level->getId();
        }
        // Zooms min et max
        minzoom = level->getId();

        std::vector<Table>* levelTables = level->getTables();
        for (int i = 0; i < levelTables->size(); i++) {
            std::string t = levelTables->at(i).getName();
            std::map<std::string, std::string>::iterator it = mins.find ( t );
            if ( it == mins.end() ) {
                tablesNames.push_back(t);
                tablesInfos.insert ( std::pair<std::string, Table*> ( t, &(levelTables->at(i)) ) );
                maxs.insert ( std::pair<std::string, std::string> ( t, level->getId() ) );
                mins.insert ( std::pair<std::string, std::string> ( t, level->getId() ) );
            } else {
                it->second = minzoom;
            }

        }

        order++;
    }

    std::ostringstream res;
    std::string jsondesc;
    res << "{\n";
    res << "  \"name\": \"" << layer->getId() << "\",\n";
    res << "  \"description\": \"" << layer->getAbstract() << "\",\n";
    res << "  \"minzoom\": " << minzoom << ",\n";
    res << "  \"maxzoom\": " << maxzoom << ",\n";
    res << "  \"crs\": \"" << layer->get_pyramid()->getTms()->getCrs()->getRequestCode() << "\",\n";

    res << std::fixed << "  \"center\": [" <<
        ((layer->getGeographicBoundingBox().xmax + layer->getGeographicBoundingBox().xmin) / 2.) << "," << 
        ((layer->getGeographicBoundingBox().ymax + layer->getGeographicBoundingBox().ymin) / 2.) << "],\n";

    res << std::fixed << "  \"bounds\": [" << 
        layer->getGeographicBoundingBox().xmin << "," << 
        layer->getGeographicBoundingBox().ymin << "," << 
        layer->getGeographicBoundingBox().xmax << "," << 
        layer->getGeographicBoundingBox().ymax << 
    "],\n";

    res << "  \"format\": \"" << Rok4Format::toExtension ( ( layer->get_pyramid()->getFormat() ) ) << "\",\n";
    res << "  \"tiles\":[\"" << endpoint_uri << "/1.0.0/" << layer->getId() << "/{z}/{x}/{y}." << Rok4Format::toExtension ( ( layer->get_pyramid()->getFormat() ) ) << "\"]";


    if (! Rok4Format::isRaster(layer->get_pyramid()->getFormat())) {
        res << ",\n  \"vector_layers\": [\n";
        for (int i = 0; i < tablesNames.size(); i++) {
            if (i != 0) res << ",\n";
            res << "      " << tablesInfos.at(tablesNames.at(i))->getMetadataJson(maxs.at(tablesNames.at(i)),mins.at(tablesNames.at(i)));
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
    if ( req->pathParams.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->pathParams.at(1);
    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->getServerConf()->getLayer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    if (! Rok4Format::isRaster(layer->get_pyramid()->getFormat())) {
        throw TmsException::get_error_message("Layer " +str_layer+" is vector data: cannot describe it with this format", 400);
    }

    if (! layer->get_pyramid()->getTms()->getIsQTree()) {
        throw TmsException::get_error_message("Layer " +str_layer+" data is not a quad tree: cannot describe it with this format", 400);
    }

    Level* best = layer->get_pyramid()->getLowestLevel();

    std::ostringstream res;
    res << "<GDAL_WMS>\n";

    res << "  <Service name=\"TMS\">\n";
    res << "    <ServerUrl>" << endpoint_uri << "/1.0.0/" << layer->getId() << "/${z}/${x}/${y}." << Rok4Format::toExtension ( ( layer->get_pyramid()->getFormat() ) ) << "</ServerUrl>\n";
    res << "  </Service>\n";

    res << "  <DataWindow>\n";
    res << "    <UpperLeftX>" << UtilsXML::doubleToStr(best->getTm()->getX0()) << "</UpperLeftX>\n";
    res << "    <UpperLeftY>" << UtilsXML::doubleToStr(best->getTm()->getY0()) << "</UpperLeftY>\n";
    res << "    <LowerRightX>" << UtilsXML::doubleToStr(best->getTm()->getX0() + best->getTm()->getMatrixW() * best->getTm()->getTileW() * best->getTm()->getRes()) << "</LowerRightX>\n";
    res << "    <LowerRightY>" << UtilsXML::doubleToStr(best->getTm()->getY0() - best->getTm()->getMatrixH() * best->getTm()->getTileH() * best->getTm()->getRes()) << "</LowerRightY>\n";
    res << "    <TileLevel>" << best->getTm()->getId() << "</TileLevel>\n";
    res << "    <TileCountX>1</TileCountX>\n";
    res << "    <TileCountY>1</TileCountY>\n";
    res << "    <YOrigin>top</YOrigin>\n";
    res << "  </DataWindow>\n";

    res << "  <Projection>" << layer->get_pyramid()->getTms()->getCrs()->getProjCode() << "</Projection>\n";
    res << "  <BlockSizeX>" << best->getTm()->getTileW() << "</BlockSizeX>\n";
    res << "  <BlockSizeY>" << best->getTm()->getTileH() << "</BlockSizeY>\n";
    res << "  <ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes>\n";
    res << "  <BandsCount>" << layer->get_pyramid()->getChannels() << "</BandsCount>\n";

    res << "</GDAL_WMS>\n";

    return new MessageDataStream ( res.str(), "application/xml", 200 );
}

DataSource* TmsService::get_tile ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->pathParams.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->pathParams.at(1);
    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->getServerConf()->getLayer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    // Le niveau
    TileMatrixSet* tms = layer->get_pyramid()->getTms();

    std::string str_tm = req->pathParams.at(2);

    if ( containForbiddenChars(str_tm)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS tileMatrix: " << str_tm ;
        throw TmsException::get_error_message("Level unknown", 400);
    }

    TileMatrix* tm = tms->getTm(str_tm);
    if ( tm == NULL )
        throw TmsException::get_error_message("Level " + str_tm + " unknown", 400);


    // La colonne
    int column;
    std::string str_column = req->pathParams.at(3);
    if ( sscanf ( str_column.c_str(),"%d",&column ) != 1 )
        throw TmsException::get_error_message("Invalid column value", 400);

    // La ligne     
    int row;
    std::string str_row = req->pathParams.at(4);
    if ( sscanf ( str_row.c_str(),"%d",&row ) != 1 )
        throw TmsException::get_error_message("Invalid row value", 400);

    std::string extension = req->pathParams.at(5);

    // TMS natif de la pyramide, les tuiles limites sont stockées dans le niveau
    Level* level = layer->get_pyramid()->getLevel(tm->getId());
    if (level == NULL) {
        // On est hors niveau -> erreur
        throw TmsException::get_error_message("No data found", 404);
    }

    if (! level->getTileLimits().containTile(column, row)) {
        // On est hors tuiles -> erreur
        throw TmsException::get_error_message("No data found", 404);
    }

    // Le style
    Style* style;
    if (Rok4Format::isRaster(layer->get_pyramid()->getFormat())) {
        style = layer->getDefaultStyle();
    }

    // Le format : on vérifie la cohérence de l'extension avec le format des données

    if ( extension == "" )
        throw TmsException::get_error_message("Empty extension", 400);

    if ( containForbiddenChars(extension)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS extension: " << extension ;
        throw TmsException::get_error_message("Invalid extension", 400);
    }

    if ( extension.compare ( Rok4Format::toExtension ( ( layer->get_pyramid()->getFormat() ) ) ) != 0 ) {
        throw TmsException::get_error_message("Invalid extension " + extension, 400);
    }

    std::string format = Rok4Format::toMimeType ( ( layer->get_pyramid()->getFormat() ) );

    DataSource* d = level->getTile(column, row);
    if (d == NULL) {
        throw TmsException::get_error_message("No data found", 404);
    }

    // Avoid using unnecessary palette
    if (format == "image/png") {
        return new PaletteDataSource(d, style->getPalette());
    } else {
        return d;
    }
}