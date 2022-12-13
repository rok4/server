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
 * \file Layer.cpp
 * \~french
 * \brief Implémentation de la classe Layer modélisant les couches de données.
 * \~english
 * \brief Implement the Layer Class handling data layer.
 */

#include "Layer.h"
#include "utils/Pyramid.h"
#include "utils/Cache.h"

#include <boost/log/trivial.hpp>
#include <libgen.h>
#include <fstream>
#include <string>
#include <iostream>

#include "utils/Utils.h"

bool Layer::parse(json11::Json& doc, ServicesConf* servicesConf) {

    bool inspire = servicesConf->isInspire();

    /********************** Default values */

    title="";
    abstract="";

    WMSAuthorized = true;
    WMTSAuthorized = true;
    TMSAuthorized = true;

    getFeatureInfoAvailability = false;
    getFeatureInfoType = "";
    getFeatureInfoBaseURL = "";
    GFIService = "";
    GFIVersion = "";
    GFIQueryLayers = "";
    GFILayers = "";
    GFIForceEPSG = true;

    // Chargement

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else {
        errorMessage = "title have to be provided and be a string";
        return false;
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else {
        errorMessage = "abstract have to be provided and be a string";
        return false;
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keyWords.push_back(Keyword ( kw.string_value()));
            } else {
                errorMessage = "keywords have to be a string array";
                return false;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        errorMessage = "keywords have to be a string array";
        return false;
    }

    if (doc["metadata"].is_array()) {
        for (json11::Json mtd : doc["metadata"].array_items()) {
            MetadataURL m = MetadataURL(mtd);
            if (m.getMissingField() != "") {
                errorMessage = "Invalid metadata: have to own a field " + m.getMissingField();
                return false;
            }
            metadataURLs.push_back (m);
        }
    } else if (! doc["metadata"].is_null()) {
        errorMessage = "metadata have to be an object array";
        return false;
    }

    if ( metadataURLs.size() == 0 && inspire ) {
        errorMessage =  "No MetadataURL found in the layer " + id + " : not compatible with INSPIRE!!"  ;
        return false;
    }

    if (doc["attribution"].is_object()) {
        attribution = new AttributionURL(doc["attribution"].object_items());
        if (attribution->getMissingField() != "") {
            errorMessage = "Invalid attribution: have to own a field " + attribution->getMissingField();
            return false;
        }
    }


    /******************* PYRAMIDES *********************/

    std::vector<Pyramid*> pyramids;
    std::vector<std::string> topLevels;
    std::vector<std::string> bottomLevels;

    if (doc["pyramids"].is_array()) {
        for (json11::Json pyr : doc["pyramids"].array_items()) {

            if (pyr.is_object()) {
                std::string pyr_path;
                if (pyr["path"].is_string()) {
                    pyr_path = pyr["path"].string_value();
                } else {
                    errorMessage =  "pyramids element have to own a 'path' string attribute";
                    return false;
                }

                if (pyr["top_level"].is_string()) {
                    topLevels.push_back(pyr["top_level"].string_value());
                } else {
                    errorMessage =  "pyramids element have to own a 'top_level' string attribute";
                    return false;
                }

                if (pyr["bottom_level"].is_string()) {
                    bottomLevels.push_back(pyr["bottom_level"].string_value());
                } else {
                    errorMessage =  "pyramids element have to own a 'bottom_level' string attribute";
                    return false;
                }

                Pyramid* p = new Pyramid( pyr_path );
                if ( ! p->isOk() ) {
                    BOOST_LOG_TRIVIAL(error) << p->getErrorMessage();
                    errorMessage =  "Pyramid " + pyr_path +" cannot be loaded"  ;
                    delete p;
                    return false;
                }

                pyramids.push_back(p);

            } else {
                errorMessage = "pyramids have to be an object array";
                return false;
            }
        }
    } else {
        errorMessage = "pyramids have to be provided and be an object array";
        return false;
    }

    if (pyramids.size() == 0) {
        errorMessage =  "No pyramid to broadcast"  ;
        return false;
    }

    dataPyramid = new Pyramid(pyramids.at(0));

    BOOST_LOG_TRIVIAL(debug) << "pyramide composée de " << pyramids.size() << " pyramide(s)";

    for (int i = 0; i < pyramids.size(); i++) {
        if (! dataPyramid->addLevels(pyramids.at(i), bottomLevels.at(i), topLevels.at(i))) {
            BOOST_LOG_TRIVIAL(error) << "Cannot compose pyramid to broadcast with input pyramid " << i;
            delete dataPyramid;
            errorMessage = "Pyramid to broadcast cannot be loaded";
            break;
        }
    }
    
    // Nettoyage des pyramides en entrée
    for (int i = 0; i < pyramids.size(); i++) {
        delete pyramids.at(i);
    }

    if ( ! dataPyramid ) {
        // Une erreur de chargement a été rencontrée lors de la composition
        return false;
    }

    /********************** Gestion de l'étendue des données */

    if (doc["bbox"].is_object()) {
        if (! doc["bbox"]["north"].is_number() || ! doc["bbox"]["south"].is_number() || ! doc["bbox"]["east"].is_number() || ! doc["bbox"]["west"].is_number()) {
            errorMessage = "Provided bbox is not valid";
            return false;
        }

        if (doc["bbox"]["north"].number_value() < doc["bbox"]["south"].number_value() || doc["bbox"]["east"].number_value() < doc["bbox"]["west"].number_value()) {
            errorMessage = "Provided bbox is not consistent";
            return false;
        }

        geographicBoundingBox = BoundingBox<double>(doc["bbox"]["west"].number_value(), doc["bbox"]["south"].number_value(), doc["bbox"]["east"].number_value(), doc["bbox"]["north"].number_value() );
        geographicBoundingBox.crs = "EPSG:4326";
        
        boundingBox = BoundingBox<double>(geographicBoundingBox);
        boundingBox.reproject(CRS::getEpsg4326(), dataPyramid->getTms()->getCrs());
        calculateNativeTileMatrixLimits();
    } else {
        /* Calcul de la bbox dans la projection des données, à partir des tuiles limites des niveaux de la pyramide */
        calculateBoundingBoxes();
    }


    // Services autorisés a priori
    if (doc["wms"].is_object() && doc["wms"]["authorized"].is_bool()) {
        WMSAuthorized = doc["wms"]["authorized"].bool_value();
    }
    if (doc["wmts"].is_object() && doc["wmts"]["authorized"].is_bool()) {
        WMTSAuthorized = doc["wmts"]["authorized"].bool_value();
    }
    if (doc["tms"].is_object() && doc["tms"]["authorized"].is_bool()) {
        TMSAuthorized = doc["tms"]["authorized"].bool_value();
    }

    if (Rok4Format::isRaster(dataPyramid->getFormat())) {
        /******************* CAS RASTER *********************/

        // Configuration du GET FEATURE INFO
        if (doc["get_feature_info"].is_object()) {
            getFeatureInfoAvailability = true;
            
            if (doc["get_feature_info"]["type"].is_string()) {

                if(doc["get_feature_info"]["type"].string_value().compare("PYRAMID") == 0) {
                    // Donnee elle-meme
                    getFeatureInfoType = "PYRAMID";
                }
                else if(doc["get_feature_info"]["type"].string_value().compare("EXTERNALWMS") == 0) {

                    getFeatureInfoType = "EXTERNALWMS";

                    if(doc["get_feature_info"]["url"].is_string()) {
                        getFeatureInfoBaseURL = doc["get_feature_info"]["url"].string_value();
                        std::string a = getFeatureInfoBaseURL.substr(getFeatureInfoBaseURL.length()-1, 1);
                        if ( a.compare("?") != 0 ) {
                            getFeatureInfoBaseURL = getFeatureInfoBaseURL + "?";
                        }
                    }
                    if(doc["get_feature_info"]["layers"].is_string()) {
                        GFILayers = doc["get_feature_info"]["layers"].string_value();
                    }
                    if(doc["get_feature_info"]["query_layers"].is_string()) {
                        GFIQueryLayers = doc["get_feature_info"]["query_layers"].string_value();
                    }
                    if(doc["get_feature_info"]["version"].is_string()) {
                        GFIVersion = doc["get_feature_info"]["version"].string_value();
                    }
                    if(doc["get_feature_info"]["service"].is_string()) {
                        GFIService = doc["get_feature_info"]["service"].string_value();
                    }
                    if(doc["get_feature_info"]["force_epsg"].is_bool()) {
                        GFIForceEPSG = doc["get_feature_info"]["force_epsg"].bool_value();
                    }

                } 
                else {
                    errorMessage = "get_feature_info.type unknown";
                    return false;
                }

            } else {
                errorMessage = "If get_feature_info is provided, get_feature_info.type is a required string";
                return false;
            }
        }

        // Configuration des styles
        std::string inspireStyleName = DEFAULT_STYLE_INSPIRE;
        if (doc["styles"].is_array()) {
            for (json11::Json st : doc["styles"].array_items()) {
                if (st.is_string()) {
                    std::string styleName = st.string_value();
                    
                    Style* sty = StyleBook::get_style(styleName);
                    if ( sty == NULL ) {
                        BOOST_LOG_TRIVIAL(warning) <<  "Style " << styleName <<" unknown or unloadable"  ;
                        continue;
                    }
                    if ( ! sty->isUsableForBroadcast() ) {
                        BOOST_LOG_TRIVIAL(warning) << "Style " << styleName << " not usable for broadcast" ;
                        continue;
                    }

                    if ( sty->getIdentifier().compare ( DEFAULT_STYLE_INSPIRE_ID ) == 0 ) {
                        // C'est le style inspire par défaut d'après son identifier
                        inspireStyleName = styleName;
                    }
                    
                    // Dans le cas inspire et du style inspire par défaut, on le rajoutera en premier après coup
                    if ( ! inspire || styleName != inspireStyleName) {
                        styles.push_back ( sty );
                    }

                } else {
                    errorMessage = "styles have to be an string array";
                    return false;
                }
            }
        } else if (! doc["styles"].is_null()) {
            errorMessage = "styles have to be an string array";
            return false;
        } else {
            std::string styleName = ( inspire ? DEFAULT_STYLE_INSPIRE : DEFAULT_STYLE );
            Style* sty = StyleBook::get_style(styleName);
            if ( sty == NULL ) {
                errorMessage =  "Default style unknown or unloadable"  ;
                return false;
            }
            if ( ! sty->isUsableForBroadcast() ) {
                errorMessage = "Default style " + styleName + " not usable for broadcast" ;
                return false;
            }

            if ( sty->getIdentifier().compare ( DEFAULT_STYLE_INSPIRE_ID ) == 0 ) {
                // C'est le style inspire par défaut d'après son identifier
                inspireStyleName = styleName;
            }
            
            // Dans le cas inspire et du style inspire par défaut, on le rajoutera en premier après coup
            if ( ! inspire || styleName != inspireStyleName) {
                styles.push_back ( sty );
            }
        }

        if ( inspire ) {

            Style* sty = StyleBook::get_style(inspireStyleName);
            if ( sty == NULL ) {
                errorMessage =  "Style inspire unknown or unloadable"  ;
                return false;
            }
            if ( ! sty->isUsableForBroadcast() ) {
                errorMessage = "Inspire style " + inspireStyleName + " not usable for broadcast" ;
                return false;
            }
            // On insère le style inspire en tête
            styles.insert ( styles.begin(), sty );
        }

        if ( styles.size() ==0 ) {
            errorMessage =  "No provided valid style, the layer is not valid"  ;
            return false;
        }

        // Configuration des reprojections possibles
        if ( servicesConf->getReprojectionCapability() ) {

            // TMS additionnels pour le WMTS
            if (doc["wmts"].is_object() && doc["wmts"]["tms"].is_array()) {
                for (json11::Json t : doc["wmts"]["tms"].array_items()) {
                    if (! t.is_string()) {
                        errorMessage =  "wmts.tms have to be a string array"  ;
                        return false;
                    }
                    TileMatrixSet* tms = TmsBook::get_tms(t.string_value());
                    if ( tms == NULL ) {
                        BOOST_LOG_TRIVIAL(warning) <<  "TMS " << t.string_value() <<" unknown"  ;
                        continue;
                    }
                    if (! servicesConf->isCRSAllowed(tms->getCrs()->getRequestCode())) {
                        BOOST_LOG_TRIVIAL(warning) <<  "Forbiden CRS for TMS " << t.string_value();
                        continue;
                    }
                    if (isInWMTSTMSList(tms) || dataPyramid->getTms()->getId() == tms->getId()) {
                        // Le TMS est déjà dans la liste ou est celui natif de la pyramide
                        continue;
                    }
                    WMTSTMSList.push_back ( tms );
                }
            }

            // Projections additionnelles pour le WMS
            if (doc["wms"].is_object() && doc["wms"]["crs"].is_array()) {
                for (json11::Json c : doc["wms"]["crs"].array_items()) {
                    if (! c.is_string()) {
                        errorMessage =  "wms.crs have to be a string array"  ;
                        return false;
                    }
                    std::string str_crs = c.string_value();
                    // On commence par regarder si la projection n'est pas déjà dans nos liste
                    if (servicesConf->isInGlobalCRSList(str_crs) || isInWMSCRSList(str_crs)) {
                        continue;
                    }
                    if (! servicesConf->isCRSAllowed(str_crs)) {
                        BOOST_LOG_TRIVIAL(warning) <<  "Forbiden CRS " << str_crs   ;
                        continue;
                    }
                    // On verifie que la CRS figure dans la liste des CRS de proj (sinon, le serveur n est pas capable de la gerer)
                    CRS* crs = new CRS ( str_crs );
                    
                    if ( ! crs->isDefine() ) {
                        BOOST_LOG_TRIVIAL(warning) << "CRS " << str_crs << " is not handled by PROJ";
                        delete crs;
                        continue;
                    }

                    // Test if the current layer bounding box is compatible with the current CRS
                    if ( ! geographicBoundingBox.intersectAreaOfCRS(crs) ) {
                        BOOST_LOG_TRIVIAL(warning) << "CRS " << str_crs << " is not compatible with the layer extent";
                        delete crs;
                        continue;
                    }

                    WMSCRSList.push_back ( crs );

                    // On voudra aussi ajouter les CRS équivalents si le service est configuré pour
                    if (servicesConf->getDoWeUseListOfEqualsCRS()) {
                        std::vector<CRS*> eqCRS = servicesConf->getEqualsCRS(str_crs );

                        for (unsigned int l = 0; l < eqCRS.size(); l++) {
                            if ( isInWMSCRSList(eqCRS.at( l )) ) {
                                continue;
                            }
                            // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
                            WMSCRSList.push_back( new CRS( eqCRS.at(l) ) );
                        }
                    }
                }
            }
        }

        if (doc["resampling"].is_string()) {
            resampling = Interpolation::fromString ( doc["resampling"].string_value() );
            if (resampling == Interpolation::KernelType::UNKNOWN) {
                errorMessage =  "Resampling " + doc["resampling"].string_value() +" unknown"  ;
                return false;
            }
        } else {
            resampling = Interpolation::fromString ( DEFAULT_RESAMPLING );
        }
    }

    return true;
}

void Layer::calculateBoundingBoxes() {

    // On calcule la bbox à partir des tuiles limites des niveaux de la pyramide
    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = dataPyramid->getOrderedLevels(true);
    bool first = true;
    for (std::pair<std::string, Level*> element : orderedLevels) {
        Level * level = element.second;

        if (first) {
            boundingBox = BoundingBox<double>(level->getBboxFromTileLimits());
            first = false;
            continue;
        }

        boundingBox = boundingBox.getUnion(level->getBboxFromTileLimits());
    }
    boundingBox.crs = dataPyramid->getTms()->getCrs()->getRequestCode();

    geographicBoundingBox = BoundingBox<double>(boundingBox);
    geographicBoundingBox.reproject(dataPyramid->getTms()->getCrs(), CRS::getEpsg4326());
}

void Layer::calculateNativeTileMatrixLimits() {

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = dataPyramid->getOrderedLevels(true);
    for (std::pair<std::string, Level*> element : orderedLevels) {
        element.second->setTileLimitsFromBbox(boundingBox);
    }
}

void Layer::calculateExtraTileMatrixLimits() {

    std::vector<TileMatrixSet *> newList;

    // On calcule les tuiles limites pour tous les TMS supplémentaires, que l'on filtre
    for (unsigned i = 0 ; i < WMTSTMSList.size(); i++) {
        TileMatrixSet* tms = WMTSTMSList.at(i);
        BOOST_LOG_TRIVIAL(debug) <<  "Tile limits calculation for TMS " << tms->getId() ;

        BoundingBox<double> tmp = geographicBoundingBox;
        tmp.print();
        tmp = tmp.cropToAreaOfCRS(tms->getCrs());
        BOOST_LOG_TRIVIAL(warning) <<  tms->getCrs()->getRequestCode();
        if ( tmp.hasNullArea()  ) {
            BOOST_LOG_TRIVIAL(warning) <<  "La couche n'est pas dans l'aire de définition du CRS du TMS supplémentaire " << tms->getId() ;
            continue;
        }
        if ( ! tmp.reproject ( CRS::getEpsg4326(), tms->getCrs() ) ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de reprojeter la bbox de la couche dans le CRS du TMS supplémentaire " << tms->getId() ;
            continue;
        }

        // Recherche du niveau du haut du TMS supplémentaire
        TileMatrix* tmTop = NULL;
        std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = dataPyramid->getOrderedLevels(false);
        for (std::pair<std::string, Level*> element : orderedLevels) {
            tmTop = tms->getCorrespondingTileMatrix(element.second->getTm(), dataPyramid->getTms()) ;
            if (tmTop != NULL) {
                // On a trouvé un niveau du haut du TMS supplémentaire satisfaisant pour la pyramide
                break;
            }
        }
        if ( tmTop == NULL ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de trouver un niveau du haut satisfaisant dans le TMS supplémentaire " << tms->getId() ;
            continue;
        }

        // Recherche du niveau du bas du TMS supplémentaire
        TileMatrix* tmBottom = NULL;
        orderedLevels = dataPyramid->getOrderedLevels(true);
        for (std::pair<std::string, Level*> element : orderedLevels) {
            tmBottom = tms->getCorrespondingTileMatrix(element.second->getTm(), dataPyramid->getTms()) ;
            if (tmBottom != NULL) {
                // On a trouvé un niveau du bas du TMS supplémentaire satisfaisant pour la pyramide
                break;
            }
        }
        if ( tmBottom == NULL ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de trouver un niveau du bas satisfaisant dans le TMS supplémentaire " << tms->getId() ;
            continue;
        }

        // Pour chaque niveau entre le haut et le base, on va calculer les tuiles limites

        std::vector<TileMatrixLimits> limits;
        std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> orderedTM = tms->getOrderedTileMatrix(false);
        bool begin = false;
        for (std::pair<std::string, TileMatrix*> element : orderedTM) {
            TileMatrix* tm = element.second;

            if (! begin && tm->getId() != tmTop->getId()) {
                continue;
            }
            begin = true;
            limits.push_back(tm->bboxToTileLimits(tmp));
            if (tm->getId() == tmBottom->getId()) {
                break;
            }
        }

        WMTSTMSLimitsList.push_back(limits);
        newList.push_back(tms);
    }

    // La nouvelle liste ne contient pas les TMS pour lesquels nous n'avons pas pu calculer les liveaux et les limites
    WMTSTMSList = newList;
}

Layer::Layer(std::string path, ServicesConf* servicesConf ) : Configuration(path), dataPyramid(NULL), attribution(NULL) {

    /********************** Id */

    id = Configuration::getFileName(filePath, ".json");

    if ( containForbiddenChars(id) ) {
        errorMessage =  "Layer identifier contains forbidden chars" ;
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "Add layer " << id << " from file or object";

    /********************** Read */

    ContextType::eContextType storage_type;
    std::string tray_name, fo_name;
    ContextType::split_path(path, storage_type, fo_name, tray_name);

    Context* context = StoragePool::get_context(storage_type, tray_name);
    if (context == NULL) {
        errorMessage = "Cannot add " + ContextType::toString(storage_type) + " storage context to read style";
        return;
    }

    int size = -1;
    uint8_t* data = context->readFull(size, fo_name);

    if (size < 0) {
        errorMessage = "Cannot read style "  + path ;
        if (data != NULL) delete[] data;
        return;
    }

    std::string err;
    json11::Json doc = json11::Json::parse ( std::string((char*) data, size), err );
    if ( doc.is_null() ) {
        errorMessage = "Cannot load JSON file "  + path + " : " + err ;
        return;
    }
    if (data != NULL) delete[] data;

    /********************** Parse */

    if (! parse(doc, servicesConf)) {
        return;
    }

    if (Rok4Format::isRaster(dataPyramid->getFormat())) {
        calculateExtraTileMatrixLimits();
    } else {
        // Une pyramide vecteur n'est diffusée qu'en WMTS et TMS et le GFI n'est pas possible
        WMSAuthorized = false;
        getFeatureInfoAvailability = false;
    }

    return;
}


Layer::Layer(std::string layerName, std::string content, ServicesConf* servicesConf ) : Configuration(), id(layerName), dataPyramid(NULL), attribution(NULL) {

    /********************** Id */

    if ( containForbiddenChars(id) ) {
        errorMessage =  "Layer identifier contains forbidden chars" ;
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "Add layer " << id << " from content";

    std::string err;
    json11::Json doc = json11::Json::parse ( content, err );
    if ( doc.is_null() ) {
        errorMessage = "Cannot load JSON content : " + err ;
        return;
    }

    /********************** Parse */

    if (! parse(doc, servicesConf)) {
        return;
    }

    if (Rok4Format::isRaster(dataPyramid->getFormat())) {
        calculateExtraTileMatrixLimits();
    } else {
        // Une pyramide vecteur n'est diffusée qu'en WMTS et TMS et le GFI n'est pas possible
        WMSAuthorized = false;
        getFeatureInfoAvailability = false;
    }
    
    return;
}


Image* Layer::getbbox (ServicesConf* servicesConf, BoundingBox<double> bbox, int width, int height, CRS* dst_crs, int dpi, int& error ) {
    error=0;

    bool crs_equals = servicesConf->are_the_two_CRS_equal( dataPyramid->getTms()->getCrs()->getProjCode(), dst_crs->getProjCode() );

    return dataPyramid->getbbox (servicesConf->getMaxTileX(), servicesConf->getMaxTileY(), bbox, width, height, dst_crs, crs_equals, resampling, dpi, error );
}

std::string Layer::getId() {
    return id;
}

Layer::~Layer() {
    for (unsigned int l = 0 ; l < WMSCRSList.size() ; l++) {
        delete WMSCRSList.at(l);
    }
    if (attribution != NULL) delete attribution;
    if (dataPyramid != NULL) delete dataPyramid;
}


std::string Layer::getAbstract() { return abstract; }
bool Layer::getWMSAuthorized() { return WMSAuthorized; }
bool Layer::getTMSAuthorized() { return TMSAuthorized; }
bool Layer::getWMTSAuthorized() { return WMTSAuthorized; }
std::vector<Keyword>* Layer::getKeyWords() { return &keyWords; }
double Layer::getMaxRes() { return dataPyramid->getHighestLevel()->getRes(); }
double Layer::getMinRes() { return dataPyramid->getLowestLevel()->getRes(); }
Pyramid* Layer::getDataPyramid() { return dataPyramid; }
Style* Layer::getDefaultStyle() {
    if (styles.size() > 0) {
        return styles[0];
    } else {
        return NULL;
    }
}
std::vector<Style*> Layer::getStyles() { return styles; }
std::vector<TileMatrixSet*> Layer::getWMTSTMSList() { return WMTSTMSList; }
std::vector<std::vector<TileMatrixLimits> > Layer::getWMTSTMSLimitsList() { return WMTSTMSLimitsList; }
bool Layer::isInWMTSTMSList(TileMatrixSet* tms) {
    for ( unsigned int k = 0; k < WMTSTMSList.size(); k++ ) {
        if ( tms->getId() == WMTSTMSList.at (k)->getId() ) {
            return true;
        }
    }
    return false;
}
TileMatrixLimits* Layer::getTmLimits(TileMatrixSet* tms, TileMatrix* tm) {
    for ( unsigned int k = 0; k < WMTSTMSList.size(); k++ ) {
        if ( tms->getId() == WMTSTMSList.at (k)->getId() ) {
            for ( unsigned int l = 0; l < WMTSTMSLimitsList.at(k).size(); l++ ) {
                if ( tm->getId() == WMTSTMSLimitsList.at(k).at(l).tileMatrixId ) {
                    return &(WMTSTMSLimitsList.at(k).at(l));
                }
            }
        }
    }
    return NULL;
}

Style* Layer::getStyle(std::string id) {
    for ( unsigned int i = 0; i < styles.size(); i++ ) {
        if ( id == styles[i]->getId() )
            return styles[i];
    }
    return NULL;
}

Style* Layer::getStyleByIdentifier(std::string identifier) {
    for ( unsigned int i = 0; i < styles.size(); i++ ) {
        if ( identifier == styles[i]->getIdentifier() )
            return styles[i];
    }
    return NULL;
}

std::string Layer::getTitle() { return title; }
std::vector<CRS*> Layer::getWMSCRSList() { return WMSCRSList; }
bool Layer::isInWMSCRSList(CRS* c) {
    for ( unsigned int k = 0; k < WMSCRSList.size(); k++ ) {
        if ( c->cmpRequestCode ( WMSCRSList.at (k)->getRequestCode() ) ) {
            return true;
        }
    }
    return false;
}
bool Layer::isInWMSCRSList(std::string c) {
    for ( unsigned int k = 0; k < WMSCRSList.size(); k++ ) {
        if ( WMSCRSList.at (k)->cmpRequestCode ( c ) ) {
            return true;
        }
    }
    return false;
}

AttributionURL* Layer::getAttribution() { return attribution; }
BoundingBox<double> Layer::getGeographicBoundingBox() { return geographicBoundingBox; }
BoundingBox<double> Layer::getBoundingBox() { return boundingBox; }
std::vector<MetadataURL> Layer::getMetadataURLs() { return metadataURLs; }
bool Layer::isGetFeatureInfoAvailable() { return getFeatureInfoAvailability; }
std::string Layer::getGFIType() { return getFeatureInfoType; }
std::string Layer::getGFIBaseUrl() { return getFeatureInfoBaseURL; }
std::string Layer::getGFILayers() { return GFILayers; }
std::string Layer::getGFIQueryLayers() { return GFIQueryLayers; }
std::string Layer::getGFIService() { return GFIService; }
std::string Layer::getGFIVersion() { return GFIVersion; }
bool Layer::getGFIForceEPSG() { return GFIForceEPSG; }
