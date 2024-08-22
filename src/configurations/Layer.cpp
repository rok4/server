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

#include <boost/log/trivial.hpp>
#include <libgen.h>
#include <fstream>
#include <string>
#include <iostream>

#include <rok4/utils/Pyramid.h>
#include <rok4/utils/Cache.h>
#include <rok4/utils/Utils.h>
#include <rok4/style/Style.h>

#include "configurations/Layer.h"

bool is_style_handled(Style* style) {
    if (style->get_identifier() == "") return false;
    if (style->get_titles().size() == 0) return false;
    if (style->get_aspect() != 0 || style->get_estompage() != 0 || style->get_pente() != 0) return false;

    return true;
}

bool Layer::parse(json11::Json& doc) {

    /********************** Default values */

    title = "";
    abstract = "";

    wms = true;
    wmts = true;
    tms = true;
    tiles = true;

    gfi_enabled = false;
    gfi_type = "";
    gfi_url = "";
    gfi_query_layers = "";
    gfi_layers = "";

    // Chargement

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else {
        error_message = "title have to be provided and be a string";
        return false;
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else {
        error_message = "abstract have to be provided and be a string";
        return false;
    }

    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keywords.push_back(Keyword ( kw.string_value()));
            } else {
                error_message = "keywords have to be a string array";
                return false;
            }
        }
    } else if (! doc["keywords"].is_null()) {
        error_message = "keywords have to be a string array";
        return false;
    }

    if (doc["metadata"].is_array()) {
        for (json11::Json mtd : doc["metadata"].array_items()) {
            Metadata m = Metadata(mtd);
            if (m.get_missing_field() != "") {
                error_message = "Invalid metadata: have to own a field " + m.get_missing_field();
                return false;
            }
            metadata.push_back (m);
        }
    } else if (! doc["metadata"].is_null()) {
        error_message = "metadata have to be an object array";
        return false;
    }

    if (doc["attribution"].is_object()) {
        attribution = new Attribution(doc["attribution"].object_items());
        if (attribution->get_missing_field() != "") {
            error_message = "Invalid attribution: have to own a field " + attribution->get_missing_field();
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
                    error_message =  "pyramids element have to own a 'path' string attribute";
                    return false;
                }

                if (pyr["top_level"].is_string()) {
                    topLevels.push_back(pyr["top_level"].string_value());
                } else {
                    error_message =  "pyramids element have to own a 'top_level' string attribute";
                    return false;
                }

                if (pyr["bottom_level"].is_string()) {
                    bottomLevels.push_back(pyr["bottom_level"].string_value());
                } else {
                    error_message =  "pyramids element have to own a 'bottom_level' string attribute";
                    return false;
                }

                Pyramid* p = new Pyramid( pyr_path );
                if ( ! p->is_ok() ) {
                    BOOST_LOG_TRIVIAL(error) << p->get_error_message();
                    error_message =  "Pyramid " + pyr_path +" cannot be loaded"  ;
                    delete p;
                    return false;
                }

                pyramids.push_back(p);

            } else {
                error_message = "pyramids have to be an object array";
                return false;
            }
        }
    } else {
        error_message = "pyramids have to be provided and be an object array";
        return false;
    }

    if (pyramids.size() == 0) {
        error_message =  "No pyramid to broadcast"  ;
        return false;
    }

    pyramid = new Pyramid(pyramids.at(0));

    BOOST_LOG_TRIVIAL(debug) << "pyramide composée de " << pyramids.size() << " pyramide(s)";

    for (int i = 0; i < pyramids.size(); i++) {
        if (! pyramid->add_levels(pyramids.at(i), bottomLevels.at(i), topLevels.at(i))) {
            BOOST_LOG_TRIVIAL(error) << "Cannot compose pyramid to broadcast with input pyramid " << i;
            delete pyramid;
            pyramid = NULL;
            error_message = "Pyramid to broadcast cannot be loaded";
            break;
        }
    }
    
    // Nettoyage des pyramides en entrée
    for (int i = 0; i < pyramids.size(); i++) {
        delete pyramids.at(i);
    }

    if ( ! pyramid ) {
        // Une erreur de chargement a été rencontrée lors de la composition
        return false;
    }

    WmtsTmsInfos infos;
    infos.tms = pyramid->get_tms();
    infos.top_level = pyramid->get_highest_level()->get_id();
    infos.bottom_level = pyramid->get_lowest_level()->get_id();
    infos.wmts_id = infos.tms->get_id() + "_" + infos.top_level + "_" + infos.bottom_level;
    wmts_tilematrixsets.push_back(infos);

    /********************** Gestion de l'étendue des données */

    if (doc["bbox"].is_object()) {
        if (! doc["bbox"]["north"].is_number() || ! doc["bbox"]["south"].is_number() || ! doc["bbox"]["east"].is_number() || ! doc["bbox"]["west"].is_number()) {
            error_message = "Provided bbox is not valid";
            return false;
        }

        if (doc["bbox"]["north"].number_value() < doc["bbox"]["south"].number_value() || doc["bbox"]["east"].number_value() < doc["bbox"]["west"].number_value()) {
            error_message = "Provided bbox is not consistent";
            return false;
        }

        geographic_bbox = BoundingBox<double>(doc["bbox"]["west"].number_value(), doc["bbox"]["south"].number_value(), doc["bbox"]["east"].number_value(), doc["bbox"]["north"].number_value() );
        geographic_bbox.crs = "EPSG:4326";
        
        native_bbox = BoundingBox<double>(geographic_bbox);
        native_bbox.reproject(CRS::get_epsg4326(), pyramid->get_tms()->get_crs());
        calculate_native_tilematrix_limits();
    } else {
        /* Calcul de la bbox dans la projection des données, à partir des tuiles limites des niveaux de la pyramide */
        calculate_bboxes();
    }


    // Services autorisés a priori
    if (doc["wms"].is_object() && doc["wms"]["enabled"].is_bool()) {
        wms = doc["wms"]["enabled"].bool_value();
    }
    if (doc["wmts"].is_object() && doc["wmts"]["enabled"].is_bool()) {
        wmts = doc["wmts"]["enabled"].bool_value();
    }
    if (doc["tms"].is_object() && doc["tms"]["enabled"].is_bool()) {
        tms = doc["tms"]["enabled"].bool_value();
    }
    if (doc["tiles"].is_object() && doc["tms"]["enabled"].is_bool()) {
        tiles = doc["tiles"]["enabled"].bool_value();
    }

    if (Rok4Format::is_raster(pyramid->get_format())) {
        /******************* CAS RASTER *********************/

        // Configuration du GET FEATURE INFO
        if (doc["get_feature_info"].is_object()) {
            gfi_enabled = true;
            
            if (doc["get_feature_info"]["type"].is_string()) {

                if(doc["get_feature_info"]["type"].string_value().compare("PYRAMID") == 0) {
                    // Donnee elle-meme
                    gfi_type = "PYRAMID";
                }
                else if(doc["get_feature_info"]["type"].string_value().compare("EXTERNALWMS") == 0) {

                    gfi_type = "EXTERNALWMS";

                    if(doc["get_feature_info"]["url"].is_string()) {
                        gfi_url = doc["get_feature_info"]["url"].string_value();
                    }
                    if(doc["get_feature_info"]["layers"].is_string()) {
                        gfi_layers = doc["get_feature_info"]["layers"].string_value();
                    }
                    if(doc["get_feature_info"]["query_layers"].is_string()) {
                        gfi_query_layers = doc["get_feature_info"]["query_layers"].string_value();
                    }
                    if(doc["get_feature_info"]["extra_query_params"].is_string()) {
                        gfi_extra_params = Utils::string_to_map(doc["get_feature_info"]["extra_query_params"].string_value(), "&", "=");
                    }

                } 
                else {
                    error_message = "get_feature_info.type unknown";
                    return false;
                }

            } else {
                error_message = "If get_feature_info is provided, get_feature_info.type is a required string";
                return false;
            }
        }

        // Configuration des styles
        if (doc["styles"].is_array()) {
            for (json11::Json st : doc["styles"].array_items()) {
                if (st.is_string()) {
                    std::string styleName = st.string_value();
                    
                    Style* sty = StyleBook::get_style(styleName);
                    if ( sty == NULL ) {
                        BOOST_LOG_TRIVIAL(warning) <<  "Style " << styleName <<" unknown or unloadable"  ;
                        continue;
                    }
                    if ( ! is_style_handled(sty) ) {
                        BOOST_LOG_TRIVIAL(warning) << "Style " << styleName << " not usable for broadcast" ;
                        continue;
                    }
                    styles.push_back ( sty );
                } else {
                    error_message = "styles have to be a string array";
                    return false;
                }
            }
        } else if (! doc["styles"].is_null()) {
            error_message = "styles have to be a string array";
            return false;
        }

        if ( styles.size() == 0 ) {
            error_message =  "No provided valid style, the layer is not valid"  ;
            return false;
        }

        // Configuration des reprojections possibles

        // TMS additionnels pour le WMTS
        if (doc["wmts"].is_object() && doc["wmts"]["tms"].is_array()) {
            for (json11::Json t : doc["wmts"]["tms"].array_items()) {
                if (! t.is_string()) {
                    error_message =  "wmts.tms have to be a string array"  ;
                    return false;
                }
                TileMatrixSet* tms = TmsBook::get_tms(t.string_value());
                if ( tms == NULL ) {
                    BOOST_LOG_TRIVIAL(warning) <<  "TMS " << t.string_value() <<" unknown"  ;
                    continue;
                }

                if (get_tilematrixset(tms->get_id()) != NULL) {
                    // Le TMS est déjà dans la liste ou est celui natif de la pyramide
                    continue;
                }

                infos.tms = tms;
                // On calculera plus tard (dans calculate_tilematrix_limits) les informations de niveaux de haut et bas (et donc l'ID WMTS de ce TMS)
                wmts_tilematrixsets.push_back(infos);
            }
        }

        // Projections additionnelles pour le WMS
        if (doc["wms"].is_object() && doc["wms"]["crs"].is_array()) {
            for (json11::Json c : doc["wms"]["crs"].array_items()) {
                if (! c.is_string()) {
                    error_message =  "wms.crs have to be a string array"  ;
                    return false;
                }
                std::string str_crs = c.string_value();
                // On verifie que la CRS figure dans la liste des CRS de proj (sinon, le serveur n est pas capable de la gerer)
                CRS* crs = new CRS ( str_crs );
                
                if ( ! crs->is_define() ) {
                    BOOST_LOG_TRIVIAL(warning) << "CRS " << str_crs << " is not handled by PROJ";
                    delete crs;
                    continue;
                }

                // Test if the current layer bounding box is compatible with the current CRS
                if ( ! geographic_bbox.intersect_crs_area(crs) ) {
                    BOOST_LOG_TRIVIAL(warning) << "CRS " << str_crs << " is not compatible with the layer extent";
                    delete crs;
                    continue;
                }

                wms_crss.push_back ( crs );
            }
        }

        if (doc["resampling"].is_string()) {
            resampling = Interpolation::from_string ( doc["resampling"].string_value() );
            if (resampling == Interpolation::KernelType::UNKNOWN) {
                error_message =  "Resampling " + doc["resampling"].string_value() +" unknown"  ;
                return false;
            }
        } else {
            resampling = Interpolation::from_string ( DEFAULT_RESAMPLING );
        }
    }

    return true;
}

void Layer::calculate_bboxes() {

    // On calcule la bbox à partir des tuiles limites du niveau le mieux résolu de la pyramide
    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = pyramid->get_ordered_levels(true);

    Level * level = orderedLevels.begin()->second;
    native_bbox = BoundingBox<double>(level->get_bbox_from_tile_limits());
    native_bbox.crs = pyramid->get_tms()->get_crs()->get_request_code();

    geographic_bbox = BoundingBox<double>(native_bbox);
    geographic_bbox.reproject(pyramid->get_tms()->get_crs(), CRS::get_epsg4326());
}

void Layer::calculate_native_tilematrix_limits() {

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = pyramid->get_ordered_levels(true);
    for (std::pair<std::string, Level*> element : orderedLevels) {
        element.second->set_tile_limits_from_bbox(native_bbox);
    }
}

void Layer::calculate_tilematrix_limits() {

    std::vector<WmtsTmsInfos> newList;

    // Le premier TMS est celui natif : les tuiles limites sont déjà calculée et stockées dans les niveaux

    WmtsTmsInfos infos = wmts_tilematrixsets.at(0);
    infos.limits = std::vector<TileMatrixLimits>();

    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = pyramid->get_ordered_levels(false);
    for (std::pair<std::string, Level*> element : orderedLevels) {
        infos.limits.push_back(element.second->get_tile_limits());
    }

    newList.push_back(infos);

    // On calcule les tuiles limites pour tous les TMS supplémentaires, que l'on filtre
    for (unsigned i = 1 ; i < wmts_tilematrixsets.size(); i++) {
        TileMatrixSet* tms = wmts_tilematrixsets.at(i).tms;

        BOOST_LOG_TRIVIAL(debug) <<  "Tile limits calculation for TMS " << tms->get_id() ;

        infos.tms = tms;
        infos.limits = std::vector<TileMatrixLimits>();

        BoundingBox<double> tmp = geographic_bbox;
        tmp.print();
        tmp = tmp.crop_to_crs_area(tms->get_crs());
        BOOST_LOG_TRIVIAL(warning) <<  tms->get_crs()->get_request_code();
        if ( tmp.has_null_area()  ) {
            BOOST_LOG_TRIVIAL(warning) <<  "La couche n'est pas dans l'aire de définition du CRS du TMS supplémentaire " << tms->get_id() ;
            continue;
        }
        if ( ! tmp.reproject ( CRS::get_epsg4326(), tms->get_crs() ) ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de reprojeter la bbox de la couche dans le CRS du TMS supplémentaire " << tms->get_id() ;
            continue;
        }

        // Recherche du niveau du haut du TMS supplémentaire
        TileMatrix* tmTop = NULL;
        std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = pyramid->get_ordered_levels(false);
        for (std::pair<std::string, Level*> element : orderedLevels) {
            tmTop = tms->get_corresponding_tm(element.second->get_tm(), pyramid->get_tms()) ;
            if (tmTop != NULL) {
                // On a trouvé un niveau du haut du TMS supplémentaire satisfaisant pour la pyramide
                break;
            }
        }
        if ( tmTop == NULL ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de trouver un niveau du haut satisfaisant dans le TMS supplémentaire " << tms->get_id() ;
            continue;
        }

        // Recherche du niveau du bas du TMS supplémentaire
        TileMatrix* tmBottom = NULL;
        orderedLevels = pyramid->get_ordered_levels(true);
        for (std::pair<std::string, Level*> element : orderedLevels) {
            tmBottom = tms->get_corresponding_tm(element.second->get_tm(), pyramid->get_tms()) ;
            if (tmBottom != NULL) {
                // On a trouvé un niveau du bas du TMS supplémentaire satisfaisant pour la pyramide
                break;
            }
        }
        if ( tmBottom == NULL ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de trouver un niveau du bas satisfaisant dans le TMS supplémentaire " << tms->get_id() ;
            continue;
        }

        // Pour chaque niveau entre le haut et le base, on va calculer les tuiles limites

        std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> orderedTM = tms->get_ordered_tm(false);
        bool begin = false;
        for (std::pair<std::string, TileMatrix*> element : orderedTM) {
            TileMatrix* tm = element.second;

            if (! begin && tm->get_id() != tmTop->get_id()) {
                continue;
            }
            begin = true;
            infos.limits.push_back(tm->bbox_to_tile_limits(tmp));
            if (tm->get_id() == tmBottom->get_id()) {
                break;
            }
        }

        infos.bottom_level = tmBottom->get_id();
        infos.top_level = tmTop->get_id();
        infos.wmts_id = infos.tms->get_id() + "_" + infos.top_level + "_" + infos.bottom_level;

        newList.push_back(infos);
    }

    // La nouvelle liste ne contient pas les TMS pour lesquels nous n'avons pas pu calculer les niveaux et les limites
    wmts_tilematrixsets = newList;
}

Layer::Layer(std::string path) : Configuration(path), pyramid(NULL), attribution(NULL) {

    /********************** Id */

    id = Configuration::get_filename(file_path, ".json");

    if ( contain_chars(id, "<>") ) {
        error_message =  "Layer identifier contains forbidden chars" ;
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "Add layer " << id << " from file or object";

    /********************** Read */

    ContextType::eContextType storage_type;
    std::string tray_name, fo_name;
    ContextType::split_path(path, storage_type, fo_name, tray_name);

    Context* context = StoragePool::get_context(storage_type, tray_name);
    if (context == NULL) {
        error_message = "Cannot add " + ContextType::to_string(storage_type) + " storage context to read style";
        return;
    }


    int size = -1;
    uint8_t* data = context->read_full(size, fo_name);

    if (size < 0) {
        error_message = "Cannot read style "  + path ;
        if (data != NULL) delete[] data;
        return;
    }

    std::string err;
    json11::Json doc = json11::Json::parse ( std::string((char*) data, size), err );
    if ( doc.is_null() ) {
        error_message = "Cannot load JSON file "  + path + " : " + err ;
        return;
    }
    if (data != NULL) delete[] data;

    /********************** Parse */

    if (! parse(doc)) {
        return;
    }

    if (Rok4Format::is_raster(pyramid->get_format())) {
        calculate_tilematrix_limits();
    } else {
        // Une pyramide vecteur n'est diffusée qu'en TMS et le GFI n'est pas possible
        wms = false;
        wmts = false;
        gfi_enabled = false;
    }

    return;
}


Layer::Layer(std::string layerName, std::string content ) : Configuration(), id(layerName), pyramid(NULL), attribution(NULL) {

    /********************** Id */

    if ( contain_chars(id, "<>") ) {
        error_message =  "Layer identifier contains forbidden chars" ;
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "Add layer " << id << " from content";

    std::string err;
    json11::Json doc = json11::Json::parse ( content, err );
    if ( doc.is_null() ) {
        error_message = "Cannot load JSON content : " + err ;
        return;
    }

    /********************** Parse */

    if (! parse(doc)) {
        return;
    }

    if (Rok4Format::is_raster(pyramid->get_format())) {
        calculate_tilematrix_limits();
    } else {
        // Une pyramide vecteur n'est diffusée qu'en WMTS et TMS et le GFI n'est pas possible
        wms = false;
        wmts = false;
        gfi_enabled = false;
    }
    
    return;
}

std::string Layer::get_id() {
    return id;
}

Layer::~Layer() {

    for (unsigned int l = 0 ; l < wms_crss.size() ; l++) {
        delete wms_crss.at(l);
    }
    if (attribution != NULL) delete attribution;
    if (pyramid != NULL) delete pyramid;
}


std::string Layer::get_abstract() { return abstract; }
bool Layer::is_wms_enabled() { return wms; }
bool Layer::is_tms_enabled() { return tms; }
bool Layer::is_wmts_enabled() { return wmts; }
bool Layer::is_tiles_enabled() { return tiles; }
std::vector<Keyword>* Layer::get_keywords() { return &keywords; }
Pyramid* Layer::get_pyramid() { return pyramid; }
Style* Layer::get_default_style() {
    if (styles.size() > 0) {
        return styles[0];
    } else {
        return NULL;
    }
}
std::vector<Style*> Layer::get_styles() { return styles; }

std::vector<WmtsTmsInfos> Layer::get_wmts_tilematrixsets() { return wmts_tilematrixsets; }
TileMatrixSet* Layer::get_tilematrixset(std::string wmts_id) {
    for ( unsigned int k = 0; k < wmts_tilematrixsets.size(); k++ ) {
        if ( wmts_id == wmts_tilematrixsets.at(k).wmts_id || wmts_id == wmts_tilematrixsets.at(k).tms->get_id() ) {
            return wmts_tilematrixsets.at(k).tms;
        }
    }
    return NULL;
}
TileMatrixLimits* Layer::get_tilematrix_limits(TileMatrixSet* tms, TileMatrix* tm) {
    for ( unsigned int k = 0; k < wmts_tilematrixsets.size(); k++ ) {
        if ( tms->get_id() == wmts_tilematrixsets.at (k).tms->get_id() ) {
            for ( unsigned int l = 0; l < wmts_tilematrixsets.at (k).limits.size(); l++ ) {
                if ( tm->get_id() == wmts_tilematrixsets.at (k).limits.at(l).tm_id ) {
                    return &(wmts_tilematrixsets.at (k).limits.at(l));
                }
            }
        }
    }
    return NULL;
}

Style* Layer::get_style_by_identifier(std::string identifier) {
    for ( unsigned int i = 0; i < styles.size(); i++ ) {
        if ( identifier == styles[i]->get_identifier() )
            return styles[i];
    }
    return NULL;
}

std::string Layer::get_title() { return title; }
bool Layer::is_wms_crs(CRS* c) {
    for ( unsigned int k = 0; k < wms_crss.size(); k++ ) {
        if ( c->cmp_request_code ( wms_crss.at (k)->get_request_code() ) ) {
            return true;
        }
    }
    return false;
}
bool Layer::is_wms_crs(std::string c) {
    for ( unsigned int k = 0; k < wms_crss.size(); k++ ) {
        if ( wms_crss.at (k)->cmp_request_code ( c ) ) {
            return true;
        }
    }
    return false;
}

Attribution* Layer::get_attribution() { return attribution; }
BoundingBox<double> Layer::get_geographical_bbox() { return geographic_bbox; }
BoundingBox<double> Layer::get_native_bbox() { return native_bbox; }
std::vector<Metadata> Layer::get_metadata() { return metadata; }
bool Layer::is_gfi_enabled() { return gfi_enabled; }
std::string Layer::get_gfi_type() { return gfi_type; }
std::string Layer::get_gfi_url() { return gfi_url; }
std::map<std::string, std::string> Layer::get_gfi_extra_params() { return gfi_extra_params; }
std::string Layer::get_gfi_layers() { return gfi_layers; }
std::string Layer::get_gfi_query_layers() { return gfi_query_layers; }
Interpolation::KernelType Layer::get_resampling() { return resampling; }
