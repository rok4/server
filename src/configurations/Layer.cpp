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

#include <boost/format.hpp>
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
    if (style->get_aspect() != 0 || style->get_estompage() != 0 || style->get_pente() != 0) return false;

    return true;
}

bool Layer::parse(json11::Json& doc, ServicesConfiguration* services) {

    /********************** Default values */

    title = "";
    abstract = "";

    wms = true;
    wms_inspire = true;
    wmts = true;
    wmts_inspire = true;
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
    } else {
        wms_inspire = false;
        wmts_inspire = false;
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
    } else {
        wmts_inspire = false;
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
    std::vector<std::string> top_levels;
    std::vector<std::string> bottom_levels;

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
                    top_levels.push_back(pyr["top_level"].string_value());
                } else {
                    error_message =  "pyramids element have to own a 'top_level' string attribute";
                    return false;
                }

                if (pyr["bottom_level"].is_string()) {
                    bottom_levels.push_back(pyr["bottom_level"].string_value());
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
        if (! pyramid->add_levels(pyramids.at(i), bottom_levels.at(i), top_levels.at(i))) {
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

    // On a forcément le TMS natif de la donnée pour le WMTS
    WmtsTmsInfos infos;
    infos.tms = pyramid->get_tms();
    infos.top_level = pyramid->get_highest_level()->get_id();
    infos.bottom_level = pyramid->get_lowest_level()->get_id();
    infos.wmts_id = infos.tms->get_id() + "_" + infos.top_level + "_" + infos.bottom_level;
    wmts_tilematrixsets.push_back(infos);


    // On a forcément le CRS natif de la donnée pour le WMS
    wms_crss.push_back ( pyramid->get_tms()->get_crs() );

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

        // Pour être inspire, le style par défaut (le premier) doit avoir le bon identifiant
        if (styles[0]->get_identifier() != "inspire_common:DEFAULT") {
            wms_inspire = false;
            wmts_inspire = false;
        }

        // Configuration des reprojections possibles

        // TMS additionnels pour le WMTS
        if (services->get_wmts_service()->reprojection_enabled() && doc["wmts"].is_object() && doc["wmts"]["tms"].is_array()) {
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
        if (services->get_wms_service()->reprojection_enabled() && doc["wms"].is_object() && doc["wms"]["crs"].is_array()) {
            for (json11::Json c : doc["wms"]["crs"].array_items()) {
                if (! c.is_string()) {
                    error_message =  "wms.crs have to be a string array"  ;
                    return false;
                }
                std::string str_crs = c.string_value();
                // On verifie que la CRS figure dans la liste des CRS de proj (sinon, le serveur n est pas capable de la gerer)
                CRS* crs = CrsBook::get_crs( str_crs );
                
                if ( ! crs->is_define() ) {
                    BOOST_LOG_TRIVIAL(warning) << "CRS " << str_crs << " is not handled by PROJ";
                    continue;
                }

                if (services->get_wms_service()->is_available_crs(str_crs)) {
                    // Cette projection est déjà disponible globalement au niveau du service
                    continue;
                }

                // Test if the current layer bounding box is compatible with the current CRS
                if ( ! geographic_bbox.intersect_crs_area(crs) ) {
                    BOOST_LOG_TRIVIAL(warning) << "CRS " << str_crs << " is not compatible with the layer extent";
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
    std::set<std::pair<std::string, Level*>, ComparatorLevel> ordered_levels = pyramid->get_ordered_levels(true);

    Level * level = ordered_levels.begin()->second;
    native_bbox = BoundingBox<double>(level->get_bbox_from_tile_limits());
    native_bbox.crs = pyramid->get_tms()->get_crs()->get_request_code();

    geographic_bbox = BoundingBox<double>(native_bbox);
    geographic_bbox.reproject(pyramid->get_tms()->get_crs(), CRS::get_epsg4326());
}

void Layer::calculate_native_tilematrix_limits() {

    std::set<std::pair<std::string, Level*>, ComparatorLevel> ordered_levels = pyramid->get_ordered_levels(true);
    for (std::pair<std::string, Level*> element : ordered_levels) {
        element.second->set_tile_limits_from_bbox(native_bbox);
    }
}

void Layer::calculate_tilematrix_limits() {

    std::vector<WmtsTmsInfos> new_list;

    // Le premier TMS est celui natif : les tuiles limites sont déjà calculée et stockées dans les niveaux

    WmtsTmsInfos infos = wmts_tilematrixsets.at(0);
    infos.limits = std::vector<TileMatrixLimits>();

    std::set<std::pair<std::string, Level*>, ComparatorLevel> ordered_levels = pyramid->get_ordered_levels(false);
    for (std::pair<std::string, Level*> element : ordered_levels) {
        infos.limits.push_back(element.second->get_tile_limits());
    }

    new_list.push_back(infos);

    // On calcule les tuiles limites pour tous les TMS supplémentaires, que l'on filtre
    for (unsigned i = 1 ; i < wmts_tilematrixsets.size(); i++) {
        TileMatrixSet* tms = wmts_tilematrixsets.at(i).tms;

        BOOST_LOG_TRIVIAL(debug) <<  "Tile limits calculation for TMS " << tms->get_id() ;

        infos.tms = tms;
        infos.limits = std::vector<TileMatrixLimits>();

        BoundingBox<double> tmp = geographic_bbox;
        tmp.print();
        tmp = tmp.crop_to_crs_area(tms->get_crs());
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
        std::set<std::pair<std::string, Level*>, ComparatorLevel> ordered_levels = pyramid->get_ordered_levels(false);
        for (std::pair<std::string, Level*> element : ordered_levels) {
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
        ordered_levels = pyramid->get_ordered_levels(true);
        for (std::pair<std::string, Level*> element : ordered_levels) {
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

        new_list.push_back(infos);
    }

    // La nouvelle liste ne contient pas les TMS pour lesquels nous n'avons pas pu calculer les niveaux et les limites
    wmts_tilematrixsets = new_list;
}

Layer::Layer(std::string path, ServicesConfiguration* services) : Configuration(path), pyramid(NULL), attribution(NULL) {

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

    if (! parse(doc, services)) {
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


Layer::Layer(std::string layerName, std::string content, ServicesConfiguration* services ) : Configuration(), id(layerName), pyramid(NULL), attribution(NULL) {

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

    if (! parse(doc, services)) {
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

    if (attribution != NULL) delete attribution;
    if (pyramid != NULL) delete pyramid;
}


std::string Layer::get_abstract() { return abstract; }
bool Layer::is_wms_enabled() { return wms; }
bool Layer::is_wms_inspire() { return wms_inspire; }
bool Layer::is_tms_enabled() { return tms; }
bool Layer::is_wmts_enabled() { return wmts; }
bool Layer::is_wmts_inspire() { return wmts_inspire; }
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
std::vector<Style*>* Layer::get_styles() { return &styles; }

std::vector<WmtsTmsInfos>* Layer::get_available_tilematrixsets_wmts() { return &wmts_tilematrixsets; }
std::vector<CRS*>* Layer::get_available_crs_wms() { return &wms_crss; }
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
bool Layer::is_available_crs_wms(CRS* c) {
    for ( unsigned int k = 0; k < wms_crss.size(); k++ ) {
        if ( c->cmp_request_code ( wms_crss.at (k)->get_request_code() ) ) {
            return true;
        }
    }
    return false;
}
bool Layer::is_available_crs_wms(std::string c) {
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
std::vector<Metadata>* Layer::get_metadata() { return &metadata; }
bool Layer::is_gfi_enabled() { return gfi_enabled; }
std::string Layer::get_gfi_type() { return gfi_type; }
std::string Layer::get_gfi_url() { return gfi_url; }
std::map<std::string, std::string> Layer::get_gfi_extra_params() { return gfi_extra_params; }
std::string Layer::get_gfi_layers() { return gfi_layers; }
std::string Layer::get_gfi_query_layers() { return gfi_query_layers; }
Interpolation::KernelType Layer::get_resampling() { return resampling; }

void Layer::add_node_wmts(ptree& parent, WmtsService* service, bool only_inspire, std::map< std::string, WmtsTmsInfos>* used_tms_list) {

    if (! wmts) {
        return;
    }

    if (only_inspire && ! wmts_inspire) {
        return;
    }

    ptree& node = parent.add("Layer", "");
    node.add("ows:Title", title);
    node.add("ows:Abstract", abstract);

    if ( keywords.size() != 0 ) {
        ptree& keywords_node = node.add("ows:Keywords", "");
        for ( Keyword k : keywords) {
            k.add_node(keywords_node, "ows:Keyword");
        }
    }

    std::ostringstream os;
    os << geographic_bbox.xmin << " " << geographic_bbox.ymin;
    node.add("ows:WGS84BoundingBox.ows:LowerCorner", os.str());
    os.str ( "" );
    os << geographic_bbox.xmax << " " << geographic_bbox.ymax;
    node.add("ows:WGS84BoundingBox.ows:UpperCorner", os.str());
    
    node.add("ows:Identifier", id);
    
    for ( Metadata m : metadata) {
        m.add_node_wmts(node);
    }

    for ( Style* s : styles) {
        s->add_node_wmts(node);
    }

    node.add("Format", Rok4Format::to_mime_type ( pyramid->get_format() ));

    if (gfi_enabled){
        for ( unsigned int j = 0; j < service->get_available_infoformats()->size(); j++ ) {
            node.add("InfoFormat", service->get_available_infoformats()->at ( j ));
        }
    }

    // On ajoute les TMS disponibles avec les tuiles limites
    // Si la reprojection WMTS n'est pas activée, nous n'avons les limites que pour le TMS natif
    for (WmtsTmsInfos wti : wmts_tilematrixsets) {
        ptree& tms_node = node.add("TileMatrixSetLink", "");
        tms_node.add("TileMatrixSet", wti.wmts_id);

        ptree& tms_limits_node = tms_node.add("TileMatrixSetLimits", "");
        
        // Niveaux
        for ( unsigned int j = 0; j < wti.limits.size(); j++ ) { 
            wti.limits.at(j).add_node(tms_limits_node);
        }

        used_tms_list->insert ( std::pair<std::string, WmtsTmsInfos> ( wti.wmts_id , wti) );
    }

    // On veut ajouter le TMS natif des données dans sa version originale
    WmtsTmsInfos origin_infos;
    origin_infos.tms = pyramid->get_tms();
    origin_infos.top_level = "";
    origin_infos.bottom_level = "";
    origin_infos.wmts_id = origin_infos.tms->get_id();
    used_tms_list->insert ( std::pair<std::string, WmtsTmsInfos> ( origin_infos.wmts_id, origin_infos) );
}

void Layer::add_node_wms(ptree& parent, WmsService* service, bool only_inspire) {

    if (! wms) {
        return;
    }

    if (only_inspire && ! wms_inspire) {
        return;
    }

    ptree& node = parent.add("Layer", "");
    if (gfi_enabled) {
        node.add("<xmlattr>.queryable", "1");
    }

    node.add("Name", id);
    node.add("Title", title);
    node.add("Abstract", abstract);

    if ( keywords.size() != 0 ) {
        ptree& keywords_node = node.add("KeywordList", "");
        for ( Keyword k : keywords) {
            k.add_node(keywords_node, "Keyword");
        }
    }

    for ( CRS* c : wms_crss) {
        node.add("CRS", c->get_request_code());
    }

    geographic_bbox.add_node(node, true, true);

    // BoundingBox
    if ( only_inspire ) {
        for ( CRS* c : wms_crss) {
            BoundingBox<double> bbox ( 0,0,0,0 );
            if ( geographic_bbox.is_in_crs_area(c)) {
                bbox = geographic_bbox;
            } else {
                bbox = geographic_bbox.crop_to_crs_area(c);
            }

            bbox.reproject(CRS::get_epsg4326(), c);
            bbox.add_node(node, false, c->is_lat_lon() );
        }
        for ( unsigned int i = 0; i < service->get_available_crs()->size(); i++ ) {
            CRS* crs = service->get_available_crs()->at(i);
            BoundingBox<double> bbox ( 0,0,0,0 );
            if ( geographic_bbox.is_in_crs_area(crs)) {
                bbox = geographic_bbox;
            } else {
                bbox = geographic_bbox.crop_to_crs_area(crs);
            }

            bbox.reproject(CRS::get_epsg4326(), crs);
            bbox.add_node(node, false, crs->is_lat_lon() );
        }
    } else {
        BoundingBox<double> bbox = native_bbox;
        native_bbox.add_node(node, false, pyramid->get_tms()->get_crs()->is_lat_lon() );
    }

    if (attribution != NULL) {
        attribution->add_node_wms(node);
    }
    
    for ( Metadata m : metadata) {
        m.add_node_wms(node);
    }

    for ( Style* s : styles) {
        s->add_node_wms(node);
    }

    node.add("MinScaleDenominator", pyramid->get_lowest_level()->get_res() * 1000 / 0.28);
    node.add("MaxScaleDenominator", pyramid->get_highest_level()->get_res() * 1000 / 0.28);
}


void Layer::add_node_tms(ptree& parent, TmsService* service) {

    if (! tms) {
        return;
    }

    std::string ln = std::regex_replace(title, std::regex("\""), "&quot;");

    ptree& node = parent.add("TileMap", "");
    node.add("<xmlattr>.title", ln);
    node.add("<xmlattr>.srs", pyramid->get_tms()->get_crs()->get_request_code());
    node.add("<xmlattr>.profile", "none");
    node.add("<xmlattr>.extension", Rok4Format::to_extension ( pyramid->get_format() ));
    node.add("<xmlattr>.href", service->get_endpoint_uri() + "/1.0.0/" + id);
}

std::string Layer::get_description_tilejson(TmsService* service) {

    std::set<std::pair<std::string, Level*>, ComparatorLevel> ordered_levels = pyramid->get_ordered_levels(true);

    int order = 0;
    std::string minzoom, maxzoom;

    std::vector<std::string> tables_names;
    std::map<std::string, Table*> tables_infos;
    std::map<std::string, int> mins;
    std::map<std::string, int> maxs;

    for (std::pair<std::string, Level*> element : ordered_levels) {
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
        { "name", id },
        { "description", abstract },
        { "minzoom", std::stoi(minzoom) },
        { "maxzoom", std::stoi(maxzoom) },
        { "crs", pyramid->get_tms()->get_crs()->get_request_code() },
        { "center", json11::Json::array { 
            ((geographic_bbox.xmax + geographic_bbox.xmin) / 2.), 
            ((geographic_bbox.ymax + geographic_bbox.ymin) / 2.)
        } },
        { "bounds", json11::Json::array { 
            geographic_bbox.xmin,
            geographic_bbox.ymin,
            geographic_bbox.xmax,
            geographic_bbox.ymax 
        } },
        { "format", Rok4Format::to_extension ( ( pyramid->get_format() ) ) },
        { "tiles", json11::Json::array { service->get_endpoint_uri() + "/1.0.0/" + id + "/{z}/{x}/{y}." + Rok4Format::to_extension ( ( pyramid->get_format() ) ) } },
    };


    if (! Rok4Format::is_raster(pyramid->get_format())) {
        std::vector<json11::Json> items;
        for (int i = 0; i < tables_names.size(); i++) {
            items.push_back(tables_infos.at(tables_names.at(i))->to_json(maxs.at(tables_names.at(i)),mins.at(tables_names.at(i))));
        }
        res["vector_layers"] = items;
    }

    return json11::Json{ res }.dump();
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
std::string Layer::get_description_gdal(TmsService* service) {
    
    Level* best = pyramid->get_lowest_level();

    ptree tree;

    ptree& root = tree.add("GDAL_WMS", "");
    root.add("Service.<xmlattr>.name", "TMS");
    root.add("Service.ServerUrl", service->get_endpoint_uri() + "/1.0.0/" + id + "/${z}/${x}/${y}." + Rok4Format::to_extension ( ( pyramid->get_format() ) ));

    root.add("DataWindow.UpperLeftX", best->get_tm()->get_x0());
    root.add("DataWindow.UpperLeftY", best->get_tm()->get_y0());
    root.add("DataWindow.LowerRightX", best->get_tm()->get_x0() + best->get_tm()->get_matrix_width() * best->get_tm()->get_tile_width() * best->get_tm()->get_res());
    root.add("DataWindow.LowerRightY", best->get_tm()->get_y0() - best->get_tm()->get_matrix_height() * best->get_tm()->get_tile_height() * best->get_tm()->get_res());
    root.add("DataWindow.TileLevel", best->get_tm()->get_id());
    root.add("DataWindow.TileCountX", 1);
    root.add("DataWindow.TileCountY", 1);
    root.add("DataWindow.YOrigin", "top");

    root.add("Projection", pyramid->get_tms()->get_crs()->get_proj_code());
    root.add("BlockSizeX", best->get_tm()->get_tile_width());
    root.add("BlockSizeY", best->get_tm()->get_tile_height());
    root.add("ZeroBlockHttpCodes", "204,404");
    root.add("BandsCount", pyramid->get_channels());

    std::stringstream ss;
    write_xml(ss, tree);
    return ss.str();
}

std::string Layer::get_description_tms(TmsService* service) {
    
    ptree tree;

    ptree& root = tree.add("TileMap", "");
    root.add("<xmlattr>.version", "1.0.0");
    root.add("<xmlattr>.tilemapservice", service->get_endpoint_uri() + "/1.0.0" );
    root.add("Title", title );

    for ( Keyword k : keywords) {
        root.add("KeywordList", k.get_content() );
    }

    for ( Metadata m : metadata) {
        m.add_node_tms(root);
    }

    if (attribution != NULL) {
        attribution->add_node_tms(root);
    }

    CRS* crs = pyramid->get_tms()->get_crs();
    root.add("SRS", crs->get_request_code() );
    native_bbox.add_node(root, false, crs->is_lat_lon());

    TileMatrix* tm = pyramid->get_tms()->getTmList()->begin()->second;

    root.add("Origin.<xmlattr>.x", tm->get_x0() );
    root.add("Origin.<xmlattr>.y", tm->get_y0() );

    root.add("TileFormat.<xmlattr>.width", tm->get_tile_width() );
    root.add("TileFormat.<xmlattr>.height", tm->get_tile_height() );
    root.add("TileFormat.<xmlattr>.mime-type", Rok4Format::to_mime_type ( pyramid->get_format() ) );
    root.add("TileFormat.<xmlattr>.extension", Rok4Format::to_extension ( pyramid->get_format() ) );

    ptree& tilesets_node = root.add("TileMap", "");
    tilesets_node.add("<xmlattr>.profile", "none");

    int order = 0;

    std::set<std::pair<std::string, Level*>, ComparatorLevel> ordered_levels = pyramid->get_ordered_levels(false);

    for (std::pair<std::string, Level*> element : ordered_levels) {
        Level * level = element.second;
        tm = level->get_tm();
        ptree& tileset_node = tilesets_node.add("TileSet", "");

        tileset_node.add("<xmlattr>.href", str(boost::format("%s/1.0.0/%s/%s") % service->get_endpoint_uri() % id % tm->get_id()) );
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
    return ss.str();
}