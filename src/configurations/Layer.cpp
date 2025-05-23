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
#include "Inspire.h"

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

    // On a forcément le TMS natif de la donnée pour le WMTS
    TileMatrixSetInfos* infos_tms_natif = new TileMatrixSetInfos(pyramid->get_tms());

    infos_tms_natif->set_bottom_top(pyramid->get_lowest_level()->get_id(), pyramid->get_highest_level()->get_id());

    for (Level* l : pyramid->get_ordered_levels(false)) {
        infos_tms_natif->limits.push_back(l->get_tile_limits());
    }

    available_tilematrixsets.push_back(infos_tms_natif);

    // On a forcément le CRS natif de la donnée pour le WMS
    available_crss.push_back ( pyramid->get_tms()->get_crs() );


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
    if (doc["tiles"].is_object() && doc["tiles"]["enabled"].is_bool()) {
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
                    } else {
                        error_message = "get_feature_info.url have to be provided and be a string for an EXTERNALWMS get feature info";
                        return false;
                    }
                    if(doc["get_feature_info"]["layers"].is_string()) {
                        gfi_layers = doc["get_feature_info"]["layers"].string_value();
                    } else {
                        error_message = "get_feature_info.layers have to be provided and be a string for an EXTERNALWMS get feature info";
                        return false;
                    }
                    if(doc["get_feature_info"]["query_layers"].is_string()) {
                        gfi_query_layers = doc["get_feature_info"]["query_layers"].string_value();
                    } else {
                        error_message = "get_feature_info.query_layers have to be provided and be a string for an EXTERNALWMS get feature info";
                        return false;
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
                    available_styles.push_back ( sty );
                } else {
                    error_message = "styles have to be a string array";
                    return false;
                }
            }
        } else if (! doc["styles"].is_null()) {
            error_message = "styles have to be a string array";
            return false;
        }

        if ( available_styles.size() == 0 ) {
            Style* sty = StyleBook::get_style(services->get_default_style_id());
            if ( sty == NULL ) {
                error_message =  "No valid style (even the default one), the layer is not valid"  ;
                return false;
            }
            if ( ! is_style_handled(sty) ) {
                error_message =  "No valid style (even the default one), the layer is not valid"  ;
                return false;
            }
            available_styles.push_back ( sty );
        }

        // Configuration des reprojections possibles

        // TMS additionnels (pour le WMTS et l'API Tiles)
        if (doc["extra_tilematrixsets"].is_array()) {
            for (json11::Json t : doc["extra_tilematrixsets"].array_items()) {
                if (! t.is_string()) {
                    error_message =  "extra_tilematrixsets have to be a string array"  ;
                    return false;
                }
                TileMatrixSet* tms = TmsBook::get_tms(t.string_value());
                if ( tms == NULL ) {
                    BOOST_LOG_TRIVIAL(warning) <<  "TMS " << t.string_value() <<" unknown"  ;
                    continue;
                }

                if (get_tilematrixset(tms->get_id()) != NULL) {
                    // Le TMS est déjà dans la liste
                    continue;
                }

                TileMatrixSetInfos* infos = new TileMatrixSetInfos(tms);
                // On calculera plus tard (dans calculate_tilematrix_limits) les informations de niveaux de haut et bas (et donc l'ID WMTS de ce TMS)
                available_tilematrixsets.push_back(infos);
            }
        }

        // Projections additionnelles (pour le WMS)
        if (doc["extra_crs"].is_array()) {
            for (json11::Json c : doc["extra_crs"].array_items()) {
                if (! c.is_string()) {
                    error_message =  "extra_crs have to be a string array"  ;
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

                available_crss.push_back ( crs );
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

    wms_inspire = Inspire::is_inspire_wms(this);
    wmts_inspire = Inspire::is_inspire_wmts(this);

    calculate_tilematrix_limits();

    if (! Rok4Format::is_raster(pyramid->get_format())) {
        // Une pyramide vecteur n'est diffusée qu'en TMS et API TILES et le GFI n'est pas possible
        wms = false;
        wms_inspire = false;
        wmts = false;
        wmts_inspire = false;
        gfi_enabled = false;
    }

    return true;
}

void Layer::calculate_bboxes() {

    // On calcule la bbox à partir des tuiles limites du niveau le mieux résolu de la pyramide
    Level * level = pyramid->get_lowest_level();
    native_bbox = BoundingBox<double>(level->get_bbox_from_tile_limits());
    native_bbox.crs = pyramid->get_tms()->get_crs()->get_request_code();

    geographic_bbox = BoundingBox<double>(native_bbox);
    geographic_bbox.reproject(pyramid->get_tms()->get_crs(), CRS::get_epsg4326());
}

void Layer::calculate_native_tilematrix_limits() {

    for (Level* l : pyramid->get_ordered_levels(true)) {
        l->set_tile_limits_from_bbox(native_bbox);
    }
}

void Layer::calculate_tilematrix_limits() {

    // Le premier TMS est celui natif : toutes les informations sont déjà présentes
    // On calcule les tuiles limites pour tous les TMS supplémentaires, que l'on filtre
    for (unsigned i = 1 ; i < available_tilematrixsets.size(); i++) {
        TileMatrixSetInfos* tmsi = available_tilematrixsets.at(i);

        BOOST_LOG_TRIVIAL(debug) <<  "Tile limits calculation for TMS " << tmsi->tms->get_id() ;

        tmsi->limits = std::vector<TileMatrixLimits>();

        BoundingBox<double> tmp = geographic_bbox;
        tmp.print();
        tmp = tmp.crop_to_crs_area(tmsi->tms->get_crs());
        if ( tmp.has_null_area()  ) {
            BOOST_LOG_TRIVIAL(warning) <<  "La couche n'est pas dans l'aire de définition du CRS du TMS supplémentaire " << tmsi->tms->get_id() ;
            available_tilematrixsets.erase (available_tilematrixsets.begin() + i);
            delete tmsi;
            i--;
            continue;
        }
        if ( ! tmp.reproject ( CRS::get_epsg4326(), tmsi->tms->get_crs() ) ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de reprojeter la bbox de la couche dans le CRS du TMS supplémentaire " << tmsi->tms->get_id() ;
            available_tilematrixsets.erase (available_tilematrixsets.begin() + i);
            delete tmsi;
            i--;
            continue;
        }

        // Recherche du niveau du haut du TMS supplémentaire
        TileMatrix* tm_top = NULL;
        for (Level* l : pyramid->get_ordered_levels(false)) {
            tm_top = tmsi->tms->get_corresponding_tm(l->get_tm(), pyramid->get_tms()) ;
            if (tm_top != NULL) {
                // On a trouvé un niveau du haut du TMS supplémentaire satisfaisant pour la pyramide
                break;
            }
        }
        if ( tm_top == NULL ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de trouver un niveau du haut satisfaisant dans le TMS supplémentaire " << tmsi->tms->get_id() ;
            available_tilematrixsets.erase (available_tilematrixsets.begin() + i);
            delete tmsi;
            i--;
            continue;
        }

        // Recherche du niveau du bas du TMS supplémentaire
        TileMatrix* tm_bottom = NULL;
        for (Level* l : pyramid->get_ordered_levels(true)) {
            tm_bottom = tmsi->tms->get_corresponding_tm(l->get_tm(), pyramid->get_tms()) ;
            if (tm_bottom != NULL) {
                // On a trouvé un niveau du bas du TMS supplémentaire satisfaisant pour la pyramide
                break;
            }
        }
        if ( tm_bottom == NULL ) {
            BOOST_LOG_TRIVIAL(warning) <<  "Impossible de trouver un niveau du bas satisfaisant dans le TMS supplémentaire " << tmsi->tms->get_id() ;
            available_tilematrixsets.erase (available_tilematrixsets.begin() + i);
            delete tmsi;
            i--;
            continue;
        }

        // Pour chaque niveau entre le haut et le base, on va calculer les tuiles limites

        bool begin = false;
        for (TileMatrix* tm : tmsi->tms->get_ordered_tm(false)) {

            if (! begin && tm->get_id() != tm_top->get_id()) {
                continue;
            }
            begin = true;
            tmsi->limits.push_back(tm->bbox_to_tile_limits(tmp));
            if (tm->get_id() == tm_bottom->get_id()) {
                break;
            }
        }

        tmsi->bottom_level = tm_bottom->get_id();
        tmsi->top_level = tm_top->get_id();
        tmsi->request_id = tmsi->tms->get_id() + "_" + tmsi->top_level + "_" + tmsi->bottom_level;
    }
}

Layer::Layer(std::string path, ServicesConfiguration* services) : Configuration(path), pyramid(NULL), attribution(NULL) {

    /********************** Id */

    id = Configuration::get_filename(file_path, ".json");

    if ( contain_chars(id, "<>\"") ) {
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

    return;
}


Layer::Layer(std::string layer_name, std::string content, ServicesConfiguration* services ) : Configuration(), id(layer_name), pyramid(NULL), attribution(NULL) {

    /********************** Id */

    if ( contain_chars(id, "<>\"") ) {
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
    
    return;
}

std::string Layer::get_id() {
    return id;
}

Layer::~Layer() {

    if (attribution != NULL) delete attribution;
    if (pyramid != NULL) delete pyramid;
    for (TileMatrixSetInfos* tmsi : available_tilematrixsets) {
        delete tmsi;
    }
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
    if (available_styles.size() > 0) {
        return available_styles[0];
    } else {
        return NULL;
    }
}
std::vector<Style*>* Layer::get_styles() { return &available_styles; }

TileMatrixSetInfos* Layer::get_tilematrixset(std::string request_id) {
    for ( unsigned int k = 0; k < available_tilematrixsets.size(); k++ ) {
        if ( request_id == available_tilematrixsets.at(k)->request_id || request_id == available_tilematrixsets.at(k)->tms->get_id() ) {
            return available_tilematrixsets.at(k);
        }
    }
    return NULL;
}
TileMatrixLimits* Layer::get_tilematrix_limits(TileMatrixSet* tms, TileMatrix* tm) {
    for ( unsigned int k = 0; k < available_tilematrixsets.size(); k++ ) {
        if ( tms->get_id() == available_tilematrixsets.at(k)->tms->get_id() ) {
            for ( unsigned int l = 0; l < available_tilematrixsets.at(k)->limits.size(); l++ ) {
                if ( tm->get_id() == available_tilematrixsets.at(k)->limits.at(l).tm_id ) {
                    return &(available_tilematrixsets.at(k)->limits.at(l));
                }
            }
        }
    }
    return NULL;
}

Style* Layer::get_style_by_identifier(std::string identifier) {
    for ( unsigned int i = 0; i < available_styles.size(); i++ ) {
        if ( identifier == available_styles[i]->get_identifier() )
            return available_styles[i];
    }
    return NULL;
}

std::string Layer::get_title() { return title; }
bool Layer::is_available_crs(CRS* c) {
    for ( unsigned int k = 0; k < available_crss.size(); k++ ) {
        if ( c->cmp_request_code ( available_crss.at (k)->get_request_code() ) ) {
            return true;
        }
    }
    return false;
}
bool Layer::is_available_crs(std::string c) {
    for ( unsigned int k = 0; k < available_crss.size(); k++ ) {
        if ( available_crss.at (k)->cmp_request_code ( c ) ) {
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

void Layer::add_node_wmts(ptree& parent, WmtsService* service, bool only_inspire, std::map< std::string, TileMatrixSetInfos*>* used_tms_list) {

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

    bool is_first = true;
    for ( Style* s : available_styles) {
        /*
        Un style est applicable en WMTS si :
           - le style est l'identité
        Ou :
           - le style est une palette (c'est forcément le cas, on n'accepte pas les styles plus complexes à la diffusion)
           - la donnée est sur un canal
           - la donnée est en PNG
        Cela permet de n'avoir à modifier que l'en tête de la tuile PNG pour que le style "soit appliqué"
        */
        if (s->is_identity() || (pyramid->get_channels() == 1 && pyramid->get_sample_compression() == Compression::PNG)) {
            s->add_node_wmts(node, is_first);
            is_first = false;
        }
    }

    node.add("Format", Rok4Format::to_mime_type ( pyramid->get_format() ));

    if (gfi_enabled){
        for ( unsigned int j = 0; j < service->get_available_infoformats()->size(); j++ ) {
            node.add("InfoFormat", service->get_available_infoformats()->at ( j ));
        }
    }

    // On ajoute les TMS disponibles avec les tuiles limites

    for (TileMatrixSetInfos* tmsi : available_tilematrixsets) {
        ptree& tms_node = node.add("TileMatrixSetLink", "");
        tms_node.add("TileMatrixSet", tmsi->request_id);

        ptree& tms_limits_node = tms_node.add("TileMatrixSetLimits", "");
        
        // Niveaux
        for ( unsigned int j = 0; j < tmsi->limits.size(); j++ ) { 
            tmsi->limits.at(j).add_node(tms_limits_node);
        }

        used_tms_list->emplace ( tmsi->request_id , tmsi );

        // Si la reprojection WMTS n'est pas activée, nous n'exposons que le premier TMS, celui natif
        if (! service->reprojection_enabled()) {
            break;
        }
    }

    // On veut ajouter le TMS natif des données dans sa version originale
    if (used_tms_list->find(pyramid->get_tms()->get_id()) == used_tms_list->end()) {
        used_tms_list->emplace ( pyramid->get_tms()->get_id(), new TileMatrixSetInfos(pyramid->get_tms()) );
    }
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

    for ( CRS* c : available_crss) {
        node.add("CRS", c->get_request_code());

        // Si la reprojection WMS n'est pas activée, nous n'exposons que le premier CRS, celui natif
        if (! service->reprojection_enabled()) {
            break;
        }
    }

    geographic_bbox.add_node(node, true, true);

    // BoundingBox
    if ( only_inspire ) {
        for ( CRS* c : available_crss) {
            BoundingBox<double> bbox ( 0,0,0,0 );
            if ( geographic_bbox.is_in_crs_area(c)) {
                bbox = geographic_bbox;
            } else {
                bbox = geographic_bbox.crop_to_crs_area(c);
            }

            bbox.reproject(CRS::get_epsg4326(), c);
            bbox.add_node(node, false, c->is_lat_lon() );

            // Si la reprojection WMS n'est pas activée, nous n'exposons que la bbox en projection native
            if (! service->reprojection_enabled()) {
                break;
            }
        }
        if (! service->reprojection_enabled()) {
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

    for ( Style* s : available_styles) {
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

json11::Json Layer::to_json_tiles(TilesService* service) {

    if (! tiles) {
        return json11::Json::NUL;
    }

    json11::Json::object res = json11::Json::object {
        { "id", id },
        { "title", title },
        { "description", abstract },
        { "extent", geographic_bbox.to_json_tiles() }
    };

    std::vector<json11::Json> links;

    links.push_back(json11::Json::object {
        { "href", service->get_endpoint_uri() + "/collections/" + id + "?f=json"},
        { "rel", "self"},
        { "type", "application/json"},
        { "title", "this document"}
    });
    
    for ( Metadata m : metadata) {
        links.push_back(m.to_json_tiles("Dataset metadata", "describedby"));
    }

    if (Rok4Format::is_raster(pyramid->get_format())) {
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/styles?f=json"},
            { "rel", "describedby"},
            { "type", "application/json"},
            { "title", "Styles list for " + id}
        });
    } else {
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/tiles?f=json"},
            { "rel", "describedby"},
            { "type", "application/json"},
            { "title", "Tilesets list for " + id}
        });
    }

    res["links"] = links;

    res["crs"] = json11::Json::array {
        pyramid->get_tms()->get_crs()->get_url()
    };

    if (Rok4Format::is_raster(pyramid->get_format())) {
        res["dataType"] = "map";
        res["dataTiles"] = false;
        res["mapTiles"] = true;
    } else {
        res["dataType"] = "vector";
        res["dataTiles"] = true;
        res["mapTiles"] = false;    
    }

    res["minCellSize"] = pyramid->get_lowest_level()->get_res();
    res["maxCellSize"] = pyramid->get_highest_level()->get_res();

    return res;
}


json11::Json Layer::to_json_styles(TilesService* service) {

    if (! tiles) {
        return json11::Json::NUL;
    }

    json11::Json::object res = json11::Json::object {
        { "id", id },
        { "title", title },
        { "links", json11::Json::array {
            json11::Json::object {
                { "href", service->get_endpoint_uri() + "/collections/" + id + "/styles?f=json"},
                { "rel", "self"},
                { "type", "application/json"},
                { "title", "this document"}
            }
        }}
    };

    std::vector<json11::Json> json_styles;
    for ( Style* s : available_styles) {
        json_styles.push_back(json11::Json::object {
            { "id", s->get_identifier()},
            { "title", s->get_titles().at(0)},
            { "links", json11::Json::array {
                json11::Json::object {
                    { "href", service->get_endpoint_uri() + "/collections/" + id + "/styles/" + s->get_identifier() + "/map/tiles?f=json"},
                    { "rel", "describedby"},
                    { "type", "application/json"},
                    { "title", "Tilesets list for " + id + " with style " + s->get_identifier()}
                }
            }}
        });
    }

    res["styles"] = json_styles;

    return res;
}

json11::Json Layer::to_json_tilesets(TilesService* service, Style* style) {

    json11::Json::object res = json11::Json::object {};

    std::vector<json11::Json> links;

    if (style == NULL) {
        // Interrogation de donnée vecteur
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/tiles?f=json"},
            { "rel", "self"},
            { "type", "application/json"},
            { "title", "this document"}
        });
    } else {
        // Interrogation de donnée raster
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/styles/" + style->get_identifier() + "/map/tiles?f=json"},
            { "rel", "self"},
            { "type", "application/json"},
            { "title", "this document"}
        });
    }

    res["links"] = links;

    std::vector<json11::Json> tilesets;

    for (TileMatrixSetInfos* t : available_tilematrixsets) {
        std::string data_type;
        std::string tileset_uri;

        if (style == NULL) {
            data_type = "vector";
            tileset_uri = service->get_endpoint_uri() + "/collections/" + id + "/tiles/" + t->tms->get_id() + "?f=json";
        } else {
            data_type = "map";
            tileset_uri = service->get_endpoint_uri() + "/collections/" + id + "/styles/" + style->get_identifier() + "/map/tiles/" + t->tms->get_id() + "?f=json";
        }

        tilesets.push_back(json11::Json::object {
            { "title", title },
            { "dataType", data_type},
            { "crs", t->tms->get_crs()->get_url()},
            { "tileMatrixSetURI", service->get_endpoint_uri() + "/tileMatrixSets/" + t->tms->get_id() + "?f=json"},
            { "links", json11::Json::array {
                json11::Json::object {
                    { "href", tileset_uri},
                    { "rel", "describedby"},
                    { "type", "application/json"},
                    { "title", "Tileset " + t->tms->get_id() + " for " + id}
                }
            }}
        });

        // Si la reprojection API Tiles n'est pas activée, nous n'exposons que le premier TMS, celui natif
        if (! service->reprojection_enabled()) {
            break;
        }
    }

    res["tilesets"] = tilesets;

    return res;
}

json11::Json Layer::to_json_tileset(TilesService* service, Style* style, TileMatrixSetInfos* tmsi) {

    json11::Json::object res = json11::Json::object {
        { "title", title},
        { "description", abstract},
        { "crs", tmsi->tms->get_crs()->get_url()},
        { "tileMatrixSetURI", service->get_endpoint_uri() + "/tileMatrixSets/" + tmsi->tms->get_id() + "?f=json"},
    };

    std::vector<json11::Json> links;

    if (style == NULL) {
        // Interrogation de donnée vecteur
        res["dataType"] = "vector";
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/tiles/" + tmsi->tms->get_id() + "?f=json"},
            { "rel", "self"},
            { "type", "application/json"},
            { "title", "this document"}
        });
        // Lien vers la tuile
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/tiles/" + tmsi->tms->get_id() + "/{tileMatrix}/{tileRow}/{tileCol}?f=" + Rok4Format::to_tiles_format(pyramid->get_format())},
            { "rel", "item"},
            { "type", Rok4Format::to_mime_type(pyramid->get_format())},
            { "title", id + " tiles as " + Rok4Format::to_mime_type(pyramid->get_format())},
            { "templated", true}
        });
    } else {
        // Interrogation de donnée raster
        res["dataType"] = "map";
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/styles/" + style->get_identifier() + "/map/tiles/" + tmsi->tms->get_id() + "?f=json"},
            { "rel", "self"},
            { "type", "application/json"},
            { "title", "this document"}
        });
        // Lien vers la tuile
        links.push_back(json11::Json::object {
            { "href", service->get_endpoint_uri() + "/collections/" + id + "/styles/" + style->get_identifier() + "/map/tiles/" + tmsi->tms->get_id() + "/{tileMatrix}/{tileRow}/{tileCol}?f=" + Rok4Format::to_tiles_format(pyramid->get_format())},
            { "rel", "item"},
            { "type", Rok4Format::to_mime_type(pyramid->get_format())},
            { "title", id + " tiles as " + Rok4Format::to_mime_type(pyramid->get_format())},
            { "templated", true}
        });
    }

    res["links"] = links;
    res["tileMatrixSetLimits"] = tmsi->limits;

    return res;
}

std::string Layer::get_description_tilejson(TmsService* service) {

    int order = 0;
    std::string minzoom, maxzoom;

    std::vector<std::string> tables_names;
    std::map<std::string, Table*> tables_infos;
    std::map<std::string, int> mins;
    std::map<std::string, int> maxs;

    for (Level* level : pyramid->get_ordered_levels(true)) {

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

    std::string description = abstract;
    // on double les backslash, en évitant de traiter les backslash déjà doublés
    boost::replace_all(description, "\\\\", "\\");
    boost::replace_all(description, "\\", "\\\\");
    // On échappe les doubles quotes
    boost::replace_all(description, "\"", "\\\"");

    json11::Json::object res = json11::Json::object {
        { "name", id },
        { "description", description },
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

    TileMatrix* tm = pyramid->get_lowest_level()->get_tm();

    root.add("Origin.<xmlattr>.x", tm->get_x0() );
    root.add("Origin.<xmlattr>.y", tm->get_y0() - tm->get_matrix_height() * tm->get_tile_height() * tm->get_res() );

    root.add("TileFormat.<xmlattr>.width", tm->get_tile_width() );
    root.add("TileFormat.<xmlattr>.height", tm->get_tile_height() );
    root.add("TileFormat.<xmlattr>.mime-type", Rok4Format::to_mime_type ( pyramid->get_format() ) );
    root.add("TileFormat.<xmlattr>.extension", Rok4Format::to_extension ( pyramid->get_format() ) );

    ptree& tilesets_node = root.add("TileSets", "");
    tilesets_node.add("<xmlattr>.profile", "none");

    int order = 0;

    for (Level* level : pyramid->get_ordered_levels(false)) {
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