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

#include "ServicesConf.h"
#include <fstream>

bool ServicesConf::loadEqualsCRSList(std::string file) {

    BOOST_LOG_TRIVIAL(info) << "Liste des CRS équivalents à partir du fichier " << file  ;

    std::string delimiter = " ";
    std::string crsStr;

    std::ifstream input ( file.c_str() );
    // We test if the stream is empty
    //   This can happen when the file can't be loaded or when the file is empty
    if ( input.peek() == std::ifstream::traits_type::eof() ) {
        errorMessage = "Cannot load equals CRS list file " + file + " or empty file"   ;
        return false;
    }
    
    for( std::string line; getline(input, line); ) {
        if (line[0] == '#' ) {
            continue;
        }

        // We split line to process every CRS in line, separated with space
        size_t pos = 0;
        std::vector<CRS*> crsVector;
        while ((pos = line.find(delimiter)) != std::string::npos) {
            crsStr = line.substr(0, pos);
            line.erase(0, pos + delimiter.length());

            CRS* crs = new CRS ( crsStr );
            if ( ! crs->isDefine() ) {
                BOOST_LOG_TRIVIAL(warning) <<   "The Equivalent CRS [" << crsStr << "] is not present in PROJ"  ;
                delete crs;
            } else {
                crsVector.push_back(crs);
            }
        }

        CRS* crs = new CRS ( line );
        if ( ! crs->isDefine() ) {
            BOOST_LOG_TRIVIAL(warning) <<   "The Equivalent CRS [" << line << "] is not present in PROJ"  ;
            delete crs;
        } else {
            crsVector.push_back(crs);
        }

        if (crsVector.size() != 0) {
            listofequalsCRS.push_back(crsVector);
        }
    }

    return true;
}


bool ServicesConf::loadRestrictedCRSList(std::string file) {

    BOOST_LOG_TRIVIAL(info) <<  "Liste restreinte de CRS à partir du fichier " << file  ;

    std::ifstream input ( file.c_str() );
    // We test if the stream is empty
    //   This can happen when the file can't be loaded or when the file is empty
    if ( input.peek() == std::ifstream::traits_type::eof() ) {
        errorMessage = "Cannot load restricted CRS list file " + file + " or empty file"   ;
        return false;
    }
    
    for( std::string line; getline(input, line); ) {
        if (line[0] == '#' ) {
            continue;
        }

        CRS* crs = new CRS ( line );
        if ( ! crs->isDefine() ) {
            BOOST_LOG_TRIVIAL(warning) <<   "The Equivalent CRS [" << line << "] is not present in PROJ"  ;
            delete crs;
        } else {
            restrictedCRSList.push_back(crs);
        }
    }

    if (doweuselistofequalsCRS) {
        // On va ajouter des éléments à la liste au fur et à mesure, on ne veut boucler que sur la liste initiale
        size_t init_size = restrictedCRSList.size();
        // On ajoute les CRS équivalents à ceux autorisés dans la liste
        for (unsigned int l = 0 ; l < init_size ; l++) {

            std::vector<CRS*> eqCRS = getEqualsCRS(restrictedCRSList.at(l)->getRequestCode());

            for (unsigned int e = 0 ; e < eqCRS.size() ; e++) {
                // On ne remet pas le CRS d'origine
                if (! eqCRS.at(e)->cmpRequestCode(restrictedCRSList.at(l)->getRequestCode())) {
                    // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
                    restrictedCRSList.push_back(new CRS(eqCRS.at(e)));
                }
            }            
        }
    }

    return true;
}

std::vector<CRS*> ServicesConf::getEqualsCRS(std::string crs)
{
    std::vector<CRS*> returnCRS;

    if (! doweuselistofequalsCRS) {
        return returnCRS;
    }

    for ( unsigned int l = 0; l < listofequalsCRS.size(); l++ ) {
        for ( unsigned int c = 0; c < listofequalsCRS.at(l).size(); c++ ) {
            if (listofequalsCRS.at(l).at(c)->cmpRequestCode(crs)) {
                return listofequalsCRS.at(l);
            }
        }
    }
    return returnCRS;
}

bool ServicesConf::isCRSAllowed(std::string crs) {

    if (! dowerestrictCRSList) {return true;}

    for (unsigned int l = 0 ; l < restrictedCRSList.size() ; l++) {
        if (restrictedCRSList.at(l)->cmpRequestCode(crs)) {
            return true;
        }
    }

    return false;
}

bool ServicesConf::parse(json11::Json& doc) {

    /********************** Default values */

    name="";
    title="";
    abstract="";
    serviceProvider="";
    fee="";
    accessConstraint="";
    postMode = false;
    providerSite="";
    individualName="";
    individualPosition="";
    voice="";
    facsimile="";
    addressType="";
    deliveryPoint="";
    city="";
    administrativeArea="";
    postCode="";
    country="";
    electronicMailAddress="";
    fullStyling = false;
    inspire = false;
    doweuselistofequalsCRS = false;
    dowerestrictCRSList = false;

    // ----------------------- Global 

    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    } else if (! doc["title"].is_null()) {
        errorMessage = "title have to be a string";
        return false;
    }

    if (doc["abstract"].is_string()) {
        abstract = doc["abstract"].string_value();
    } else if (! doc["abstract"].is_null()) {
        errorMessage = "abstract have to be a string";
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

    if (doc["fee"].is_string()) {
        fee = doc["fee"].string_value();
    } else if (! doc["fee"].is_null()) {
        errorMessage = "fee have to be a string";
        return false;
    }

    if (doc["access_constraint"].is_string()) {
        accessConstraint = doc["access_constraint"].string_value();
    } else if (! doc["access_constraint"].is_null()) {
        errorMessage = "access_constraint have to be a string";
        return false;
    }

    if (doc["provider"].is_string()) {
        serviceProvider = doc["provider"].string_value();
    } else if (! doc["provider"].is_null()) {
        errorMessage = "provider have to be a string";
        return false;
    }

    if (doc["site"].is_string()) {
        providerSite = doc["site"].string_value();
    } else if (! doc["site"].is_null()) {
        errorMessage = "site have to be a string";
        return false;
    }

    // ----------------------- Contact

    json11::Json contactSection = doc["contact"];
    if (contactSection.is_null()) {
        errorMessage = "No contact section";
        return false;
    } else if (! contactSection.is_object()) {
        errorMessage = "contact have to be an object";
        return false;
    }
    
    if (contactSection["name"].is_string()) {
        individualName = contactSection["name"].string_value();
    } else if (! contactSection["name"].is_null()) {
        errorMessage = "contact.name have to be a string";
        return false;
    }
    if (contactSection["position"].is_string()) {
        individualPosition = contactSection["position"].string_value();
    } else if (! contactSection["position"].is_null()) {
        errorMessage = "contact.position have to be a string";
        return false;
    }
    if (contactSection["voice"].is_string()) {
        voice = contactSection["voice"].string_value();
    } else if (! contactSection["voice"].is_null()) {
        errorMessage = "contact.voice have to be a string";
        return false;
    }
    if (contactSection["facsimile"].is_string()) {
        facsimile = contactSection["facsimile"].string_value();
    } else if (! contactSection["facsimile"].is_null()) {
        errorMessage = "contact.facsimile have to be a string";
        return false;
    }
    if (contactSection["address_type"].is_string()) {
        addressType = contactSection["address_type"].string_value();
    } else if (! contactSection["address_type"].is_null()) {
        errorMessage = "contact.address_type have to be a string";
        return false;
    }
    if (contactSection["delivery_point"].is_string()) {
        deliveryPoint = contactSection["delivery_point"].string_value();
    } else if (! contactSection["delivery_point"].is_null()) {
        errorMessage = "contact.delivery_point have to be a string";
        return false;
    }
    if (contactSection["city"].is_string()) {
        city = contactSection["city"].string_value();
    } else if (! contactSection["city"].is_null()) {
        errorMessage = "contact.city have to be a string";
        return false;
    }
    if (contactSection["administrative_area"].is_string()) {
        administrativeArea = contactSection["administrative_area"].string_value();
    } else if (! contactSection["administrative_area"].is_null()) {
        errorMessage = "contact.administrative_area have to be a string";
        return false;
    }
    if (contactSection["post_code"].is_string()) {
        postCode = contactSection["post_code"].string_value();
    } else if (! contactSection["post_code"].is_null()) {
        errorMessage = "contact.post_code have to be a string";
        return false;
    }
    if (contactSection["country"].is_string()) {
        country = contactSection["country"].string_value();
    } else if (! contactSection["country"].is_null()) {
        errorMessage = "contact.country have to be a string";
        return false;
    }
    if (contactSection["email"].is_string()) {
        electronicMailAddress = contactSection["email"].string_value();
    } else if (! contactSection["email"].is_null()) {
        errorMessage = "contact.email have to be a string";
        return false;
    }

    // ----------------------- Commun à plusieurs services 
    json11::Json commonSection = doc["common"];
    if (commonSection.is_object()) {
        
        if (commonSection["info_formats"].is_array()) {
            for (json11::Json info : commonSection["info_formats"].array_items()) {
                if (info.is_string()) {
                    infoFormatList.push_back ( info.string_value() );
                } else {
                    errorMessage = "common.info_formats have to be a string array";
                    return false;
                }
            }
        } else if (! commonSection["info_formats"].is_null()) {
            errorMessage = "common.info_formats have to be a string array";
            return false;
        }
        if (commonSection["styling"].is_bool()) {
            fullStyling = commonSection["styling"].bool_value();
        } else if (! commonSection["styling"].is_null()) {
            errorMessage = "common.styling have to be a boolean";
            return false;
        }
        if (commonSection["reprojection"].is_bool()) {
            reprojectionCapability = commonSection["reprojection"].bool_value();
        } else if (! commonSection["reprojection"].is_null()) {
            errorMessage = "common.reprojection have to be a boolean";
            return false;
        }
        if (commonSection["inspire"].is_bool()) {
            inspire = commonSection["inspire"].bool_value();
        } else if (! commonSection["inspire"].is_null()) {
            errorMessage = "common.inspire have to be a boolean";
            return false;
        }
        StyleBook::set_inspire(inspire);
        if (commonSection["post_mode"].is_bool()) {
            postMode = commonSection["post_mode"].bool_value();
        } else if (! commonSection["post_mode"].is_null()) {
            errorMessage = "common.post_mode have to be a boolean";
            return false;
        }
        if (commonSection["crs_restrictions"].is_string()) {
            std::string restritedCRSListfile = commonSection["crs_restrictions"].string_value();
            if (! loadRestrictedCRSList(restritedCRSListfile)) {
                return false;
            }
            dowerestrictCRSList = true;
        } else if (! commonSection["crs_restrictions"].is_null()) {
            errorMessage = "common.crs_restrictions have to be a string";
            return false;
        }
        if (commonSection["crs_equivalency"].is_string()) {
            std::string equalsCRSListfile = commonSection["crs_equivalency"].string_value();
            if (! loadEqualsCRSList(equalsCRSListfile)) {
                return false;
            }
            doweuselistofequalsCRS = true;
        } else if (! commonSection["crs_equivalency"].is_null()) {
            errorMessage = "common.crs_equivalency have to be a string";
            return false;
        }

    } else if (! commonSection.is_null()) {
        errorMessage = "common have to be an object";
        return false;
    }

    // ----------------------- WMS 
    json11::Json wmsSection = doc["wms"];
    if (wmsSection.is_null()) {
        supportWMS = false;
    } else if (! wmsSection.is_object()) {
        errorMessage = "wms have to be an object";
        return false;
    } else {

        if (wmsSection["active"].is_bool()) {
            supportWMS = wmsSection["active"].bool_value();
        } else if (! wmsSection["active"].is_null()) {
            errorMessage = "wms.active have to be a boolean";
            return false;
        }
        if (wmsSection["endpoint_uri"].is_string()) {
            wmsPublicUrl = wmsSection["endpoint_uri"].string_value();
        } else if (! wmsSection["endpoint_uri"].is_null()) {
            errorMessage = "wms.endpoint_uri have to be a string";
            return false;
        } else {
            wmsPublicUrl = "/wms";
        }
        if (wmsSection["name"].is_string()) {
            name = wmsSection["name"].string_value();
        } else if (! wmsSection["name"].is_null()) {
            errorMessage = "wms.name have to be a string";
            return false;
        }
        if (wmsSection["max_width"].is_number()) {
            maxWidth = wmsSection["max_width"].int_value();
        } else if (! wmsSection["max_width"].is_null()) {
            errorMessage = "wms.max_width have to be an integer";
            return false;
        }
        if (wmsSection["max_height"].is_number()) {
            maxHeight = wmsSection["max_height"].int_value();
        } else if (! wmsSection["max_height"].is_null()) {
            errorMessage = "wms.max_height have to be an integer";
            return false;
        }
        if (wmsSection["layer_limit"].is_number()) {
            layerLimit = wmsSection["layer_limit"].int_value();
        } else if (! wmsSection["layer_limit"].is_null()) {
            errorMessage = "wms.layer_limit have to be an integer";
            return false;
        }
        if (wmsSection["max_tile_x"].is_number()) {
            maxTileX = wmsSection["max_tile_x"].int_value();
        } else if (! wmsSection["max_tile_x"].is_null()) {
            errorMessage = "wms.max_tile_x have to be an integer";
            return false;
        }
        if (wmsSection["max_tile_y"].is_number()) {
            maxTileY = wmsSection["max_tile_y"].int_value();
        } else if (! wmsSection["max_tile_y"].is_null()) {
            errorMessage = "wms.max_tile_y have to be an integer";
            return false;
        }
        if (wmsSection["formats"].is_array()) {
            for (json11::Json f : wmsSection["formats"].array_items()) {
                if (f.is_string()) {
                    std::string format = f.string_value();
                    if ( format != "image/jpeg" &&
                        format != "image/png"  &&
                        format != "image/tiff" &&
                        format != "image/geotiff" &&
                        format != "image/x-bil;bits=32" &&
                        format != "image/gif" && 
                        format != "text/asc" ) {
                        errorMessage = "WMS image format [" + format + "] is not an handled MIME format";
                        return false;
                    } else {
                        formatList.push_back ( format );
                    }
                } else {
                    errorMessage = "wms.formats have to be a string array";
                    return false;
                }
            }
        } else if (! wmsSection["formats"].is_null()) {
            errorMessage = "wms.formats have to be a string array";
            return false;
        }

        // WMS Global CRS List
        bool isCRS84 = false;
        if (wmsSection["crs"].is_array()) {
            for (json11::Json c : wmsSection["crs"].array_items()) {
                if (c.is_string()) {
                    std::string crsStr = c.string_value();
                    if (! isCRSAllowed(crsStr)) {
                        BOOST_LOG_TRIVIAL(warning) <<  "Forbiden global CRS " << crsStr ;
                        continue;
                    }
                    if (isInGlobalCRSList(crsStr)) {
                        continue;
                    }

                    CRS* crs = new CRS( crsStr );
                    if ( ! crs->isDefine() ) {
                        BOOST_LOG_TRIVIAL(warning) << "The (WMS) CRS [" << crsStr <<"] is not present in PROJ"  ;
                        continue;
                    }

                    BOOST_LOG_TRIVIAL(info) <<  "Adding global CRS " << crs->getRequestCode()   ;
                    globalCRSList.push_back(crs);
                    if (crs->getRequestCode() == "CRS:84") {
                        isCRS84 = true;
                    }

                    if (doweuselistofequalsCRS) {
                        std::vector<CRS*> eqCRS = getEqualsCRS(crs->getRequestCode());
                        size_t init_size = globalCRSList.size();
                        for (unsigned int e = 0; e < eqCRS.size(); e++) {
                            bool already_in = false;
                            for ( int i = 0; i < init_size ; i++ ) {
                                if (globalCRSList.at( i )->cmpRequestCode(eqCRS.at(e)->getRequestCode() ) ){
                                    already_in = true;
                                    break;
                                }
                            }
                            if (! already_in) {
                                BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqCRS.at(e)->getRequestCode() <<"] of [" << crs->getRequestCode() << "]"  ;
                                // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
                                globalCRSList.push_back(new CRS(eqCRS.at(e)));
                                if (eqCRS.at(e)->getRequestCode() == "CRS:84") {
                                    isCRS84 = true;
                                }
                            }
                        }
                    }


                } else {
                    errorMessage = "wms.crs have to be a string array";
                    return false;
                }
            }
        } else if (! wmsSection["crs"].is_null()) {
            errorMessage = "wms.crs have to be a string array";
            return false;
        }
        // On veut forcément CRS:84 comme CRS global
        if (! isCRS84) {
            BOOST_LOG_TRIVIAL(info) <<  "CRS:84 not found -> adding global CRS CRS:84"   ;
            CRS* crs = new CRS( "CRS:84" );

            if ( ! crs->isDefine() ) {
                errorMessage = "The CRS [CRS:84] is not present in PROJ"  ;
                return false;
            }

            globalCRSList.push_back ( crs );

            if (doweuselistofequalsCRS) {
                std::vector<CRS*> eqCRS = getEqualsCRS(crs->getRequestCode());
                size_t init_size = globalCRSList.size();
                for (unsigned int e = 0; e < eqCRS.size(); e++) {
                    bool already_in = false;
                    for ( int i = 0; i < init_size ; i++ ) {
                        if (globalCRSList.at( i )->cmpRequestCode(eqCRS.at(e)->getRequestCode() ) ){
                            already_in = true;
                        }
                    }
                    if (! already_in) {
                        BOOST_LOG_TRIVIAL(info) <<  "Adding equivalent global CRS [" << eqCRS.at(e)->getRequestCode() <<"] of [CRS:84]"  ;
                        // On clone bien le CRS, pour ne pas avoir un conflit lors du nettoyage
                        globalCRSList.push_back(new CRS(eqCRS.at(e)));
                    }
                }
            }
        }

        // Métadonnée
        if (wmsSection["metadata"].is_object()) {
            mtdWMS = new MetadataURL ( wmsSection["metadata"] );
            if (mtdWMS->getMissingField() != "") {
                errorMessage = "Invalid WMS metadata: have to own a field " + mtdWMS->getMissingField();
                return false;
            }
        }
        
    }


    // ----------------------- WMTS 
    json11::Json wmtsSection = doc["wmts"];
    if (wmtsSection.is_null()) {
        supportWMTS = false;
    } else if (! wmtsSection.is_object()) {
        errorMessage = "wmts have to be an object";
        return false;
    } else {

        if (wmtsSection["active"].is_bool()) {
            supportWMTS = wmtsSection["active"].bool_value();
        } else if (! wmtsSection["active"].is_null()) {
            errorMessage = "wmts.active have to be a boolean";
            return false;
        }
        if (wmtsSection["endpoint_uri"].is_string()) {
            wmtsPublicUrl = wmtsSection["endpoint_uri"].string_value();
        } else if (! wmtsSection["endpoint_uri"].is_null()) {
            errorMessage = "wmts.endpoint_uri have to be a string";
            return false;
        } else {
            wmtsPublicUrl = "/wmts";
        }

        // Métadonnée
        if (wmtsSection["metadata"].is_object()) {
            mtdWMTS = new MetadataURL ( wmtsSection["metadata"] );
            if (mtdWMTS->getMissingField() != "") {
                errorMessage = "Invalid WMTS metadata: have to own a field " + mtdWMTS->getMissingField();
                return false;
            }
        }
        
    }

    // ----------------------- TMS 
    json11::Json tmsSection = doc["tms"];
    if (tmsSection.is_null()) {
        supportTMS = false;
    } else if (! tmsSection.is_object()) {
        errorMessage = "tms have to be an object";
        return false;
    } else {

        if (tmsSection["active"].is_bool()) {
            supportTMS = tmsSection["active"].bool_value();
        } else if (! tmsSection["active"].is_null()) {
            errorMessage = "tms.active have to be a boolean";
            return false;
        }
        if (tmsSection["endpoint_uri"].is_string()) {
            tmsPublicUrl = tmsSection["endpoint_uri"].string_value();
        } else if (! tmsSection["endpoint_uri"].is_null()) {
            errorMessage = "tms.endpoint_uri have to be a string";
            return false;
        } else {
            tmsPublicUrl = "/tms";
        }

        // Métadonnée
        if (tmsSection["metadata"].is_object()) {
            mtdTMS = new MetadataURL ( tmsSection["metadata"] );
            if (mtdTMS->getMissingField() != "") {
                errorMessage = "Invalid TMS metadata: have to own a field " + mtdTMS->getMissingField();
                return false;
            }
        }
        
    }

    // ----------------------- OGC 
    json11::Json ogcSection = doc["ogctiles"];
    if (ogcSection.is_null()) {
        supportOGCTILES = false;
    } else if (! ogcSection.is_object()) {
        errorMessage = "ogctiles have to be an object";
        return false;
    } else {

        if (ogcSection["active"].is_bool()) {
            supportOGCTILES = ogcSection["active"].bool_value();
        } else if (! ogcSection["active"].is_null()) {
            errorMessage = "ogctiles.active have to be a boolean";
            return false;
        }
        if (ogcSection["endpoint_uri"].is_string()) {
            ogctilesPublicUrl = ogcSection["endpoint_uri"].string_value();
        } else if (! ogcSection["endpoint_uri"].is_null()) {
            errorMessage = "ogctiles.endpoint_uri have to be a string";
            return false;
        } else {
            ogctilesPublicUrl = "/ogcapitiles";
        }
        
    }

    return true;
}

ServicesConf::ServicesConf(std::string path) : Configuration(path) {

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

bool ServicesConf::are_the_two_CRS_equal( std::string crs1, std::string crs2 ) {

    if (crs1 == crs2) {
        return true;
    }

    if (! doweuselistofequalsCRS) {
        return false;
    }

    for ( unsigned int l = 0; l < listofequalsCRS.size(); l++ ) {
        bool crs1found = false;
        bool crs2found = false;
        for ( unsigned int c = 0; c < listofequalsCRS.at(l).size(); c++ ) {
            if (listofequalsCRS.at(l).at(c)->cmpRequestCode(crs1)) {
                crs1found = true;
            }
            if (listofequalsCRS.at(l).at(c)->cmpRequestCode(crs2)) {
                crs2found = true;
            }
        }
        if (crs1found && crs2found) {return true;}
    }

    return false;
}

ServicesConf::~ServicesConf(){ 
    delete mtdWMS;
    delete mtdWMTS;
    delete mtdTMS;

    for ( unsigned int l = 0; l < listofequalsCRS.size(); l++ ) {
        for ( unsigned int c = 0; c < listofequalsCRS.at(l).size(); c++ ) {
            delete listofequalsCRS.at(l).at(c);
        }
    }

    for (unsigned int l = 0 ; l < restrictedCRSList.size() ; l++) {
        delete restrictedCRSList.at(l);
    }

    for (unsigned int l = 0 ; l < globalCRSList.size() ; l++) {
        delete globalCRSList.at(l);
    }

}

unsigned int ServicesConf::getMaxTileX() const { return maxTileX; }
unsigned int ServicesConf::getMaxTileY() const { return maxTileY; }
bool ServicesConf::isInFormatList(std::string f) {
    for ( unsigned int k = 0; k < formatList.size(); k++ ) {
        if ( formatList.at(k) == f ) {
            return true;
        }
    }
    return false;
}
bool ServicesConf::isInInfoFormatList(std::string f) {
    for ( unsigned int k = 0; k < infoFormatList.size(); k++ ) {
        if ( infoFormatList.at(k) == f ) {
            return true;
        }
    }
    return false;
}

bool ServicesConf::isInGlobalCRSList(CRS* c) {
    for ( unsigned int k = 0; k < globalCRSList.size(); k++ ) {
        if ( c->cmpRequestCode ( globalCRSList.at (k)->getRequestCode() ) ) {
            return true;
        }
    }
    return false;
}

bool ServicesConf::isInGlobalCRSList(std::string c) {
    for ( unsigned int k = 0; k < globalCRSList.size(); k++ ) {
        if ( globalCRSList.at (k)->cmpRequestCode ( c ) ) {
            return true;
        }
    }
    return false;
}


bool ServicesConf::isFullStyleCapable() { return fullStyling; }
bool ServicesConf::isInspire() {
    return inspire;
}

bool ServicesConf::getDoWeUseListOfEqualsCRS() { return doweuselistofequalsCRS; }
bool ServicesConf::getReprojectionCapability() { return reprojectionCapability; }
