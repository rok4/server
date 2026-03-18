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

#include <fstream>

#include "configurations/Services.h"

bool ServicesConfiguration::parse(json11::Json& doc) {

    /********************** Default values */

    service_provider="";
    fee="";
    access_constraint="";
    provider_site="";
    default_style="normal";
    default_inspire=false;

    info_formats.push_back("text/plain");
    info_formats.push_back("text/xml");
    info_formats.push_back("text/html");
    info_formats.push_back("application/json");

    map_reprojection = true;
    tile_reprojection = false;

    map_max_layers_count = 1;
    map_max_width = 5000;
    map_max_height = 5000;
    map_max_tile_x = 32;
    map_max_tile_y = 32;

    map_formats.push_back("image/jpeg");
    map_formats.push_back("image/png");
    map_formats.push_back("image/tiff");
    map_formats.push_back("image/geotiff");
    map_formats.push_back("image/x-bil;bits=32");

    // ----------------------- Global 

    if (doc["global"].is_object()) {

        json11::Json global_section = doc["global"];

        if (global_section["fee"].is_string()) {
            fee = global_section["fee"].string_value();
        } else if (! global_section["fee"].is_null()) {
            error_message = "Services configuration: global.fee have to be a string";
            return false;
        }

        if (global_section["access_constraint"].is_string()) {
            access_constraint = global_section["access_constraint"].string_value();
        } else if (! global_section["access_constraint"].is_null()) {
            error_message = "Services configuration: global.access_constraint have to be a string";
            return false;
        }

        if (global_section["provider"].is_string()) {
            service_provider = global_section["provider"].string_value();
        } else if (! global_section["provider"].is_null()) {
            error_message = "Services configuration: global.provider have to be a string";
            return false;
        }

        if (global_section["site"].is_string()) {
            provider_site = global_section["site"].string_value();
        } else if (! global_section["site"].is_null()) {
            error_message = "Services configuration: global.site have to be a string";
            return false;
        }

        if (global_section["crs_equivalences"].is_string()) {
            std::string crs_equivalences_file = global_section["crs_equivalences"].string_value();
            if (! load_crs_equivalences(crs_equivalences_file)) {
                return false;
            }
        } else if (! global_section["crs_equivalences"].is_null()) {
            error_message = "Services configuration: global.crs_equivalences have to be a string";
            return false;
        }

        if (global_section["default_style"].is_string()) {
            default_style = global_section["default_style"].string_value();
        } else if (! global_section["default_style"].is_null()) {
            error_message = "Services configuration: global.default_style have to be a string";
            return false;
        }

        // Contact
        json11::Json contact_section = global_section["contact"];

        contact = new Contact(contact_section);
        if (! contact->is_ok()) {
            error_message = "Services configuration: " + contact->get_error_message();
            return false;
        }

        // Inspire
        if (global_section["inspire"].is_bool()) {
            default_inspire = global_section["inspire"].bool_value();
        } else if (! global_section["inspire"].is_null()) {
            error_message = "Services configuration: global.inspire have to be a boolean";
            return false;
        }

        // Map
        if (global_section["map"].is_object()) {

            json11::Json map_section = global_section["map"];

            if (map_section["reprojection"].is_bool()) {
                map_reprojection = map_section["reprojection"].bool_value();
            } else if (! map_section["reprojection"].is_null()) {
                error_message = "Services configuration: global.map.reprojection have to be a boolean";
                return false;
            }

            if (map_section["formats"].is_array()) {
                map_formats.clear();
                for (json11::Json f : map_section["formats"].array_items()) {
                    if (f.is_string()) {
                        std::string format = f.string_value();
                        if ( format != "image/jpeg" &&
                            format != "image/png"  &&
                            format != "image/tiff" &&
                            format != "image/geotiff" &&
                            format != "image/x-bil;bits=32" &&
                            format != "image/gif" && 
                            format != "text/asc" ) {
                            error_message = "Services configuration: global.map.formats [" + format + "] is not an handled MIME format";
                            return false;
                        } else {
                            map_formats.push_back ( format );
                        }
                    } else {
                        error_message = "Services configuration: global.map.formats have to be a string array";
                        return false;
                    }
                }
            } else if (! map_section["formats"].is_null()) {
                error_message = "Services configuration: global.map.formats have to be a string array";
                return false;
            }

            if (map_section["limits"].is_object()) {
                if (map_section["limits"]["layers_count"].is_number() && map_section["limits"]["layers_count"].number_value() >= 1) {
                    map_max_layers_count = map_section["limits"]["layers_count"].number_value();
                } else if (! map_section["limits"]["layers_count"].is_null()) {
                    error_message = "Services configuration: global.map.limits.layers_count have to be an integer >= 1";
                    return false;
                }

                if (map_section["limits"]["width"].is_number() && map_section["limits"]["width"].number_value() >= 1) {
                    map_max_width = map_section["limits"]["width"].number_value();
                } else if (! map_section["limits"]["width"].is_null()) {
                    error_message = "Services configuration: global.map.limits.width have to be an integer >= 1";
                    return false;
                }

                if (map_section["limits"]["height"].is_number() && map_section["limits"]["height"].number_value() >= 1) {
                    map_max_height = map_section["limits"]["height"].number_value();
                } else if (! map_section["limits"]["height"].is_null()) {
                    error_message = "Services configuration: global.map.limits.height have to be an integer >= 1";
                    return false;
                }

                if (map_section["limits"]["tile_x"].is_number() && map_section["limits"]["tile_x"].number_value() >= 1) {
                    map_max_tile_x = map_section["limits"]["tile_x"].number_value();
                } else if (! map_section["limits"]["tile_x"].is_null()) {
                    error_message = "Services configuration: global.map.limits.tile_x have to be an integer >= 1";
                    return false;
                }

                if (map_section["limits"]["tile_y"].is_number() && map_section["limits"]["tile_y"].number_value() >= 1) {
                    map_max_tile_y = map_section["limits"]["tile_y"].number_value();
                } else if (! map_section["limits"]["tile_y"].is_null()) {
                    error_message = "Services configuration: global.map.limits.tile_y have to be an integer >= 1";
                    return false;
                }

            } else if (! map_section["limits"].is_null()) {
                error_message = "Services configuration: global.map.limits have to be an object";
                return false;
            }


            bool crs84_present = false;
            if (map_reprojection && map_section["crs"].is_array()) {
                for (json11::Json c : map_section["crs"].array_items()) {
                    if (c.is_string()) {
                        std::string crs_string = c.string_value();

                        CRS* crs = CrsBook::get_crs( crs_string );
                        if ( ! crs->is_define() ) {
                            BOOST_LOG_TRIVIAL(warning) << "The (WMS) CRS [" << crs_string <<"] is not present in PROJ"  ;
                            continue;
                        }

                        BOOST_LOG_TRIVIAL(info) <<  "Adding global CRS " << crs->get_request_code()   ;
                        map_crss.push_back(crs);
                        if (crs->get_request_code() == "CRS:84") {
                            crs84_present = true;
                        }

                        if (handle_crs_equivalences()) {
                            std::vector<CRS*> eqs = get_equals_crs(crs->get_request_code());
                            size_t init_size = map_crss.size();
                            for (unsigned int e = 0; e < eqs.size(); e++) {
                                bool already_in = false;
                                for ( int i = 0; i < init_size ; i++ ) {
                                    if (map_crss.at( i )->cmp_request_code(eqs.at(e)->get_request_code() ) ){
                                        already_in = true;
                                        break;
                                    }
                                }
                                if (! already_in) {
                                    BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqs.at(e)->get_request_code() <<"] of [" << crs->get_request_code() << "]"  ;
                                    map_crss.push_back(eqs.at(e));
                                    if (eqs.at(e)->get_request_code() == "CRS:84") {
                                        crs84_present = true;
                                    }
                                }
                            }
                        }
                    } else {
                        error_message = "WMS service: crs have to be a string array";
                        return false;
                    }
                }
            } else if (! map_section["crs"].is_null()) {
                error_message = "WMS service: crs have to be a string array with enabled reprojection";
                return false;
            }

            if (! crs84_present) {
                BOOST_LOG_TRIVIAL(info) <<  "CRS:84 not found -> adding global CRS CRS:84"   ;
                CRS* crs = CrsBook::get_crs( "CRS:84" );

                if ( ! crs->is_define() ) {
                    error_message = "WMS service: The CRS [CRS:84] is not present in PROJ"  ;
                    return false;
                }

                map_crss.push_back ( crs );

                if (handle_crs_equivalences()) {
                    std::vector<CRS*> eqs = get_equals_crs(crs->get_request_code());
                    size_t init_size = map_crss.size();
                    for (unsigned int e = 0; e < eqs.size(); e++) {
                        bool already_in = false;
                        for ( int i = 0; i < init_size ; i++ ) {
                            if (map_crss.at( i )->cmp_request_code(eqs.at(e)->get_request_code() ) ){
                                already_in = true;
                            }
                        }
                        if (! already_in) {
                            BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqs.at(e)->get_request_code() <<"] of [CRS:84]"  ;
                            map_crss.push_back(eqs.at(e));
                        }
                    }
                }
            }  

        } else if (! global_section["map"].is_null()) {
            error_message = "Services configuration: global.map have to be an object";
            return false;
        }

        // Tiles

        
        if (global_section["tile"].is_object()) {

            json11::Json tile_section = global_section["tile"];


            if (tile_section["reprojection"].is_bool()) {
                tile_reprojection = doc["reprojection"].bool_value();
            } else if (! tile_section["reprojection"].is_null()) {
                error_message = "Services configuration: global.tile.reprojection have to be a boolean";
                return false;
            }


        } else if (! global_section["tile"].is_null()) {
            error_message = "Services configuration: global.tile have to be an object";
            return false;
        }

    } else if (! doc["global"].is_null()) {
        error_message = "Services configuration: global have to be an object";
        return false;
    }

    // ----------------------- Chargement des services

    json11::Json health_section = doc["health"];

    health_service = new HealthService(health_section);
    if (! health_service->is_ok()) {
        error_message = "Services configuration: " + health_service->get_error_message();
        return false;
    }
    if (health_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "HEALTH service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "HEALTH service disabled";
    }

    json11::Json tms_section = doc["tms"];

    tms_service = new TmsService(tms_section);
    if (! tms_service->is_ok()) {
        error_message = "Services configuration: " + tms_service->get_error_message();
        return false;
    }
    if (tms_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "TMS service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "TMS service disabled";
    }

    json11::Json wmts_section = doc["wmts"];

    wmts_service = new WmtsService(wmts_section);
    if (! wmts_service->is_ok()) {
        error_message = "Services configuration: " + wmts_service->get_error_message();
        return false;
    }
    if (wmts_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "WMTS service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "WMTS service disabled";
    }

    json11::Json wms_section = doc["wms"];

    wms_service = new WmsService(wms_section, this);
    if (! wms_service->is_ok()) {
        error_message = "Services configuration: " + wms_service->get_error_message();
        return false;
    }
    if (wms_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "Services configuration enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "Services configuration disabled";
    }

    json11::Json admin_section = doc["admin"];

    admin_service = new AdminService(admin_section);
    if (! admin_service->is_ok()) {
        error_message = "Services configuration: " + admin_service->get_error_message();
        return false;
    }
    if (admin_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "ADMIN service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "ADMIN service disabled";
    }

    json11::Json ogcapi_section = doc["ogcapi"];

    ogcapi_service = new OgcApiService(ogcapi_section);
    if (! ogcapi_service->is_ok()) {
        error_message = "Services configuration: " + ogcapi_service->get_error_message();
        return false;
    }
    if (ogcapi_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "OGC API service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "OGC API service disabled";
    }

    return true;
}

ServicesConfiguration::ServicesConfiguration(std::string path) : Configuration(path) {

    std::cout << "Loading services configuration from file " << file_path << std::endl;

    std::ifstream is(file_path);
    std::stringstream ss;
    ss << is.rdbuf();

    std::string err;
    json11::Json doc = json11::Json::parse ( ss.str(), err );
    if ( doc.is_null() ) {
        error_message = "Cannot load JSON file "  + file_path + " : " + err ;
        return;
    }

    /********************** Parse */

    if (! parse(doc)) {
        return;
    }

    return;
}


bool ServicesConfiguration::load_crs_equivalences(std::string file) {

    std::ifstream is(file);
    std::stringstream ss;
    ss << is.rdbuf();

    std::string err;
    json11::Json doc = json11::Json::parse ( ss.str(), err );
    if ( doc.is_null() ) {
        error_message = "Cannot load JSON file "  + file + " : " + err ;
        return false;
    }

    BOOST_LOG_TRIVIAL(info) << "Load CRS equivalences from file " << file  ;

    if (! doc.is_object()) {
        error_message = "CRS equivalences file have to be a JSON object";
        return false;
    }

    for (auto const& it : doc.object_items()) {
        std::string index_crs = it.first;
        std::vector<CRS*> eqs;

        // On met en premier le CRS correspondant à la clé
        CRS* c = CrsBook::get_crs( index_crs );
        index_crs = c->get_request_code();
        if ( ! c->is_define() ) {
            BOOST_LOG_TRIVIAL(warning) << "The Equivalent CRS [" << c << "] is not present in PROJ"  ;
            continue;
        } else {
            eqs.push_back(c);
        }

        if (it.second.is_array()) {
            for (json11::Json eq_crs : it.second.array_items()) {
                if (eq_crs.is_string()) {
                    c = CrsBook::get_crs( eq_crs.string_value() );
                    if ( ! c->is_define() ) {
                        BOOST_LOG_TRIVIAL(warning) << "The Equivalent CRS [" << c << "] is not present in PROJ";
                    } else {
                        eqs.push_back(c);
                    }
                } else {
                    error_message = "CRS equivalences file have to be a JSON object where each value is a string array";
                    return false;
                }
            }
        } else {
            error_message = "CRS equivalences file have to be a JSON object where each value is a string array";
            return false;
        }

        crs_equivalences.insert ( std::pair<std::string, std::vector<CRS*> > ( index_crs, eqs ) );
    }

    return true;
}

std::vector<CRS*> ServicesConfiguration::get_equals_crs(std::string crs)
{
    std::map<std::string, std::vector<CRS*> >::iterator it = crs_equivalences.find ( to_upper_case(crs) );
    if ( it == crs_equivalences.end() ) {
        return {};
    }
    return it->second;
}

bool ServicesConfiguration::are_crs_equals( std::string crs1, std::string crs2 ) {

    if (to_upper_case(crs1) == to_upper_case(crs2)) {
        return true;
    }

    std::vector<CRS*> eqs = get_equals_crs(crs1);
    // On commence à 1, le premier CRS correpondant à la clé
    for ( unsigned int i = 1; i < eqs.size(); i++ ) {
        if (eqs.at(i)->cmp_request_code(crs2)) {
            return true;
        }
    }

    eqs = get_equals_crs(crs2);
    for ( unsigned int i = 1; i < eqs.size(); i++ ) {
        if (eqs.at(i)->cmp_request_code(crs1)) {
            return true;
        }
    }

    return false;
}

bool ServicesConfiguration::is_available_infoformat(std::string f) {
    return (std::find(info_formats.begin(), info_formats.end(), f) != info_formats.end());
}

std::vector<std::string>* ServicesConfiguration::get_available_infoformats() {
    return &info_formats;
}


bool ServicesConfiguration::is_map_available_crs(CRS* c) {
    return is_map_available_crs(c->get_request_code());
}
bool ServicesConfiguration::is_map_available_crs(std::string c) {
    if (! map_reprojection) {
        return false;
    }
    for ( unsigned int k = 0; k < map_crss.size(); k++ ) {
        if ( map_crss.at (k)->cmp_request_code ( c ) ) {
            return true;
        }
    }
    return false;
}

bool ServicesConfiguration::is_map_available_format(std::string f) {
    return (std::find(map_formats.begin(), map_formats.end(), f) != map_formats.end());
}

std::vector<CRS*>* ServicesConfiguration::get_map_available_crs() {
    return &map_crss;
}

ServicesConfiguration::~ServicesConfiguration(){ 
    delete tms_service;
    delete health_service;
    delete wmts_service;
    delete wms_service;
    delete admin_service;
    delete ogcapi_service;
    delete contact;

    // Les couches
    std::map<std::string, Layer*>::iterator itLay;
    for ( itLay = layers.begin(); itLay != layers.end(); itLay++ )
        delete itLay->second;
}

void ServicesConfiguration::clean_cache() {
    wms_service->clean_cache();
    wmts_service->clean_cache();
    tms_service->clean_cache();
    ogcapi_service->clean_cache();
};
std::map<std::string, Layer*>& ServicesConfiguration::get_layers() {return layers;}
void ServicesConfiguration::add_layer(Layer* l) {
    layers.insert ( std::pair<std::string, Layer *> ( l->get_id(), l ) );
}
int ServicesConfiguration::get_layers_count() {
    return layers.size();
}
Layer* ServicesConfiguration::get_layer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layers.find ( id );
    if ( itLay == layers.end() ) {
        return NULL;
    }
    return itLay->second;
}
void ServicesConfiguration::delete_layer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layers.find ( id );
    if ( itLay != layers.end() ) {
        delete itLay->second;
        layers.erase(itLay);
    }
}