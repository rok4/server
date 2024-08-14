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

    // ----------------------- Global 

    if (doc["fee"].is_string()) {
        fee = doc["fee"].string_value();
    } else if (! doc["fee"].is_null()) {
        error_message = "Services configuration: fee have to be a string";
        return false;
    }

    if (doc["access_constraint"].is_string()) {
        access_constraint = doc["access_constraint"].string_value();
    } else if (! doc["access_constraint"].is_null()) {
        error_message = "Services configuration: access_constraint have to be a string";
        return false;
    }

    if (doc["provider"].is_string()) {
        service_provider = doc["provider"].string_value();
    } else if (! doc["provider"].is_null()) {
        error_message = "Services configuration: provider have to be a string";
        return false;
    }

    if (doc["site"].is_string()) {
        provider_site = doc["site"].string_value();
    } else if (! doc["site"].is_null()) {
        error_message = "Services configuration: site have to be a string";
        return false;
    }

    if (doc["crs_equivalences"].is_string()) {
        std::string crs_equivalences_file = doc["crs_equivalences"].string_value();
        if (! load_crs_equivalences(crs_equivalences_file)) {
            return false;
        }
    } else if (! doc["crs_equivalences"].is_null()) {
        error_message = "crs_equivalences have to be a string";
        return false;
    }

    // ----------------------- Contact

    json11::Json contact_section = doc["contact"];

    contact = new Contact(contact_section);
    if (! contact->is_ok()) {
        error_message = "Services configuration: " + contact->get_error_message();
        return false;
    }

    // ----------------------- Chargement des services

    json11::Json common_section = doc["common"];

    common_service = new CommonService(common_section);
    if (! common_service->is_ok()) {
        error_message = "Services configuration: " + common_service->get_error_message();
        return false;
    }
    if (common_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "COMMON service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "COMMON service disabled";
    }

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
        BOOST_LOG_TRIVIAL(info) <<  "WMS service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "WMS service disabled";
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

    json11::Json tiles_section = doc["tiles"];

    tiles_service = new TilesService(tiles_section);
    if (! tiles_service->is_ok()) {
        error_message = "Services configuration: " + tiles_service->get_error_message();
        return false;
    }
    if (tiles_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "TILES service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "TILES service disabled";
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
        CRS* c = new CRS ( index_crs );
        index_crs = c->get_request_code();
        if ( ! c->is_define() ) {
            BOOST_LOG_TRIVIAL(warning) << "The Equivalent CRS [" << c << "] is not present in PROJ"  ;
            delete c;
            continue;
        } else {
            eqs.push_back(c);
        }

        if (it.second.is_array()) {
            for (json11::Json eq_crs : it.second.array_items()) {
                if (eq_crs.is_string()) {
                    c = new CRS ( eq_crs.string_value() );
                    if ( ! c->is_define() ) {
                        BOOST_LOG_TRIVIAL(warning) << "The Equivalent CRS [" << c << "] is not present in PROJ"  ;
                        delete c;
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

    if (crs1 == crs2) {
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

ServicesConfiguration::~ServicesConfiguration(){ 
    delete common_service;
    delete tms_service;
    delete health_service;
    delete wmts_service;
    delete wms_service;
    delete admin_service;
    delete tiles_service;
    delete contact;

    // Les CRS équivalents
    std::map<std::string, std::vector<CRS*> >::iterator it;
    for ( it = crs_equivalences.begin(); it != crs_equivalences.end(); it++ )
        for (unsigned int i = 0 ; i < it->second.size() ; i++) {
            delete it->second.at(i);
        }
}
