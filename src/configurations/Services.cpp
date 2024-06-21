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
    individual_name="";
    individual_position="";
    voice="";
    facsimile="";
    address_type="";
    delivery_point="";
    city="";
    administrative_area="";
    post_code="";
    country="";
    email="";

    // ----------------------- Global 

    if (doc["fee"].is_string()) {
        fee = doc["fee"].string_value();
    } else if (! doc["fee"].is_null()) {
        errorMessage = "Services configuration: fee have to be a string";
        return false;
    }

    if (doc["access_constraint"].is_string()) {
        access_constraint = doc["access_constraint"].string_value();
    } else if (! doc["access_constraint"].is_null()) {
        errorMessage = "Services configuration: access_constraint have to be a string";
        return false;
    }

    if (doc["provider"].is_string()) {
        service_provider = doc["provider"].string_value();
    } else if (! doc["provider"].is_null()) {
        errorMessage = "Services configuration: provider have to be a string";
        return false;
    }

    if (doc["site"].is_string()) {
        provider_site = doc["site"].string_value();
    } else if (! doc["site"].is_null()) {
        errorMessage = "Services configuration: site have to be a string";
        return false;
    }

    if (doc["crs_equivalences"].is_string()) {
        std::string crs_equivalences_file = doc["crs_equivalences"].string_value();
        if (! load_crs_equivalences(crs_equivalences_file)) {
            return false;
        }
    } else if (! doc["crs_equivalences"].is_null()) {
        errorMessage = "crs_equivalences have to be a string";
        return false;
    }

    // ----------------------- Contact

    json11::Json contactSection = doc["contact"];
    if (contactSection.is_null()) {
        errorMessage = "No contact section";
        return false;
    } else if (! contactSection.is_object()) {
        errorMessage = "Services configuration: contact have to be an object";
        return false;
    }
    
    if (contactSection["name"].is_string()) {
        individual_name = contactSection["name"].string_value();
    } else if (! contactSection["name"].is_null()) {
        errorMessage = "Services configuration: contact.name have to be a string";
        return false;
    }
    if (contactSection["position"].is_string()) {
        individual_position = contactSection["position"].string_value();
    } else if (! contactSection["position"].is_null()) {
        errorMessage = "Services configuration: contact.position have to be a string";
        return false;
    }
    if (contactSection["voice"].is_string()) {
        voice = contactSection["voice"].string_value();
    } else if (! contactSection["voice"].is_null()) {
        errorMessage = "Services configuration: contact.voice have to be a string";
        return false;
    }
    if (contactSection["facsimile"].is_string()) {
        facsimile = contactSection["facsimile"].string_value();
    } else if (! contactSection["facsimile"].is_null()) {
        errorMessage = "Services configuration: contact.facsimile have to be a string";
        return false;
    }
    if (contactSection["address_type"].is_string()) {
        address_type = contactSection["address_type"].string_value();
    } else if (! contactSection["address_type"].is_null()) {
        errorMessage = "Services configuration: contact.address_type have to be a string";
        return false;
    }
    if (contactSection["delivery_point"].is_string()) {
        delivery_point = contactSection["delivery_point"].string_value();
    } else if (! contactSection["delivery_point"].is_null()) {
        errorMessage = "Services configuration: contact.delivery_point have to be a string";
        return false;
    }
    if (contactSection["city"].is_string()) {
        city = contactSection["city"].string_value();
    } else if (! contactSection["city"].is_null()) {
        errorMessage = "Services configuration: contact.city have to be a string";
        return false;
    }
    if (contactSection["administrative_area"].is_string()) {
        administrative_area = contactSection["administrative_area"].string_value();
    } else if (! contactSection["administrative_area"].is_null()) {
        errorMessage = "Services configuration: contact.administrative_area have to be a string";
        return false;
    }
    if (contactSection["post_code"].is_string()) {
        post_code = contactSection["post_code"].string_value();
    } else if (! contactSection["post_code"].is_null()) {
        errorMessage = "Services configuration: contact.post_code have to be a string";
        return false;
    }
    if (contactSection["country"].is_string()) {
        country = contactSection["country"].string_value();
    } else if (! contactSection["country"].is_null()) {
        errorMessage = "Services configuration: contact.country have to be a string";
        return false;
    }
    if (contactSection["email"].is_string()) {
        email = contactSection["email"].string_value();
    } else if (! contactSection["email"].is_null()) {
        errorMessage = "Services configuration: contact.email have to be a string";
        return false;
    }

    // ----------------------- Commun à plusieurs services 
    // json11::Json commonSection = doc["common"];
    // if (commonSection.is_object()) {
        
    //     if (commonSection["styling"].is_bool()) {
    //         fullStyling = commonSection["styling"].bool_value();
    //     } else if (! commonSection["styling"].is_null()) {
    //         errorMessage = "common.styling have to be a boolean";
    //         return false;
    //     }

    // } else if (! commonSection.is_null()) {
    //     errorMessage = "common have to be an object";
    //     return false;
    // }

    // ----------------------- Chargement des services

    json11::Json common_section = doc["common"];

    common_service = new CommonService(common_section);
    if (! common_service->isOk()) {
        errorMessage = "Services configuration: " + common_service->getErrorMessage();
        return false;
    }
    if (common_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "COMMON service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "COMMON service disabled";
    }

    json11::Json health_section = doc["health"];

    health_service = new HealthService(health_section);
    if (! health_service->isOk()) {
        errorMessage = "Services configuration: " + health_service->getErrorMessage();
        return false;
    }
    if (health_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "HEALTH service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "HEALTH service disabled";
    }

    json11::Json tms_section = doc["tms"];

    tms_service = new TmsService(tms_section);
    if (! tms_service->isOk()) {
        errorMessage = "Services configuration: " + tms_service->getErrorMessage();
        return false;
    }
    if (tms_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "TMS service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "TMS service disabled";
    }

    json11::Json wmts_section = doc["wmts"];

    wmts_service = new WmtsService(wmts_section);
    if (! wmts_service->isOk()) {
        errorMessage = "Services configuration: " + wmts_service->getErrorMessage();
        return false;
    }
    if (wmts_service->is_enabled()) {
        BOOST_LOG_TRIVIAL(info) <<  "WMTS service enabled";
    } else {
        BOOST_LOG_TRIVIAL(info) <<  "WMTS service disabled";
    }

    // ----------------------- WMS 
    // json11::Json wmsSection = doc["wms"];
    // if (wmsSection.is_null()) {
    //     supportWMS = false;
    // } else if (! wmsSection.is_object()) {
    //     errorMessage = "wms have to be an object";
    //     return false;
    // } else {

    //     if (wmsSection["name"].is_string()) {
    //         name = wmsSection["name"].string_value();
    //     } else if (! wmsSection["name"].is_null()) {
    //         errorMessage = "wms.name have to be a string";
    //         return false;
    //     }
    //     if (wmsSection["layer_root_title"].is_string()) {
    //         layerRootTitle = wmsSection["layer_root_title"].string_value();
    //     } else if (! wmsSection["layer_root_title"].is_null()) {
    //         errorMessage = "wms.layer_root_title have to be a string";
    //         return false;
    //     } else {
    //         layerRootTitle = "WMS layers";
    //     }
    //     if (wmsSection["layer_root_abstract"].is_string()) {
    //         layerRootAbstract = wmsSection["layer_root_abstract"].string_value();
    //     } else if (! wmsSection["layer_root_abstract"].is_null()) {
    //         errorMessage = "wms.layer_root_abstract have to be a string";
    //         return false;
    //     } else {
    //         layerRootAbstract = "WMS layers";
    //     }
    //     if (wmsSection["max_width"].is_number()) {
    //         maxWidth = wmsSection["max_width"].int_value();
    //     } else if (! wmsSection["max_width"].is_null()) {
    //         errorMessage = "wms.max_width have to be an integer";
    //         return false;
    //     }
    //     if (wmsSection["max_height"].is_number()) {
    //         maxHeight = wmsSection["max_height"].int_value();
    //     } else if (! wmsSection["max_height"].is_null()) {
    //         errorMessage = "wms.max_height have to be an integer";
    //         return false;
    //     }
    //     if (wmsSection["layer_limit"].is_number()) {
    //         layerLimit = wmsSection["layer_limit"].int_value();
    //     } else if (! wmsSection["layer_limit"].is_null()) {
    //         errorMessage = "wms.layer_limit have to be an integer";
    //         return false;
    //     }
    //     if (wmsSection["max_tile_x"].is_number()) {
    //         maxTileX = wmsSection["max_tile_x"].int_value();
    //     } else if (! wmsSection["max_tile_x"].is_null()) {
    //         errorMessage = "wms.max_tile_x have to be an integer";
    //         return false;
    //     }
    //     if (wmsSection["max_tile_y"].is_number()) {
    //         maxTileY = wmsSection["max_tile_y"].int_value();
    //     } else if (! wmsSection["max_tile_y"].is_null()) {
    //         errorMessage = "wms.max_tile_y have to be an integer";
    //         return false;
    //     }
    //     if (wmsSection["formats"].is_array()) {
    //         for (json11::Json f : wmsSection["formats"].array_items()) {
    //             if (f.is_string()) {
    //                 std::string format = f.string_value();
    //                 if ( format != "image/jpeg" &&
    //                     format != "image/png"  &&
    //                     format != "image/tiff" &&
    //                     format != "image/geotiff" &&
    //                     format != "image/x-bil;bits=32" &&
    //                     format != "image/gif" && 
    //                     format != "text/asc" ) {
    //                     errorMessage = "WMS image format [" + format + "] is not an handled MIME format";
    //                     return false;
    //                 } else {
    //                     formatList.push_back ( format );
    //                 }
    //             } else {
    //                 errorMessage = "wms.formats have to be a string array";
    //                 return false;
    //             }
    //         }
    //     } else if (! wmsSection["formats"].is_null()) {
    //         errorMessage = "wms.formats have to be a string array";
    //         return false;
    //     }

    //     // WMS Global CRS List
    //     bool isCRS84 = false;
    //     if (wmsSection["crs"].is_array()) {
    //         for (json11::Json c : wmsSection["crs"].array_items()) {
    //             if (c.is_string()) {
    //                 std::string crsStr = c.string_value();
    //                 if (! isCRSAllowed(crsStr)) {
    //                     BOOST_LOG_TRIVIAL(warning) <<  "Forbiden global CRS " << crsStr ;
    //                     continue;
    //                 }
    //                 if (isInGlobalCRSList(crsStr)) {
    //                     continue;
    //                 }

    //                 CRS* crs = new CRS( crsStr );
    //                 if ( ! crs->isDefine() ) {
    //                     BOOST_LOG_TRIVIAL(warning) << "The (WMS) CRS [" << crsStr <<"] is not present in PROJ"  ;
    //                     continue;
    //                 }

    //                 BOOST_LOG_TRIVIAL(info) <<  "Adding global CRS " << crs->getRequestCode()   ;
    //                 globalCRSList.push_back(crs);
    //                 if (crs->getRequestCode() == "CRS:84") {
    //                     isCRS84 = true;
    //                 }

    //                 if (doweuselistofequalsCRS) {
    //                     std::vector<CRS*> eqCRS = getEqualsCRS(crs->getRequestCode());
    //                     size_t init_size = globalCRSList.size();
    //                     for (unsigned int e = 0; e < eqCRS.size(); e++) {
    //                         bool already_in = false;
    //                         for ( int i = 0; i < init_size ; i++ ) {
    //                             if (globalCRSList.at( i )->cmpRequestCode(eqCRS.at(e)->getRequestCode() ) ){
    //                                 already_in = true;
    //                                 break;
    //                             }
    //                         }
    //                         if (! already_in) {
    //                             BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqCRS.at(e)->getRequestCode() <<"] of [" << crs->getRequestCode() << "]"  ;
    //                             // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
    //                             globalCRSList.push_back(new CRS(eqCRS.at(e)));
    //                             if (eqCRS.at(e)->getRequestCode() == "CRS:84") {
    //                                 isCRS84 = true;
    //                             }
    //                         }
    //                     }
    //                 }


    //             } else {
    //                 errorMessage = "wms.crs have to be a string array";
    //                 return false;
    //             }
    //         }
    //     } else if (! wmsSection["crs"].is_null()) {
    //         errorMessage = "wms.crs have to be a string array";
    //         return false;
    //     }
    //     // On veut forcément CRS:84 comme CRS global
    //     if (! isCRS84) {
    //         BOOST_LOG_TRIVIAL(info) <<  "CRS:84 not found -> adding global CRS CRS:84"   ;
    //         CRS* crs = new CRS( "CRS:84" );

    //         if ( ! crs->isDefine() ) {
    //             errorMessage = "The CRS [CRS:84] is not present in PROJ"  ;
    //             return false;
    //         }

    //         globalCRSList.push_back ( crs );

    //         if (doweuselistofequalsCRS) {
    //             std::vector<CRS*> eqCRS = getEqualsCRS(crs->getRequestCode());
    //             size_t init_size = globalCRSList.size();
    //             for (unsigned int e = 0; e < eqCRS.size(); e++) {
    //                 bool already_in = false;
    //                 for ( int i = 0; i < init_size ; i++ ) {
    //                     if (globalCRSList.at( i )->cmpRequestCode(eqCRS.at(e)->getRequestCode() ) ){
    //                         already_in = true;
    //                     }
    //                 }
    //                 if (! already_in) {
    //                     BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqCRS.at(e)->getRequestCode() <<"] of [CRS:84]"  ;
    //                     // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
    //                     globalCRSList.push_back(new CRS(eqCRS.at(e)));
    //                 }
    //             }
    //         }
    //     }

    //     // Métadonnée
    //     if (wmsSection["metadata"].is_object()) {
    //         mtdWMS = new Metadata ( wmsSection["metadata"] );
    //         if (mtdWMS->getMissingField() != "") {
    //             errorMessage = "Invalid WMS metadata: have to own a field " + mtdWMS->getMissingField();
    //             return false;
    //         }
    //     } else if (inspire) {
    //         errorMessage = "Inspire WMS service require a metadata";
    //         return false;
    //     }
        
    // }


    return true;
}

ServicesConfiguration::ServicesConfiguration(std::string path) : Configuration(path) {

    std::cout << "Loading services configuration from file " << filePath << std::endl;

    std::ifstream is(filePath);
    std::stringstream ss;
    ss << is.rdbuf();

    std::string err;
    json11::Json doc = json11::Json::parse ( ss.str(), err );
    if ( doc.is_null() ) {
        errorMessage = "Cannot load JSON file "  + filePath + " : " + err ;
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
        errorMessage = "Cannot load JSON file "  + file + " : " + err ;
        return false;
    }

    BOOST_LOG_TRIVIAL(info) << "Load CRS equivalences from file " << file  ;

    if (! doc.is_object()) {
        errorMessage = "CRS equivalences file have to be a JSON object";
        return false;
    }

    for (auto const& it : doc.object_items()) {
        std::string index_crs = it.first;
        std::vector<CRS*> eqs;

        // On met en premier le CRS correspondant à la clé
        CRS* c = new CRS ( index_crs );
        if ( ! c->isDefine() ) {
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
                    if ( ! c->isDefine() ) {
                        BOOST_LOG_TRIVIAL(warning) << "The Equivalent CRS [" << c << "] is not present in PROJ"  ;
                        delete c;
                    } else {
                        eqs.push_back(c);
                    }
                } else {
                    errorMessage = "CRS equivalences file have to be a JSON object where each value is a string array";
                    return false;
                }
            }
        } else {
            errorMessage = "CRS equivalences file have to be a JSON object where each value is a string array";
            return false;
        }

        crs_equivalences.insert ( std::pair<std::string, std::vector<CRS*> > ( index_crs, eqs ) );
    }

    return true;
}

std::vector<CRS*> ServicesConfiguration::get_equals_crs(std::string crs)
{
    std::map<std::string, std::vector<CRS*> >::iterator it = crs_equivalences.find ( crs );
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
        if (eqs.at(i)->cmpRequestCode(crs2)) {
            return true;
        }
    }

    eqs = get_equals_crs(crs2);
    for ( unsigned int i = 1; i < eqs.size(); i++ ) {
        if (eqs.at(i)->cmpRequestCode(crs1)) {
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

    // Les CRS équivalents
    std::map<std::string, std::vector<CRS*> >::iterator it;
    for ( it = crs_equivalences.begin(); it != crs_equivalences.end(); it++ )
        for (unsigned int i = 0 ; i < it->second.size() ; i++) {
            delete it->second.at(i);
        }
}
