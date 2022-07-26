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

#include <regex>
#include <numeric>
#include <boost/algorithm/string.hpp>

#include "Rok4Server.h"
#include "utils/Utils.h"

void Rok4Server::buildOGCTILESCapabilities() {
    BOOST_LOG_TRIVIAL(warning) <<  "Not completly implemented !";

    // build collections...
    // schemas openapi tiles :
    // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/schemas/common-geodata/collections.yaml
    // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/schemas/common-geodata/collectionInfo.yaml
    // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/responses/common-geodata/rCollectionsList.yaml
    // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/responses/common-geodata/rCollection.yaml

    // ex. impl. Geoserver : 
    // https://vtp2.geo-solutions.it/geoserver/ogc/features?f=text%2Fhtml
    // https://vtp2.geo-solutions.it/geoserver/ogc/tiles?f=text%2Fhtml

    std::ostringstream res_coll;
    res_coll << "{\n";
    res_coll << "  \"links\" : [\n";
    // TODO [OGC] ex. de links ...
    // {
    //     href: "/ogcapitiles/collections?f=applicaiton/json",
    //     rel: "self",
    //     type: "applicaiton/json",
    //     title: "",
    //     templated: false
    // },
    // FIXME [OGC] format de sortie en html ou yaml ?
    res_coll << "  ],\n";
    res_coll << "  \"collections\" : [\n";
    // Layers
    auto layers = this->getLayerList();
    std::map<std::string, Layer *>::iterator itl = layers.begin();
    while(itl != layers.end()) {
        // Layer
        Layer* layer = itl->second;

        res_coll << "    {\n";

        std::ostringstream res_coll_id;
        res_coll_id << "{\n";
        res_coll_id << "  \"links\" : [\n";
        // TODO [OGC] ex. de links ...
        // {
        //     href: "/ogcapitiles/collections/{collectionId}/map/tiles?f=applicaiton/json",
        //     rel: "item",
        //     type: "applicaiton/json",
        //     title: "",
        //     templated: false
        // },
        res_coll_id << "  ],\n";
        res_coll_id << "  \"tilesets\" : [\n";
        res_coll_id << "    {\n";

        std::ostringstream res;
        res << "     \"id\": " << "\"" << layer->getId() << "\",\n";
        res << "     \"title\": \"" << layer->getTitle() << "\",\n";
        res << "     \"description\": \"" << layer->getAbstract() << "\",\n";
        res << "     \"extent\": {\n"; // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/schemas/common-geodata/extent.yaml
        res << "       \"spatial\": {\n";
        res << "         \"crs\": \"http://www.opengis.net/def/crs/OGC/1.3/CRS84\",\n";
        res << "         \"bbox\": [\n";
        res << "            [\n";
        res << "              " << layer->getGeographicBoundingBox().xmin << ",\n";
        res << "              " << layer->getGeographicBoundingBox().ymin << ",\n";
        res << "              " << layer->getGeographicBoundingBox().xmax << ",\n";
        res << "              " << layer->getGeographicBoundingBox().ymax <<  "\n";
        res << "            ]\n";
        res << "          ]\n";
        res << "        }\n";
        res << "      },\n";
        res << "     \"crs\": [],\n"; // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/schemas/common-geodata/crs.yaml
        // https://github.com/opengeospatial/ogcapi-maps/blob/master/openapi/schemas/common-geodata/dataType.yaml
        std::string dataType = "map"; // map / vector 
        if (! Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
            dataType = "vector";
        }
        res << "     \"dataType\": \"" << dataType << "\",\n";
        res << "     \"geometryDimension\": \"\",\n";
        res << "     \"minScaleDenominator\": \"" << layer->getMinRes() * 1000/0.28 << "\",\n";
        res << "     \"maxScaleDenominator\": \"" << layer->getMaxRes() * 1000/0.28 << "\",\n";
        res << "     \"tileMatrixSetURI\": \"/ogcapitiles/tileMatrixSets/"  << layer->getDataPyramid()->getTms()->getId() << "\",\n";
        res << "     \"links\":[]\n";
        // TODO [OGC] ex. de links :
        // {
        //     href: "/ogcapitiles/collections/{collectionId}/map/tiles/{tileMatrixSetId}/{tileMatrix}/{tileRow}/{tileCol}?f=image/png",
        //     rel: "item",
        //     type: "image/png",
        //     title: "get tile with style by default",
        //     templated: true
        // },
        // {
        //     href: "/ogcapitiles/map/tiles/{tileMatrixSetId}/{tileMatrix}/{tileRow}/{tileCol}?f=image/png&collections={collectionId}",
        //     rel: "item",
        //     type: "image/png",
        //     title: "get tile with style by default",
        //     templated: true
        // }

        res_coll_id << res.str();
        res_coll_id << "    }\n";
        res_coll_id << "  ]\n";
        res_coll_id << "}\n";
        ogctilesCapabilities.insert(std::pair<std::string, std::string>("collections::" + itl->first, res_coll_id.str()));

        res_coll << res.str();
        res_coll << "    }";
        if (++itl != layers.end()) {
            res_coll << ",";
        }
        res_coll << "\n";
    }
    res_coll << "  ]\n";
    res_coll << "}\n";
    ogctilesCapabilities.insert(std::pair<std::string, std::string>("collections", res_coll.str()));

    // build tms...
    // https://github.com/opengeospatial/ogcapi-maps/blob/master/openapi/schemas/tms/tileMatrixSet-item.yaml
    // https://github.com/opengeospatial/ogcapi-maps/blob/master/openapi/schemas/tms/tileMatrixSet.yaml
    // https://github.com/opengeospatial/ogcapi-tiles/blob/master/openapi/schemas/tms/tileMatrix.yaml
    std::ostringstream res_tms;
    res_tms << "{\n";
    res_tms << "  \"tilematrixsets\" : [\n";
    auto tms = this->getTmsList();
    std::map<std::string, TileMatrixSet *>::iterator itt = tms.begin();
    while(itt != tms.end()) {
        TileMatrixSet* otms = itt->second;
        // on construit un tileMatrixSet-item
        res_tms << "     {\n";
        res_tms << "       \"id\": \"" << otms->getId() << "\",\n";
        res_tms << "       \"title\": \"" << otms->getTitle() << "\",\n";
        res_tms << "       \"uri\": \"/ogcapitiles/tilematrixsets/" << otms->getId() << "\",\n";
        res_tms << "       \"crs\": \"" << otms->getCrs()->getRequestCode() << "\",\n";
        res_tms << "       \"links\": [\n";
        res_tms << "          {\n";
        res_tms << "            \"href\": \"/ogcapitiles/tilematrixsets/" << otms->getId() << "\",\n";
        res_tms << "            \"rel\": \"item\",\n";
        res_tms << "            \"type\": \"application/json\",\n";
        res_tms << "            \"title\": \"\",\n";
        res_tms << "            \"templated\": false\n";
        res_tms << "          }\n";
        res_tms << "        ]\n";

        // puis, on construit le tileMatrix
        std::ostringstream res_tms_id;
        res_tms_id << "{\n";
        res_tms_id << "  \"id\": \"" << otms->getId() << "\",\n";
        res_tms_id << "  \"title\": \"" << otms->getTitle() << "\",\n";
        res_tms_id << "  \"crs\": \"" << otms->getCrs()->getRequestCode() << "\",\n";
        res_tms_id << "  \"description\": \"" << otms->getAbstract() << "\",\n";
        auto k = otms->getKeyWords();
        std::string keyWords;
        for (size_t i = 0; i < k->size(); i++){
            keyWords += k->at(i).getContent();
            if (i != k->size()-1) {
                keyWords += ",";
            }
        }
        res_tms_id << "  \"keywords\":  [" << keyWords << "],\n";
        // TODO [OGC] global tms limits with crs native !
        // comment calculer la bbox du tms ?
        res_tms_id << "  \"boundingBox\": {\n";
        res_tms_id << "    \"lowerLeft\" : [],\n";
        res_tms_id << "    \"upperRight\" : [],\n";
        res_tms_id << "    \"crs\" : \"\",\n";
        res_tms_id << "    \"orderedAxes\" : []\n";
        res_tms_id << "   },\n";

        // FIXME [OGC] gestion des getTileLimits()- ?
        res_tms_id << "  \"tileMatrices\": [\n";
        auto tmOrderd = otms->getOrderedTileMatrix(false);
        auto ittm = tmOrderd.begin();
        while(ittm != tmOrderd.end()) {
            TileMatrix* otm = ittm->second;
            res_tms_id << "     {\n";
            res_tms_id << "      \"identifier\" : \"" << otm->getId() << "\",\n";
            res_tms_id << "      \"title\" : \"\",\n";
            res_tms_id << "      \"abstract\" : \"\",\n";
            res_tms_id << "      \"keywords\" : [],\n";
            double scaleDenominator = ( ( long double ) ( otm->getRes() * otms->getCrs()->getMetersPerUnit() ) /0.00028 );
            res_tms_id << "      \"scaleDenominator\" : " << scaleDenominator << ",\n";
            res_tms_id << "      \"cornerOfOrigin\" : \"topLeft\",\n";
            res_tms_id << "      \"pointOfOrigin\" : [\n"; // FIXME [OGC] la norme n'est pas claire "topLeftCorner" !?
            res_tms_id << "         " << otm->getX0() << ",\n";
            res_tms_id << "         " << otm->getY0()  << "\n";
            res_tms_id << "       ],\n";
            res_tms_id << "      \"tileWidth\" : " << otm->getTileW() << ",\n";
            res_tms_id << "      \"tileHeight\" : " << otm->getTileH() << ",\n";
            res_tms_id << "      \"matrixHeight\" : " << otm->getMatrixW() << ",\n";
            res_tms_id << "      \"matrixWidth\" : " << otm->getMatrixH() << "\n";
            res_tms_id << "     }";

            if (++ittm != tmOrderd.end()) {
                res_tms_id << ",";
            }
            res_tms_id << "\n";
        }

        res_tms_id << "   ]\n";
        res_tms_id << "}";
        ogctilesCapabilities.insert(std::pair<std::string, std::string>("tilematrixsets::" + itt->first, res_tms_id.str()));
        
        res_tms << "     }";
        if (++itt != tms.end()) {
            res_tms << ",";
        }
        res_tms << "\n";
    }
    res_tms << "  ]\n";
    res_tms << "}\n";
    ogctilesCapabilities.insert(std::pair<std::string, std::string>("tilematrixsets", res_tms.str()));
}

DataStream* Rok4Server::OGCTILESGetCapabilities ( Request* request ) {
    if (ogctilesCapabilities.size() == 0) {
        std::string message = "Les requetes getCapabilities OGC Tiles pre-buildées sont vides !?";
        BOOST_LOG_TRIVIAL(error) <<  message  ;
        return new SERDataStream ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, message, "ogctiles" ) );
    }

    // on determine la bonne collections en fonction du template demandé.
    std::string capabilities = "";
    if (request->tmpl == TemplateOGC::GETCAPABILITIESBYCOLLECTION) {
        // all-collections
        capabilities = ogctilesCapabilities.at("collections");
    }
    else if (request->tmpl == TemplateOGC::GETTILEMATRIXSET) {
        // all-tilematrixsets
        capabilities = ogctilesCapabilities.at("tilematrixsets");
    }
    else {
        // collections ou tilematrisets by id
        // on parse l'URL pour trouver l'ID (layer ou tms)
        const std::regex re(TemplateOGC::toString(request->tmpl));
        std::smatch m;
        
        std::string str_id; // layerId ou tmsId !
        if (std::regex_match(request->path, m, re)) {
            str_id = m[1].str();
        }

        if ( str_id.empty() ) {
            return new SERDataStream ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre ID absent.", "ogcapitiles" ) 
            );
        }

        // on prefixe les ID par le type de service
        // on evite les doublons possibles
        if (request->tmpl == TemplateOGC::GETCAPABILITIESRASTERBYCOLLECTION ||
            request->tmpl == TemplateOGC::GETCAPABILITIESVECTORBYCOLLECTION) {
            Layer* layer = serverConf->getLayer(str_id);
            if ( layer == NULL ) {
                return new SERDataStream ( 
                    new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "LAYER ID " + str_id + " inconnu.", "ogcapitiles" ) 
                );
            }
            // collections-id
            capabilities = ogctilesCapabilities.at("collections::" + str_id);
        }
        else if (request->tmpl == TemplateOGC::GETTILEMATRIXSETBYID) {
            TileMatrixSet* tms = serverConf->getTMS(str_id);
            if ( tms == NULL ) {
                return new SERDataStream ( 
                    new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "TMS ID " + str_id + " inconnu.", "ogcapitiles" ) 
                );
            }
            // tilematrixsets-id
            capabilities = ogctilesCapabilities.at("tilematrixsets::" + str_id);
        }
        else {
            std::string message = "Probleme dans la requete getCapabilities OGC Tiles";
            BOOST_LOG_TRIVIAL(error) <<  message  ;
            return new SERDataStream ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, message, "ogcapitiles" ) );
        }
    }
    

    if (capabilities.empty()) {
        std::string message = "La requete getCapabilities OGC Tiles est vide !?";
        BOOST_LOG_TRIVIAL(error) <<  message  ;
        return new SERDataStream ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, message, "ogctiles" ) );
    }

    return new MessageDataStream ( capabilities, "application/json" );
}

DataSource* Rok4Server::getTileParamOGCTILES ( Request* request, Layer*& layer, TileMatrixSet*& tms, TileMatrix*& tm, int& tileCol, int& tileRow, std::string& format, Style*& style) {
    const std::regex re(TemplateOGC::toString(request->tmpl));
    std::smatch m;
 
    if (std::regex_match(request->path, m, re)) {

        // Analyse generique du template :
        //  1.les 5 derniers groupement sont toujours : tms, tm, tileCol, tileRow, (info)
        //    le groupe (info) est toujours vide car c'est pour le getFeatureInfo
        //  2.les 2 autres groupes possibles :
        //      si recherche /collections/ -> layer
        //      sinon -> request->getParam("collections");
        //      et
        //      si recherche /styles/ -> style
        //      sinon style par defaut
        //  3.le format est toujours dans les param -> request->getParam("f");
        //  4.informer pour les parametres non gérés !

        int last = m.size() - 1;
        std::string str_tileRow = m[last - 1].str();
        std::string str_tileCol = m[last - 2].str();
        std::string str_tm      = m[last - 3].str();
        std::string str_tms     = m[last - 4].str();
        
        // LAYER (collections)
        std::string str_layer;
        if (request->tmpl == TemplateOGC::GETTILERASTERBYCOLLECTION || 
            request->tmpl == TemplateOGC::GETTILEVECTORBYCOLLECTION) {
            str_layer = m[1].str();
        } else if (request->tmpl == TemplateOGC::GETTILERASTERSTYLEDBYCOLLECTION) {
            str_layer = m[2].str();
        } else {
            str_layer = request->getParam("collections");
        }

        if ( str_layer.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre LAYER absent.", "ogcapitiles" ) 
            );
        }
    
        std::vector<std::string> layers;
        boost::split(layers, str_layer, boost::is_any_of(","));
        if (layers.size() > 1) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "Parametre LAYER multiple non géré.", "ogcapitiles" ) 
            );
        }

        if ( containForbiddenChars(str_layer)) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS layer: " << str_layer ;
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "Layer inconnu.", "ogcapitiles" ) 
            );
        }

        layer = serverConf->getLayer(str_layer);
        if ( layer == NULL || ! layer->getWMTSAuthorized() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "Layer " + str_layer + " inconnu.", "ogcapitiles" ) 
            );
        }

        // FORMAT
        std::string str_format = request->getParam("f");
        if ( str_format.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre FORMAT absent.", "ogcapitiles" ) 
            );
        }

        format = str_format;
        if ( containForbiddenChars(str_format) ) {
            // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS format: " << str_format ;
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "Le format n'est pas gere pour la couche " + str_layer, "ogcapitiles" ) 
            );
        }

        if ( str_format.compare ( Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) != 0 ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "Le format " + str_format + " n'est pas gere pour la couche " + str_layer, "ogcapitiles" ) 
            );
        }

        // STYLE
        std::string str_style;
        if (request->tmpl == TemplateOGC::GETTILERASTERSTYLED ||
            request->tmpl == TemplateOGC::GETTILERASTERSTYLEDBYCOLLECTION) {
               str_style = m[1].str();
        } else {
            str_style = "default"; // FIXME [OGC] style par defaut ?
        }

        if ( str_style.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre STYLE absent.", "ogcapitiles" ) 
            );
        }

        if (Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
            style = layer->getStyleByIdentifier(str_style);
            if ( ! ( style ) ) {
                return new SERDataSource ( 
                    new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "Le style " + str_style + " n'est pas gere pour la couche " + str_layer, "ogcapitiles" ) 
                );
            }
        }

        // TILEMATRIXSET
        if ( str_tms.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre TILEMATRIXSET absent.", "ogcapitiles" ) 
            );
        }

        if ( containForbiddenChars(str_tms)) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS tilematrixset: " << str_tms ;
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "TILEMATRIXSET inconnu.", "ogcapitiles" ) 
            );
        }

        tms = serverConf->getTMS(str_tms);
        if ( tms == NULL ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "TILEMATRIXSET " + str_tms + " inconnu.", "ogcapitiles" ) 
            );
        }

        if ( tms->getId() != layer->getDataPyramid()->getTms()->getId() && ! layer->isInWMTSTMSList(tms)) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "TILEMATRIXSET " + str_tms + " inconnu pour le layer.", "ogcapitiles" ) 
            );
        }
    
        // TILEMATRIX
        if ( str_tm.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre TILEMATRIX absent.", "ogcapitiles" )
            );
        }

        if ( containForbiddenChars(str_tm)) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS tilematrix: " << str_tm ;
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "TileMatrix inconnu pour le TileMatrixSet " + str_tms, "ogcapitiles" )
            );
        }

        tm = tms->getTm(str_tm);
        if ( tm == NULL ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "TileMatrix " + str_tm + " inconnu pour le TileMatrixSet " + str_tms, "ogcapitiles" )
            );
        }

        // TILEROW
        if ( str_tileRow.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre TILEROW absent.", "ogcapitiles" ) 
            );
        }
        
        if ( sscanf ( str_tileRow.c_str(),"%d",&tileRow ) != 1 ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "La valeur du parametre TILEROW est incorrecte.", "ogcapitiles" ) 
            );
        }

        // TILECOL
        if ( str_tileCol.empty() ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_MISSING_PARAMETER_VALUE, "Parametre TILECOL absent.", "ogcapitiles" ) 
            );
        }
        
        if ( sscanf ( str_tileCol.c_str(),"%d",&tileCol ) != 1 ) {
            return new SERDataSource ( 
                new ServiceException ( "", OWS_INVALID_PARAMETER_VALUE, "La valeur du parametre TILECOL est incorrecte.", "ogcapitiles" ) 
            );
        }

        if ( tms->getId() == layer->getDataPyramid()->getTms()->getId()) {
            // TMS natif de la pyramide, les tuiles limites sont stockées dans le niveau
            Level* level = layer->getDataPyramid()->getLevel(tm->getId());
            if (level == NULL) {
                // On est hors niveau -> erreur
                return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "ogcapitiles" ) );
            }

            if (! level->getTileLimits().containTile(tileCol, tileRow)) {
                // On est hors tuiles -> erreur
                return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "ogcapitiles" ) );
            }
        } else if (layer->isInWMTSTMSList(tms)) {
            // TMS supplémentaire, les tuiles limites sont stockées dans la couche
            TileMatrixLimits* tml = layer->getTmLimits(tms, tm);
            if (tml == NULL) {
                // On est hors niveau -> erreur
                return new SERDataSource ( 
                    new ServiceException ( "", HTTP_NOT_FOUND, "No data found", "ogcapitiles" ) 
                );
            }
            if (! tml->containTile(tileCol, tileRow)) {
                // On est hors tuiles -> erreur
                return new SERDataSource ( 
                    new ServiceException ( "", HTTP_NOT_FOUND, "No data found", "ogcapitiles" ) 
                );
            }
        }

        // OTHER PARAMS NOT YET IMPLEMENTED §
        std::string str_crs = request->getParam("crs");
        if ( !str_crs.empty() ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Parametre CRS non géré :" << str_crs;
        }
        std::string str_subset = request->getParam("subset");
        if ( !str_subset.empty() ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Parametre SUBSET non géré :" << str_subset;
        }
        std::string str_bgcolor = request->getParam("bgcolor");
        if ( !str_bgcolor.empty() ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Parametre BGCOLOR non géré :" << str_bgcolor;
        }
        std::string str_transparent = request->getParam("transparent");
        if ( !str_transparent.empty() ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Parametre TRANSPARENT non géré :" << str_transparent;
        }
        std::string str_subset_crs = request->getParam("subset-crs");
        if ( !str_subset_crs.empty() ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Parametre SUBSET-CRS non géré :" << str_subset_crs;
        }
        std::string str_datetime = request->getParam("datetime");
        if ( !str_datetime.empty() ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Parametre DATETIME non géré :" << str_datetime;
        }

        return NULL;
    }

    return new SERDataSource ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, "Not yet implemented !", "ogctiles" ) );
}