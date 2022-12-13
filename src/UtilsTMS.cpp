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
 * \file CapabilitiesBuilder.cpp
 * \~french
 * \brief Implémentation des fonctions de générations des GetCapabilities
 * \~english
 * \brief Implement the GetCapabilities generation function
 */

#include "Rok4Server.h"
#include <iostream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cmath>
#include "utils/TileMatrixSet.h"
#include "utils/Pyramid.h"
#include "config.h"
#include "utils/Utils.h"

DataSource* Rok4Server::getTileParamTMS ( Request* request, Layer*& layer, TileMatrixSet*& tms, TileMatrix*& tm, int& tileCol, int& tileRow, std::string& format, Style*& style) {
    
    if ( request->pathParts.at(1) != "1.0.0" )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.0.0 disponible seulement))","tms" ) );

    // La couche
    std::string str_layer = request->pathParts.at(2);

    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer inconnu.","tms" ) );
    }

    layer = serverConf->getLayer(str_layer);

    if ( layer == NULL || ! layer->getTMSAuthorized() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer " +str_layer+" inconnu.","tms" ) );


    // Le niveau
    tms = layer->getDataPyramid()->getTms();

    std::string str_tm = request->pathParts.at(3);

    if ( containForbiddenChars(str_tm)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS tileMatrix: " << str_tm ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrix inconnu pour le TileMatrixSet " +tms->getId(),"wmts" ) );
    }

    tm = tms->getTm(str_tm);
    if ( tm == NULL )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrix " +str_tm+" inconnu pour le TileMatrixSet " +tms->getId(),"wmts" ) );


    // La colonne
    std::string str_TileCol = request->pathParts.at(4);
    if ( sscanf ( str_TileCol.c_str(),"%d",&tileCol ) != 1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILECOL est incorrecte.","tms" ) );

    // La ligne        
    std::string tileRowWithExtension = request->pathParts.at(5);
    std::string str_TileRow;
    std::string extension;
    char delim = '.';
    std::stringstream ss = std::stringstream(tileRowWithExtension);
    std::getline(ss, str_TileRow, delim);
    std::getline(ss, extension, delim);

    if ( sscanf ( str_TileRow.c_str(),"%d",&tileRow ) != 1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILEROW est incorrecte.","tms" ) );

    // TMS natif de la pyramide, les tuiles limites sont stockées dans le niveau
    Level* level = layer->getDataPyramid()->getLevel(tm->getId());
    if (level == NULL) {
        // On est hors niveau -> erreur
        return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "tms" ) );
    }

    if (! level->getTileLimits().containTile(tileCol, tileRow)) {
        // On est hors tuiles -> erreur
        return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "tms" ) );
    }

    // Le style
    if (Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
        style = layer->getDefaultStyle();
    }

    // Le format : on vérifie la cohérence de l'extension avec le format des données

    if ( extension == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Extension absente.","tms" ) );

    if ( containForbiddenChars(extension)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS extension: " << extension ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"L'extension n'est pas gere pour la couche " +str_layer,"tms" ) );
    }

    if ( extension.compare ( Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) ) != 0 ) {
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"L'extension " +extension+" n'est pas gere pour la couche " +str_layer,"tms" ) );
    }

    format = Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) );

    return NULL;
}


DataStream* Rok4Server::getLayerParamTMS ( Request* request, Layer*& layer ) {

    if ( request->pathParts.at(1) != "1.0.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.0.0 disponible seulement))","tms" ) );


    // La couche
    std::string str_layer = request->pathParts.at(2);

    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer inconnu.","tms" ) );
    }

    layer = serverConf->getLayer(str_layer);

    if ( layer == NULL || ! layer->getTMSAuthorized() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer " +str_layer+" inconnu.","tms" ) );
    }


    return NULL;
}

DataStream* Rok4Server::TMSGetLayer ( Request* request ) {

    Layer* layer;

    DataStream* errorResp = getLayerParamTMS(request, layer);

    if ( errorResp ) {
        return errorResp;
    }
    errorResp = NULL;
    std::ostringstream res;
    res << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    res << "<TileMap version=\"1.0.0\" tilemapservice=\"" + servicesConf->tmsPublicUrl + "/1.0.0\">\n";
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

    res << "  <SRS>" << layer->getDataPyramid()->getTms()->getCrs()->getRequestCode() << "</SRS>\n";
    res << "  <BoundingBox minx=\"" <<
        layer->getBoundingBox().xmin << "\" miny=\"" <<
        layer->getBoundingBox().ymin << "\" maxx=\"" <<
        layer->getBoundingBox().xmax << "\" maxy=\"" <<
        layer->getBoundingBox().ymax << "\" />\n";

    TileMatrix* tm = layer->getDataPyramid()->getTms()->getTmList()->begin()->second;

    res << "  <Origin x=\"" << tm->getX0() << "\" y=\"" << tm->getY0() << "\" />\n";

    res << "  <TileFormat width=\"" << tm->getTileW() << 
        "\" height=\"" << tm->getTileH() <<
        "\" mime-type=\"" << Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) << 
        "\" extension=\"" << Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) << "\" />\n";

    res << "  <TileSets profile=\"none\">\n";

    int order = 0;

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->getDataPyramid()->getOrderedLevels(false);

    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;
        tm = level->getTm();
        res << "    <TileSet href=\"" << servicesConf->tmsPublicUrl << "/1.0.0/" << layer->getId() << "/" << tm->getId() << 
            "\" minrow=\"" << level->getMinTileRow() << "\" maxrow=\"" << level->getMaxTileRow() << 
            "\" mincol=\"" << level->getMinTileCol() << "\" maxcol=\"" << level->getMaxTileCol() <<
            "\" units-per-pixel=\"" << tm->getRes() << "\" order=\"" << order << "\" />\n";
        order++;
    }

    res << "  </TileSets>\n";
    res << "</TileMap>\n";

    return new MessageDataStream ( res.str(),"application/xml" );
}

DataStream* Rok4Server::TMSGetLayerMetadata ( Request* request ) {

    Layer* layer;
    DataStream* errorResp = getLayerParamTMS(request, layer);

    if ( errorResp ) {
        return errorResp;
    }
    errorResp = NULL;


    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->getDataPyramid()->getOrderedLevels(true);

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
    res << "  \"crs\": \"" << layer->getDataPyramid()->getTms()->getCrs()->getRequestCode() << "\",\n";

    res << std::fixed << "  \"center\": [" <<
        ((layer->getGeographicBoundingBox().xmax + layer->getGeographicBoundingBox().xmin) / 2.) << "," << 
        ((layer->getGeographicBoundingBox().ymax + layer->getGeographicBoundingBox().ymin) / 2.) << "],\n";

    res << std::fixed << "  \"bounds\": [" << 
        layer->getGeographicBoundingBox().xmin << "," << 
        layer->getGeographicBoundingBox().ymin << "," << 
        layer->getGeographicBoundingBox().xmax << "," << 
        layer->getGeographicBoundingBox().ymax << 
    "],\n";

    res << "  \"format\": \"" << Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) << "\",\n";
    res << "  \"tiles\":[\"" << servicesConf->tmsPublicUrl << "/1.0.0/" << layer->getId() << "/{z}/{x}/{y}." << Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) << "\"]";


    if (! Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
        res << ",\n  \"vector_layers\": [\n";
        for (int i = 0; i < tablesNames.size(); i++) {
            if (i != 0) res << ",\n";
            res << "      " << tablesInfos.at(tablesNames.at(i))->getMetadataJson(maxs.at(tablesNames.at(i)),mins.at(tablesNames.at(i)));
        }
        res << "\n  ]";
    }
    res << "\n}\n";

    return new MessageDataStream ( res.str(),"application/json" );
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
DataStream* Rok4Server::TMSGetLayerGDAL ( Request* request ) {

    Layer* layer;
    DataStream* errorResp = getLayerParamTMS(request, layer);

    if ( errorResp ) {
        return errorResp;
    }
    errorResp = NULL;

    if (! Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE, "Layer " +layer->getId()+ " non interrogeable (donnee vecteur).","tms" ) );   
    }

    if (! layer->getDataPyramid()->getTms()->getIsQTree()) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE, "Layer " +layer->getId()+ " non interrogeable (Tile Matrix Set non QTree).","tms" ) );   
    }

    Level* best = layer->getDataPyramid()->getLowestLevel();

    std::ostringstream res;
    res << "<GDAL_WMS>\n";

    res << "  <Service name=\"TMS\">\n";
    res << "    <ServerUrl>" << servicesConf->tmsPublicUrl << "/1.0.0/" << layer->getId() << "/${z}/${x}/${y}." << Rok4Format::toExtension ( ( layer->getDataPyramid()->getFormat() ) ) << "</ServerUrl>\n";
    res << "  </Service>\n";

    res << "  <DataWindow>\n";
    res << "    <UpperLeftX>" << doubleToStr(best->getTm()->getX0()) << "</UpperLeftX>\n";
    res << "    <UpperLeftY>" << doubleToStr(best->getTm()->getY0()) << "</UpperLeftY>\n";
    res << "    <LowerRightX>" << doubleToStr(best->getTm()->getX0() + best->getTm()->getMatrixW() * best->getTm()->getTileW() * best->getTm()->getRes()) << "</LowerRightX>\n";
    res << "    <LowerRightY>" << doubleToStr(best->getTm()->getY0() - best->getTm()->getMatrixH() * best->getTm()->getTileH() * best->getTm()->getRes()) << "</LowerRightY>\n";
    res << "    <TileLevel>" << best->getTm()->getId() << "</TileLevel>\n";
    res << "    <TileCountX>1</TileCountX>\n";
    res << "    <TileCountY>1</TileCountY>\n";
    res << "    <YOrigin>top</YOrigin>\n";
    res << "  </DataWindow>\n";

    res << "  <Projection>" << layer->getDataPyramid()->getTms()->getCrs()->getProjCode() << "</Projection>\n";
    res << "  <BlockSizeX>" << best->getTm()->getTileW() << "</BlockSizeX>\n";
    res << "  <BlockSizeY>" << best->getTm()->getTileH() << "</BlockSizeY>\n";
    res << "  <ZeroBlockHttpCodes>204,404</ZeroBlockHttpCodes>\n";
    res << "  <BandsCount>" << layer->getDataPyramid()->getChannels() << "</BandsCount>\n";

    res << "</GDAL_WMS>\n";

    return new MessageDataStream ( res.str(),"application/xml" );
}

DataStream* Rok4Server::TMSGetCapabilities ( Request* request ) {
    
    if ( request->pathParts.at(1) != "1.0.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.0.0 disponible seulement))","tms" ) );

    return new MessageDataStream ( tmsCapabilities, "application/xml" );
}

void Rok4Server::buildTMSCapabilities() {

    tmsCapabilities = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    tmsCapabilities += "<TileMapService version=\"1.0.0\" services=\"" + servicesConf->tmsPublicUrl + "\">\n";
    tmsCapabilities += "  <Title>" + servicesConf->title + "</Title>\n";
    tmsCapabilities += "  <Abstract>" + servicesConf->abstract + "</Abstract>\n";
    tmsCapabilities += "  <TileMaps>\n";

    std::map<std::string, Layer*>::iterator itLay ( serverConf->layersList.begin() ), itLayEnd ( serverConf->layersList.end() );
    for ( ; itLay != itLayEnd; ++itLay ) {
        Layer* lay = itLay->second;

        if (lay->getTMSAuthorized()) {
            std::string ln = std::regex_replace(lay->getTitle(), std::regex("\""), "&quot;");
            tmsCapabilities += "    <TileMap\n";
            tmsCapabilities += "      title=\"" + ln + "\" \n";
            tmsCapabilities += "      srs=\"" + lay->getDataPyramid()->getTms()->getCrs()->getRequestCode() + "\" \n";
            tmsCapabilities += "      profile=\"none\" \n";
            tmsCapabilities += "      extension=\"" + Rok4Format::toExtension ( ( lay->getDataPyramid()->getFormat() ) ) + "\" \n";
            tmsCapabilities += "      href=\"" + servicesConf->tmsPublicUrl + "/1.0.0/" + lay->getId() + "\" />\n";
        }
    }

    tmsCapabilities += "  </TileMaps>\n";
    tmsCapabilities += "</TileMapService>\n";
}
