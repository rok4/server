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
 * \file Request.cpp
 * \~french
 * \brief Implémentation de la classe Request, analysant les requêtes HTTP
 * \~english
 * \brief Implement the Request Class analysing HTTP requests
 */

#include "Request.h"
#include "Message.h"
#include <rok4/utils/CRS.h>
#include <rok4/utils/Pyramid.h>
#include <cstdlib>
#include <climits>
#include <vector>
#include <cstdio>
#include "UtilsXML.h"
#include "config.h"
#include <algorithm>
#include <regex>


namespace RequestType {

    const char* const requesttype_name[] = {
        "UNKNOWN",
        "MISSING",
        "GetServices",
        "GetCapabilities",
        "GetLayer",
        "GetLayerMetadata",
        "GetLayerGdal",
        "GetMap",
        "GetTile",
        "GetMapTile",
        "GetFeatureInfo",
        "GetVersion",
        "AddLayer",
        "UpdateLayer",
        "DeleteLayer",
        "BuildCapabilities",
        "TurnOn",
        "TurnOff",
        "GetStatus",
        "GetInfos"
        "GetThreads",
        "GetDepends"
    };

    std::string toString ( eRequestType rt ) {
        return std::string ( requesttype_name[rt] );
    }
}

namespace ServiceType {

    const char* const servicetype_name[] = {
        "MISSING",
        "UNKNOWN",
        "WMTS",
        "WMS",
        "TMS",
        "OGCTILES",
        "GLOBAL",
        "ADMIN",
        "HEALTHCHECK"
    };

    std::string toString ( eServiceType st ) {
        return std::string ( servicetype_name[st] );
    }
}

namespace TemplateOGC {

    // TODO [OGC] Route to obtain "tileMatrixSetLimits" and "metadata" of layers : 
    // ^/ogcapitiles/tiles/(.*)?
    // ^/ogcapitiles/map/tiles/(.*)?
    // ex. /ogcapitiles/tiles/LAMB93 or /ogcapitiles/map/tiles/PM

    const char* const regex[] = {
        "^/ogcapitiles/styles/(.*)/map/tiles/(.*)/(\\d{1,})/(\\d{1,})/(\\d{1,})(/info)?",
        "^/ogcapitiles/map/tiles/(.*)/(\\d{1,})/(\\d{1,})/(\\d{1,})(/info)?",
        "^/ogcapitiles/collections/(.*)/styles/(.*)/map/tiles/(.*)/(\\d{1,})/(\\d{1,})/(\\d{1,})(/info)?",
        "^/ogcapitiles/collections/(.*)/map/tiles/(.*)/(\\d{1,})/(\\d{1,})/(\\d{1,})(/info)?",
        "^/ogcapitiles/tiles/(.*)/(\\d{1,})/(\\d{1,})/(\\d{1,})(/info)?",
        "^/ogcapitiles/collections/(.*)/tiles/(.*)/(\\d{1,})/(\\d{1,})/(\\d{1,})(/info)?",
        "^/ogcapitiles/collections$",
        "^/ogcapitiles/collections/(.*)/map/tiles$",
        "^/ogcapitiles/collections/(.*)/tiles$",
        "^/ogcapitiles/tilematrixsets$",
        "^/ogcapitiles/tilematrixsets/(.*)$"
    };
    std::string toString ( eTemplateOGC r ) {
        return std::string ( regex[r] );
    }
}

/**
 * \~french
 * \brief Convertit un caractère héxadécimal (0-9, A-Z, a-z) en décimal
 * \param[in] hex caractère
 * \return 0xFF sur une entrée invalide
 * \~english
 * \brief Converts hex char (0-9, A-Z, a-z) to decimal.
 * \param[in] hex character
 * \return 0xFF on invalid input.
 */
char hex2int ( unsigned char hex ) {
    hex = hex - '0';
    // Si hex <= 9 on a le résultat
    //   Sinon
    if ( hex > 9 ) {
        hex = ( hex + '0' - 1 ) | 0x20; // Pour le passage des majuscules aux minuscules dans la table ASCII
        hex = hex - 'a' + 11;
    }
    if ( hex > 15 ) // En cas d'erreur
        hex = 0xFF;

    return hex;
}

void Request::url_decode ( char *src ) {
    unsigned char high, low;
    char* dst = src;

    while ( ( *src ) != '\0' ) {
        if ( *src == '+' ) {
            *dst = ' ';
        } else if ( *src == '%' ) {
            *dst = '%';

            high = hex2int ( * ( src + 1 ) );
            if ( high != 0xFF ) {
                low = hex2int ( * ( src + 2 ) );
                if ( low != 0xFF ) {
                    high = ( high << 4 ) | low;

                    /* map control-characters out */
                    if ( high < 32 || high == 127 ) high = '_';

                    *dst = high;
                    src += 2;
                }
            }
        } else {
            *dst = *src;
        }

        dst++;
        src++;
    }

    *dst = '\0';
}

/**
 * \~french
 * \brief Supprime l'espace de nom (la partie avant :) de la balise XML
 * \param[in,out] elementName le nom de la balise
 * \~english
 * \brief Remove the namespace (before :) in the XML element
 * \param[in,out] elementName the element name
 */
void removeNameSpace ( std::string& elementName ) {
    size_t pos = elementName.find ( ":" );
    if ( elementName.size() <= pos ) {
        return;
    }
    // Garde le ":" -> "left:right" devient ":right"
    elementName.erase ( elementName.begin(),elementName.begin() +pos );
}

/**
 * \~french
 * \brief Analyse des paramètres d'une requête GetCapabilities en POST XML
 * \param[in] hGetCap élement XML de la requête
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a GetCapabilities request in POST XML
 * \param[in] hGetCap request XML element
 * \param[in,out] parameters associative parameters list
 */
void parseGetCapabilitiesPost ( TiXmlHandle& hGetCap, std::map< std::string, std::string >& parameters ) {
    BOOST_LOG_TRIVIAL(debug) <<   "Parse GetCapabilities Request" ;
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "getcapabilities" ) );

    TiXmlElement* pElem = hGetCap.ToElement();

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No service attribute" ;
        return;
    }

    std::transform ( service.begin(), service.end(), service.begin(), tolower );

    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No version attribute" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

}
/**
 * \~french
 * \brief Analyse des paramètres d'une requête GetTile en POST XML
 * \param[in] hGetTile élement XML de la requête
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a GetTile request in POST XML
 * \param[in] hGetTile request XML element
 * \param[in,out] parameters associative parameters list
 */
void parseGetTilePost ( TiXmlHandle& hGetTile, std::map< std::string, std::string >& parameters ) {
    BOOST_LOG_TRIVIAL(debug) <<   "Parse GetTile Request" ;
    TiXmlElement* pElem = hGetTile.ToElement();
    std::string version;
    std::string service;

    parameters.insert ( std::pair<std::string, std::string> ( "request", "gettile" ) );

    if ( pElem->QueryStringAttribute ( "service",&service ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No service attribute" ;
        return;
    }
    std::transform ( service.begin(), service.end(), service.begin(), tolower );
    parameters.insert ( std::pair<std::string, std::string> ( "service", service ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No version attribute" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );
    //Layer
    pElem = hGetTile.FirstChildElement().ToElement();

    if ( !pElem && pElem->ValueStr().find ( "Layer" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No Layer" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "layer", UtilsXML::getTextStrFromElem(pElem) ) );

    //Style
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Style" ) == std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No Style" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "style", UtilsXML::getTextStrFromElem(pElem) ) );

    //Format
    pElem = pElem->NextSiblingElement();

    if ( !pElem && pElem->ValueStr().find ( "Format" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No Format" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "format", UtilsXML::getTextStrFromElem(pElem) ) );

    //DimensionNameValue name="NAME" OPTIONAL
    pElem = pElem->NextSiblingElement();
    while ( pElem && pElem->ValueStr().find ( "DimensionNameValue" ) !=std::string::npos && pElem->GetText() ) {
        std::string dimensionName;
        if ( pElem->QueryStringAttribute ( "name",&dimensionName ) != TIXML_SUCCESS ) {
            BOOST_LOG_TRIVIAL(debug) <<   "No Name attribute" ;
            continue;
        } else {
            parameters.insert ( std::pair<std::string, std::string> ( dimensionName, UtilsXML::getTextStrFromElem(pElem) ) );
        }
        pElem = pElem->NextSiblingElement();
    }

    //TileMatrixSet
    if ( !pElem && pElem->ValueStr().find ( "TileMatrixSet" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No TileMatrixSet" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrixset", UtilsXML::getTextStrFromElem(pElem) ) );

    //TileMatrix
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileMatrix" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No TileMatrix" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilematrix", UtilsXML::getTextStrFromElem(pElem) ) );

    //TileRow
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileRow" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No TileRow" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilerow", UtilsXML::getTextStrFromElem(pElem) ) );
    //TileCol
    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "TileCol" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No TileCol" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "tilecol", UtilsXML::getTextStrFromElem(pElem) ) );

    pElem = pElem->NextSiblingElement();
    // "VendorOption"
    while ( pElem ) {
        if ( pElem->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElem->Attribute ( "name" ) ) {
            BOOST_LOG_TRIVIAL(debug) <<   "VendorOption" ;
            std::string vendorOpt = pElem->Attribute ( "name" );
            std::transform ( vendorOpt.begin(), vendorOpt.end(), vendorOpt.begin(), ::tolower );
            parameters.insert ( std::pair<std::string, std::string> ( vendorOpt, ( pElem->GetText() ?UtilsXML::getTextStrFromElem(pElem) :"true" ) ) );
        }
        pElem =  pElem->NextSiblingElement();
    }

    pElem = pElem->NextSiblingElement();
    // "VendorOption"
    while ( pElem ) {
        if ( pElem->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElem->Attribute ( "name" ) ) {
            BOOST_LOG_TRIVIAL(debug) <<  "VendorOption" ;
            std::string vendorOpt = pElem->Attribute ( "name" );
            std::transform ( vendorOpt.begin(), vendorOpt.end(), vendorOpt.begin(), ::tolower );
            parameters.insert ( std::pair<std::string, std::string> ( vendorOpt, ( pElem->GetText() ?pElem->GetText() :"true" ) ) );
        }
        pElem =  pElem->NextSiblingElement();
    }

}
/**
 * \~french
 * \brief Analyse des paramètres d'une requête GetMap en POST XML
 * \param[in] hGetMap élement XML de la requête
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a GetMap request in POST XML
 * \param[in] hGetMap request XML element
 * \param[in,out] parameters associative parameters list
 */
void parseGetMapPost ( TiXmlHandle& hGetMap, std::map< std::string, std::string >& parameters ) {
    BOOST_LOG_TRIVIAL(debug) <<   "Parse GetMap Request" ;
    TiXmlElement* pElem = hGetMap.ToElement();
    std::string version;
    std::string sldVersion;
    std::string layers;
    std::string styles;
    std::string bbox;
    std::stringstream format_options;

    parameters.insert ( std::pair<std::string, std::string> ( "service", "wms" ) );
    parameters.insert ( std::pair<std::string, std::string> ( "request", "getmap" ) );

    if ( pElem->QueryStringAttribute ( "version",&version ) != TIXML_SUCCESS ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No version attribute" ;
        return;
    }
    parameters.insert ( std::pair<std::string, std::string> ( "version", version ) );

    //StyledLayerDescriptor Layers and Style

    pElem=hGetMap.FirstChildElement().ToElement();
    if ( !pElem && pElem->ValueStr().find ( "StyledLayerDescriptor" ) ==std::string::npos ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No StyledLayerDescriptor" ;
        return;
    }
    pElem=hGetMap.FirstChildElement().FirstChildElement().ToElement();
    if ( !pElem ) {
        BOOST_LOG_TRIVIAL(debug) <<   "Empty StyledLayerDescriptor" ;
        return;
    }

    //NamedLayer
    while ( pElem ) {

        //NamedLayer test
        if ( pElem->ValueStr().find ( "NamedLayer" ) ==std::string::npos ) {

            pElem= pElem->NextSiblingElement();
            continue;
        }

        TiXmlElement* pElemNamedLayer = pElem->FirstChildElement();
        if ( !pElemNamedLayer && pElemNamedLayer->ValueStr().find ( "Name" ) ==std::string::npos && !pElemNamedLayer->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<   "NamedLayer without Name" ;
            return;
        }
        if ( layers.size() >0 ) {
            layers.append ( "," );
            styles.append ( "," );
        }
        layers.append ( pElemNamedLayer->GetText() );


        while ( pElemNamedLayer->ValueStr().find ( "NamedStyle" ) ==std::string::npos ) {
            pElemNamedLayer = pElemNamedLayer->NextSiblingElement();
            if ( !pElemNamedLayer ) {
                BOOST_LOG_TRIVIAL(debug) <<   "NamedLayer without NamedStyle, use default style" ;
                break;
            }
        }
        pElemNamedLayer = pElemNamedLayer->FirstChildElement();
        if ( !pElemNamedLayer && pElemNamedLayer->ValueStr().find ( "Name" ) ==std::string::npos && !pElemNamedLayer->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<   "NamedStyle without Name" ;
            return;
        } else {
            styles.append ( pElemNamedLayer->GetText() );
        }
        pElem= pElem->NextSiblingElement ( pElem->ValueStr() );

    }
    if ( layers.size() <=0 ) {
        BOOST_LOG_TRIVIAL(debug) <<   "StyledLayerDescriptor without NamedLayer" ;
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "layers", layers ) );
    parameters.insert ( std::pair<std::string, std::string> ( "styles", styles ) );

    //CRS
    pElem=hGetMap.FirstChildElement().ToElement()->NextSiblingElement();
    ;
    if ( !pElem && pElem->ValueStr().find ( "CRS" ) ==std::string::npos && !pElem->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No CRS" ;
        return;
    }

    parameters.insert ( std::pair<std::string, std::string> ( "crs", UtilsXML::getTextStrFromElem(pElem) ) );

    //BoundingBox

    pElem = pElem->NextSiblingElement();
    if ( !pElem && pElem->ValueStr().find ( "BoundingBox" ) ==std::string::npos ) {
        BOOST_LOG_TRIVIAL(debug) <<   "No BoundingBox" ;
        return;
    }

    //FIXME crs attribute might be different from CRS in the request
    //eg output image in EPSG:3857 BBox defined in EPSG:4326
    TiXmlElement * pElemBBox = pElem->FirstChildElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "LowerCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "BoundingBox Invalid" ;
        return;
    }

    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );
    bbox.append ( "," );

    pElemBBox = pElemBBox->NextSiblingElement();
    if ( !pElemBBox && pElemBBox->ValueStr().find ( "UpperCorner" ) ==std::string::npos && !pElemBBox->GetText() ) {
        BOOST_LOG_TRIVIAL(debug) <<   "BoundingBox Invalid" ;
        return;
    }
    bbox.append ( pElemBBox->GetText() );
    bbox.replace ( bbox.find ( " " ),1,"," );

    parameters.insert ( std::pair<std::string, std::string> ( "bbox", bbox ) );

    //Output
    {
        pElem = pElem->NextSiblingElement();
        if ( !pElem && pElem->ValueStr().find ( "Output" ) ==std::string::npos ) {
            BOOST_LOG_TRIVIAL(debug) <<   "No Output" ;
            return;
        }
        TiXmlElement * pElemOut =  pElem->FirstChildElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Size" ) ==std::string::npos ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Output Invalid no Size" ;
            return;
        }
        TiXmlElement * pElemOutTmp =  pElemOut->FirstChildElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Width" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Output Invalid, Width incorrect" ;
            return;
        }
        parameters.insert ( std::pair<std::string, std::string> ( "width", pElemOutTmp->GetText() ) );

        pElemOutTmp =  pElemOutTmp->NextSiblingElement();
        if ( !pElemOutTmp && pElemOutTmp->ValueStr().find ( "Height" ) ==std::string::npos && !pElemOutTmp->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<  "Output Invalid, Height incorrect" ;
            return;
        }
        parameters.insert ( std::pair<std::string, std::string> ( "height", pElemOutTmp->GetText() ) );

        pElemOut =  pElemOut->NextSiblingElement();
        if ( !pElemOut && pElemOut->ValueStr().find ( "Format" ) ==std::string::npos && !pElemOut->GetText() ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Output Invalid no Format" ;
            return;
        }

        parameters.insert ( std::pair<std::string, std::string> ( "format", pElemOut->GetText() ) );

        pElemOut =  pElemOut->NextSiblingElement();

        if ( pElemOut ) {

            if ( pElemOut->ValueStr().find ( "Transparent" ) !=std::string::npos && pElemOut->GetText() ) {

                parameters.insert ( std::pair<std::string, std::string> ( "transparent", pElemOut->GetText() ) );
                pElemOut =  pElemOut->NextSiblingElement();
            }

        }
        if ( pElemOut ) {
            if ( pElemOut->ValueStr().find ( "BGcolor" ) !=std::string::npos && pElemOut->GetText() ) {

                parameters.insert ( std::pair<std::string, std::string> ( "bgcolor", pElemOut->GetText() ) );
                pElemOut =  pElemOut->NextSiblingElement();
            }
        }
        // "VendorOption"
        while ( pElemOut ) {
            if ( pElemOut->ValueStr().find ( "VendorOption" ) !=std::string::npos && pElemOut->Attribute ( "name" ) && pElemOut->GetText() ) {
                if ( format_options.str().size() >0 ) {
                    format_options << ";";
                }
                format_options <<  pElemOut->Attribute ( "name" ) << ":"<< pElemOut->GetText();

            }
            pElemOut =  pElemOut->NextSiblingElement();
        }

    }

    // Handle output format options
    if ( format_options.str().size() > 0 ) {
        parameters.insert ( std::pair<std::string, std::string> ( "format_options", format_options.str() ) );
    }

    //OPTIONAL

    //Exceptions
    pElem = pElem->NextSiblingElement();
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Exceptions" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "exceptions", UtilsXML::getTextStrFromElem(pElem) ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Time
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Time" ) !=std::string::npos && pElem->GetText() ) {
            parameters.insert ( std::pair<std::string, std::string> ( "time", UtilsXML::getTextStrFromElem(pElem) ) );
            pElem = pElem->NextSiblingElement();
        }
    }

    //Elevation
    if ( pElem ) {
        if ( pElem->ValueStr().find ( "Elevation" ) !=std::string::npos ) {
            pElem = pElem->FirstChildElement();
            if ( !pElem ) {
                BOOST_LOG_TRIVIAL(debug) <<   "Elevation incorrect" ;
                return;
            }
            std::string elevation;
            if ( pElem->ValueStr().find ( "Interval" ) !=std::string::npos ) {
                pElem = pElem->FirstChildElement();
                if ( !pElem && pElem->ValueStr().find ( "Min" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( UtilsXML::getTextStrFromElem(pElem) );
                elevation.append ( "/" );
                pElem = pElem->NextSiblingElement();
                if ( !pElem && pElem->ValueStr().find ( "Max" ) ==std::string::npos && !pElem->GetText() ) {
                    return;
                }
                elevation.append ( UtilsXML::getTextStrFromElem(pElem) );

            } else if ( pElem->ValueStr().find ( "Value" ) !=std::string::npos ) {
                while ( pElem && pElem->GetText() ) {
                    if ( elevation.size() >0 ) {
                        elevation.append ( "," );
                    }
                    elevation.append ( UtilsXML::getTextStrFromElem(pElem) );
                    pElem = pElem->NextSiblingElement ( pElem->ValueStr() );
                }
            }
            parameters.insert ( std::pair<std::string, std::string> ( "elevation", elevation ) );
        }
    }

}

/**
 * \~french
 * \brief Analyse des paramètres d'une requête POST XML
 * \param[in] content contenu de la requête POST
 * \param[in,out] parameters liste associative des paramètres
 * \~english
 * \brief Parse a POST XML request
 * \param[in] content POST request content
 * \param[in,out] parameters associative parameters list
 */
void parsePostContent ( std::string content, std::map< std::string, std::string >& parameters ) {
    TiXmlDocument doc ( "request" );
    if ( !doc.Parse ( content.c_str() ) ) {
        BOOST_LOG_TRIVIAL(info) <<   "POST request with invalid content" ;
        return;
    }
    TiXmlElement *rootEl = doc.FirstChildElement();
    if ( !rootEl ) {
        BOOST_LOG_TRIVIAL(info) <<   "Cannot retrieve XML root element" ;
        return;
    }
    //TODO Unfold Soap envelope
    std::string value = rootEl->ValueStr();
    //  removeNameSpace(value);
    TiXmlHandle hRoot ( rootEl );

    if ( value.find ( "GetCapabilities" ) !=std::string::npos ) {
        parseGetCapabilitiesPost ( hRoot,parameters );
    } else if ( value.find ( "GetMap" ) !=std::string::npos ) {
        parseGetMapPost ( hRoot,parameters );
    } else if ( value.find ( "GetTile" ) !=std::string::npos ) {
        parseGetTilePost ( hRoot,parameters );
    }
    // Administration
    else if ( value.find ( "layer" ) != std::string::npos ) {
        parseGetTilePost ( hRoot,parameters );
    }

}

void Request::determineServiceAndRequest() {
    if (pathParts.size() == 0) {
        service = ServiceType::GLOBAL;
        request = RequestType::GETSERVICES;
        return;
    }

    BOOST_LOG_TRIVIAL(debug) <<  "First part of path :" << pathParts.at(0);

    // ************************ WMS
    if (pathParts.at(0) == "wms" || pathParts.at(0) == "ows") {
        if (method != "POST" && method != "GET") {
            return;
        }

        if (method == "POST" && body != "") {
            parsePostContent(body, bodyParams);
        }

        // Service
        std::string param_service = getParam("service");
        std::transform(param_service.begin(), param_service.end(), param_service.begin(), ::tolower);

        if (param_service == "wms") {
            service = ServiceType::WMS;
        } else {
            return;
        }

        // Requête
        std::string param_request = getParam("request");
        std::transform(param_request.begin(), param_request.end(), param_request.begin(), ::tolower);
        
        if (param_request == "getcapabilities") {
            request = RequestType::GETCAPABILITIES;
        } else if (param_request == "getfeatureinfo") {
            request = RequestType::GETFEATUREINFO;
        } else if (param_request == "getmap") {
            request = RequestType::GETMAP;
        } else if (param_request != "") {
            request = RequestType::REQUEST_UNKNOWN;
        }
    }

    // ************************ WMTS
    else if (pathParts.at(0) == "wmts" || pathParts.at(0) == "ows") {
        if (method != "POST" && method != "GET") {
            return;
        }

        if (method == "POST" && body != "") {
            parsePostContent(body, bodyParams);
        }

        // Service
        std::string param_service = getParam("service");
        std::transform(param_service.begin(), param_service.end(), param_service.begin(), ::tolower);

        if (param_service == "wmts") {
            service = ServiceType::WMTS;
        } else {
            return;
        }

        // Requête
        std::string param_request = getParam("request");
        std::transform(param_request.begin(), param_request.end(), param_request.begin(), ::tolower);

        if (param_request == "getcapabilities") {
            request = RequestType::GETCAPABILITIES;
        } else if (param_request == "getfeatureinfo") {
            request = RequestType::GETFEATUREINFO;
        } else if (param_request == "gettile") {
            request = RequestType::GETTILE;
        } else if (param_request != "") {
            request = RequestType::REQUEST_UNKNOWN;
        }
    }

    // ************************ TMS
    else if (pathParts.at(0) == "tms") {

        if (method != "GET") {
            return;
        }

        // Service
        service = ServiceType::TMS;

        // Requête
        if (pathParts.size() < 2) {
            request = RequestType::REQUEST_UNKNOWN;
            return;
        }

        if (pathParts.size() == 2) {
            //-->  /tms/1.0.0
            request = RequestType::GETCAPABILITIES;
        }

        else if (pathParts.size() == 3) {
            //-->  /tms/1.0.0/{layer}
            request = RequestType::GETLAYER;
        }

        else if (pathParts.size() == 4 && pathParts.back() == "metadata.json") {
            //-->  /tms/1.0.0/{layer}/metadata.json
            request = RequestType::GETLAYERMETADATA;
        }

        else if (pathParts.size() == 4 && pathParts.back() == "gdal.xml") {
            //-->  /tms/1.0.0/{layer}/gdal.xml
            request = RequestType::GETLAYERGDAL;
        }

        else if (pathParts.size() == 6) {
            //-->  /tms/1.0.0/{layer}/{level}/{x}/{y}.{ext}
            request = RequestType::GETTILE;
        } else {
            // La profondeur de requête ne permet pas de savoir l'action demandée -> ERREUR
            request = RequestType::REQUEST_UNKNOWN;
        }
    }

    // ************************ OGC
    else if (pathParts.at(0) == "ogcapitiles") {

        if (method != "GET") {
            return;
        }

        // Service
        service = ServiceType::OGCTILES;

        // Requête
        // - GetTile Raster & Vector
        // - GetFeatureInfo
        // - GetCapabilities

        if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILERASTERSTYLED)))) {
            tmpl = TemplateOGC::GETTILERASTERSTYLED;
            request = RequestType::GETMAPTILE;
            if (pathParts.size() == 10) {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILERASTER)))) {
            tmpl = TemplateOGC::GETTILERASTER;
            request = RequestType::GETMAPTILE;
            if (pathParts.size() == 8) {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILERASTERSTYLEDBYCOLLECTION)))) {
            tmpl = TemplateOGC::GETTILERASTERSTYLEDBYCOLLECTION;
            request = RequestType::GETMAPTILE;
            if (pathParts.size() == 12) {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILERASTERBYCOLLECTION)))) {
            tmpl = TemplateOGC::GETTILERASTERBYCOLLECTION;
            request = RequestType::GETMAPTILE;
            if (pathParts.size() == 10) {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILEVECTOR)))) {
            tmpl = TemplateOGC::GETTILEVECTOR;
            request = RequestType::GETTILE;
            if (pathParts.size() == 7) {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILEVECTORBYCOLLECTION)))) {
            tmpl = TemplateOGC::GETTILEVECTORBYCOLLECTION;
            request = RequestType::GETTILE;
            if (pathParts.size() == 9) {
                request = RequestType::GETFEATUREINFO;
            }
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETCAPABILITIESBYCOLLECTION)))) {
            tmpl = TemplateOGC::GETCAPABILITIESBYCOLLECTION;
            request = RequestType::GETCAPABILITIES;
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETCAPABILITIESRASTERBYCOLLECTION)))) {
            tmpl = TemplateOGC::GETCAPABILITIESRASTERBYCOLLECTION;
            request = RequestType::GETCAPABILITIES;
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETCAPABILITIESVECTORBYCOLLECTION)))) {
            tmpl = TemplateOGC::GETCAPABILITIESVECTORBYCOLLECTION;
            request = RequestType::GETCAPABILITIES;
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILEMATRIXSET)))) {
            tmpl = TemplateOGC::GETTILEMATRIXSET;
            request = RequestType::GETCAPABILITIES;
        }
        else if (std::regex_match(path, std::regex(TemplateOGC::toString(TemplateOGC::GETTILEMATRIXSETBYID)))) {
            tmpl = TemplateOGC::GETTILEMATRIXSETBYID;
            request = RequestType::GETCAPABILITIES;
        }
        else {
            request = RequestType::REQUEST_UNKNOWN;
        }
    }

    // ************************ ADMIN
    else if (pathParts.at(0) == "admin") {
        // Service
        service = ServiceType::ADMIN;

        // Requête
        if (method == "POST" && pathParts.size() == 3 && pathParts.at(1) == "layers") {
            //--> POST /admin/layers/{layername}
            request = RequestType::ADDLAYER;
        }

        else if (method == "PUT" && pathParts.size() == 2 && pathParts.at(1) == "layers") {
            //--> PUT /admin/layers
            request = RequestType::BUILDCAPABILITIES;
        }

        else if (method == "PUT" && pathParts.size() == 3 && pathParts.at(1) == "layers") {
            //--> PUT /admin/layers/{layername}
            request = RequestType::UPDATELAYER;
        }

        else if (method == "DELETE" && pathParts.size() == 3 && pathParts.at(1) == "layers") {
            //--> DELETE /admin/layers/{layername}
            request = RequestType::DELETELAYER;

        }

        else if (method == "PUT" && pathParts.size() == 2 && pathParts.at(1) == "on") {
            //--> PUT /admin/on
            request = RequestType::TURNON;

        }

        else if (method == "PUT" && pathParts.size() == 2 && pathParts.at(1) == "off") {
            //--> PUT /admin/off
            request = RequestType::TURNOFF;

        } else {
            // La profondeur de requête ne permet pas de savoir l'action demandée -> ERREUR
            request = RequestType::REQUEST_UNKNOWN;
        }
    }
    
    // ************************ HEALTHCHECK
    else if (pathParts.at(0) == "healthcheck") {
        // Uniquement du GET !
        if (method != "GET") {
            return;
        }

        // Service
        service = ServiceType::HEALTHCHECK;
        // Requête

        if (pathParts.size() == 1) {
            //--> GET /healthcheck/
            request = RequestType::GETHEALTHSTATUS;
        }
        else if (pathParts.size() == 2 && pathParts.at(1) == "info") {
            //--> GET /healthcheck/threads
            request = RequestType::GETINFOSTATUS;
        }
        else if (pathParts.size() == 2 && pathParts.at(1) == "threads") {
            //--> GET /healthcheck/threads
            request = RequestType::GETTHREADSTATUS;
        }
        else if (pathParts.size() == 2 && pathParts.at(1) == "depends") {
            //--> GET /healthcheck/depends
            request = RequestType::GETDEPENDSTATUS;
        } else {
            // La profondeur de requête ne permet pas de savoir l'action demandée -> ERREUR
            request = RequestType::REQUEST_UNKNOWN;
        }
    }
    else {
        service = ServiceType::SERVICE_UNKNOWN;
    }
}

Request::Request ( FCGX_Request& fcgxRequest ) : service(ServiceType::SERVICE_MISSING), request(RequestType::REQUEST_MISSING) {
    
    // Méthode
    method = std::string(FCGX_GetParam ( "REQUEST_METHOD",fcgxRequest.envp ));

    // Body
    if (method == "POST" || method == "PUT") {
        char* contentBuffer = ( char* ) malloc ( sizeof ( char ) *200 );
        while ( FCGX_GetLine ( contentBuffer, 200, fcgxRequest.in ) ) {
            body.append ( contentBuffer );
        }
        free ( contentBuffer );
        contentBuffer = NULL;
        body.append("\n");
        BOOST_LOG_TRIVIAL(debug) <<  "Request Content :" << std::endl << body ;
    }

    // Chemin
    path = std::string(FCGX_GetParam ( "SCRIPT_NAME",fcgxRequest.envp ));
    // Suppression du slash final
    if (path.compare ( path.size()-1,1,"/" ) == 0) {
        path.pop_back();
    }

    std::stringstream ss(path);
    std::string token;
    char delim = '/';
    std::vector<std::string> tmp;
    while (std::getline(ss, token, delim)) {
        tmp.push_back(token);
    }
    // Suppression des éléments vide (slash de début, de fin, ou double slash)
    for (int i = 0; i < tmp.size(); i++) {
        if (tmp.at(i) != "") {
            pathParts.push_back(tmp.at(i));
        }
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Request: " << method << " " << path ;

    // Query parameters
    char* query = FCGX_GetParam ( "QUERY_STRING",fcgxRequest.envp );
    url_decode ( query );
    BOOST_LOG_TRIVIAL(debug) <<  "Query parameters: " << query ;
    for ( int pos = 0; query[pos]; ) {
        char* key = query + pos;
        for ( ; query[pos] && query[pos] != '=' && query[pos] != '&'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = query + pos;
        for ( ; query[pos] && query[pos] != '&'; pos++ ); // on trouve le suivant "&" ou 0
        if ( *value == '=' ) *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if ( query[pos] ) query[pos++] = 0; // on met un 0 à la fin du char* value

        toLowerCase ( key );
        queryParams.insert ( std::pair<std::string, std::string> ( key, value ) );
    }

    determineServiceAndRequest();
}


Request::~Request() {}

bool Request::hasParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = queryParams.find ( paramName );
    if ( it == queryParams.end() ) {
        it = bodyParams.find ( paramName );
        if ( it == bodyParams.end() ) {
            return false;
        }
    }
    return true;
}

std::string Request::getParam ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = queryParams.find ( paramName );
    if ( it == queryParams.end() ) {
        it = bodyParams.find ( paramName );
        if ( it == bodyParams.end() ) {
            return "";
        }
    }
    return it->second;
}
    
