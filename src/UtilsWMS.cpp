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
 * \file UtilsWMS.cpp
 * \~french
 * \brief Implémentation des fonctions de générations des GetCapabilities
 * \~english
 * \brief Implement the GetCapabilities generation function
 */

#include "Rok4Server.h"
#include "UtilsXML.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>
#include <cmath>
#include <rok4/utils/TileMatrixSet.h>
#include <rok4/utils/Pyramid.h>
#include <rok4/utils/BoundingBox.h>
#include <rok4/utils/Utils.h>

int Rok4Server::GetDecimalPlaces ( double dbVal ) {
    dbVal = fmod(dbVal, 1);
    static const int MAX_DP = 10;
    double THRES = pow ( 0.1, MAX_DP );
    if ( dbVal == 0.0 )
        return 0;
    int nDecimal = 0;
    while ( dbVal - floor ( dbVal ) > THRES && nDecimal < MAX_DP && ceil(dbVal)-dbVal > THRES) {
        dbVal *= 10.0;
        THRES *= 10.0;
        nDecimal++;
    }
    return nDecimal;
}

DataStream* Rok4Server::getMapParamWMS (
    Request* request, std::vector<Layer*>& layers, BoundingBox< double >& bbox, int& width, int& height, CRS*& crs,
    std::string& format, std::vector<Style*>& styles, std::map< std::string, std::string >& format_option,int& dpi
) {
    // VERSION
    std::string version = request->getParam ( "version" );
    if ( version == "" ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre VERSION absent.","wms" ) );
    }

    if ( version != "1.3.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.3.0 disponible seulement))","wms" ) );

    // LAYERS
    std::string str_layers = request->getParam ( "layers" );
    if ( str_layers == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYERS absent.","wms" ) );
    
    //Split layer Element
    std::vector<std::string> vector_layers = split ( str_layers,',' );
    BOOST_LOG_TRIVIAL(debug) <<  "Nombre de couches demandees =" << vector_layers.size()  ;

    if ( vector_layers.size() > servicesConf->layerLimit ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le nombre de couche demande excede la valeur du LayerLimit.","wms" ) );
    }

    for (unsigned int i = 0 ; i < vector_layers.size(); i++ ) {
        if ( containForbiddenChars(vector_layers.at(i)) ) {
            // On a détecté un caractère interdit, on ne met pas le layer fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS layers: " << vector_layers.at(i) ;
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,"Layer inconnu.","wms" ) );
        }

        Layer* lay = serverConf->getLayer(vector_layers.at(i));
        if ( lay == NULL || ! lay->getWMSAuthorized())
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,"Layer " +vector_layers.at(i)+" inconnu.","wms" ) );
       
        layers.push_back ( lay );
    }
    BOOST_LOG_TRIVIAL(debug) <<  "Nombre de couches =" << layers.size()  ;

    // WIDTH
    std::string strWidth = request->getParam ( "width" );
    if ( strWidth == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre WIDTH absent.","wms" ) );
    width=atoi ( strWidth.c_str() );
    if ( width == 0 || width == INT_MAX || width == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH n'est pas une valeur entiere.","wms" ) );
    if ( width < 0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH est negative.","wms" ) );
    if ( width > servicesConf->maxWidth )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre WIDTH est superieure a la valeur maximum autorisee par le service.","wms" ) );

    // HEIGHT
    std::string strHeight = request->getParam ( "height" );
    if ( strHeight == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre HEIGHT absent.","wms" ) );
    height=atoi ( strHeight.c_str() );
    if ( height == 0 || height == INT_MAX || height == INT_MIN )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT n'est pas une valeur entiere.","wms" ) ) ;
    if ( height<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT est negative.","wms" ) );
    if ( height>servicesConf->maxHeight )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre HEIGHT est superieure a la valeur maximum autorisee par le service.","wms" ) );
    
    // CRS
    std::string str_crs = request->getParam ( "crs" );
    if ( str_crs == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre CRS absent.","wms" ) );


    if ( containForbiddenChars(str_crs) ) {
        // On a détecté un caractère interdit, on ne met pas le crs fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS crs: " << str_crs ;
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_CRS,"CRS  inconnu","wms" ) );
    }
   
    // Existence du CRS dans la liste globale de CRS ou de chaque layers
    if (! servicesConf->isInGlobalCRSList(str_crs) ) {
        for ( unsigned int j = 0; j < layers.size() ; j++ ) {
            if (! layers.at ( j )->isInWMSCRSList(str_crs) )
                return new SERDataStream ( new ServiceException ( "",WMS_INVALID_CRS,"CRS " +str_crs+" inconnu pour le layer " +vector_layers.at ( j ) +".","wms" ) );
        }
    }
    crs = new CRS( str_crs );

    // FORMAT
    format = request->getParam ( "format" );
    if ( format == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wms" ) );

    if ( containForbiddenChars(format) ) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS format: " << format ;
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,"Format non gere par le service.","wms" ) );
    }

    if ( ! servicesConf->isInFormatList(format) )
        return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,"Format " +format+" non gere par le service.","wms" ) );

    // BBOX
    std::string strBbox = request->getParam ( "bbox" );
    if ( strBbox == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre BBOX absent.","wms" ) );
    std::vector<std::string> coords = split ( strBbox,',' );

    if ( coords.size() != 4 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );
    double bb[4];
    for ( int i = 0; i < 4; i++ ) {
        if ( sscanf ( coords[i].c_str(),"%lf",&bb[i] ) !=1 )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );
        //Test NaN values
        if (bb[i]!=bb[i])
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );
    }
    if ( bb[0] >= bb[2] || bb[1] >= bb[3] )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Parametre BBOX incorrect.","wms" ) );

    bbox.xmin=bb[0];
    bbox.ymin=bb[1];
    bbox.xmax=bb[2];
    bbox.ymax=bb[3];

    // Data are stored in Long/Lat, Geographical system need to be inverted in EPSG registry
    if ( crs->getAuthority() == "EPSG" && crs->isLongLat() && version == "1.3.0" ) {
        bbox.xmin=bb[1];
        bbox.ymin=bb[0];
        bbox.xmax=bb[3];
        bbox.ymax=bb[2];
    }

    bbox.crs = crs->getRequestCode();

    // EXCEPTION
    std::string str_exception = request->getParam ( "exception" );
    if ( str_exception != "" && str_exception != "XML" ) {

        if ( containForbiddenChars(str_exception) ) {
            // On a détecté un caractère interdit, on ne met pas le str_exception fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS exception: " << str_exception ;
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Format d'exception non pris en charge","wms" ) );
        }

        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Format d'exception " +str_exception+" non pris en charge","wms" ) );
    }

    //STYLES
    if ( ! request->hasParam ( "styles" ) )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLES absent.","wms" ) );

    std::string str_styles = request->getParam ( "styles" );
    if ( str_styles == "" ) {
        str_styles.append ( layers.at ( 0 )->getDefaultStyle()->getIdentifier() );
        for ( int i = 1;  i < layers.size(); i++ ) {
            str_styles.append ( "," );
            str_styles.append ( layers.at ( i )->getDefaultStyle()->getIdentifier() );
        }
    }

    std::vector<std::string> vector_styles = split ( str_styles,',' );
    BOOST_LOG_TRIVIAL(debug) <<  "Nombre de styles demandes =" << vector_styles.size()  ;
    if ( vector_styles.size() != vector_layers.size() ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLES incomplet.","wms" ) );
    }
    for ( int k = 0 ; k  < vector_styles.size(); k++ ) {
        if ( vector_styles.at ( k ) == "" ) {
            vector_styles.at (k) = layers.at ( k )->getDefaultStyle()->getIdentifier();
        }


        if ( containForbiddenChars(vector_styles.at ( k )) ) {
            // On a détecté un caractère interdit, on ne met pas le style fourni dans la réponse pour éviter une injection
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS styles: " << vector_styles.at ( k ) ;
            return new SERDataStream ( new ServiceException ( "",WMS_STYLE_NOT_DEFINED,"Le style n'est pas gere pour la couche " +vector_layers.at ( k ),"wms" ) );
        }

        Style* s = layers.at ( k )->getStyleByIdentifier(vector_styles.at ( k ));
        if ( s == NULL )
            return new SERDataStream ( new ServiceException ( "",WMS_STYLE_NOT_DEFINED,"Le style " +vector_styles.at ( k ) +" n'est pas gere pour la couche " +vector_layers.at ( k ),"wms" ) );

        styles.push_back(s);
    }

    //DPI
    std::string strDPI = request->getParam("dpi");
    if (strDPI != "") {
        dpi = atoi(strDPI.c_str());
        if ( dpi == 0 || dpi == INT_MAX || dpi == INT_MIN ) {
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre DPI n'est pas une valeur entiere.","wms" ) );
        }
        if ( dpi<0 ) {
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre DPI est negative.","wms" ) );
        }

    } else {
        dpi = 0;
    }

    // FORMAT OPTIONS
    std::string formatOptionString = request->getParam ( "format_options" ).c_str();
    char* formatOptionChar = new char[formatOptionString.size() +1];
    memset ( formatOptionChar,0,formatOptionString.size() +1 );
    memcpy ( formatOptionChar,formatOptionString.c_str(),formatOptionString.size() );

    for ( int pos = 0; formatOptionChar[pos]; ) {
        char* key = formatOptionChar + pos;
        for ( ; formatOptionChar[pos] && formatOptionChar[pos] != ':' && formatOptionChar[pos] != ';'; pos++ ); // on trouve le premier "=", "&" ou 0
        char* value = formatOptionChar + pos;
        for ( ; formatOptionChar[pos] && formatOptionChar[pos] != ';'; pos++ ); // on trouve le suivant "&" ou 0
        if ( *value == ':' ) *value++ = 0; // on met un 0 à la place du '=' entre key et value
        if ( formatOptionChar[pos] ) formatOptionChar[pos++] = 0; // on met un 0 à la fin du char* value

        Request::toLowerCase ( key );
        Request::toLowerCase ( value );
        format_option.insert ( std::pair<std::string, std::string> ( key, value ) );
    }
    delete[] formatOptionChar;
    formatOptionChar = NULL;
    return NULL;
}

// Parameters for WMS GetFeatureInfo
DataStream* Rok4Server::getFeatureInfoParamWMS (
    Request* request, std::vector<Layer*>& layers, std::vector<Layer*>& query_layers,
    BoundingBox< double >& bbox, int& width, int& height, CRS*& crs, std::string& format,
    std::vector<Style*>& styles, std::string& info_format, int& X, int& Y, int& feature_count,std::map <std::string, std::string >& format_option
){

    int dpi;
    DataStream* getMapError = getMapParamWMS(request,layers, bbox,width,height,crs,format,styles,format_option,dpi);
    
    if (getMapError) {
        return getMapError;
    }
    
    // QUERY_LAYERS
    std::string str_query_layer = request->getParam ( "query_layers" );
    if ( str_query_layer == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre QUERY_LAYERS absent.","wms" ) );
    //Split layer Element
    std::vector<std::string> queryLayersString = split ( str_query_layer,',' );
    BOOST_LOG_TRIVIAL(debug) <<  "Nombre de couches demandees =" << queryLayersString.size()  ;
    if ( queryLayersString.size() > 1 ) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le nombre de couche interrogée est limité à 1.","wms" ) );
    }

    for (unsigned u1 = 0; u1 < queryLayersString.size(); u1++) {

        if ( containForbiddenChars(queryLayersString.at(u1))) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS query_layer : " << queryLayersString.at(u1) ;
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,"Query_Layer inconnu.","wms" ) );
        }

        Layer* lay = serverConf->getLayer(queryLayersString.at(u1));
        if ( lay == NULL )
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_DEFINED,"Query_Layer " +queryLayersString.at(u1)+" inconnu.","wms" ) );
    
        bool querylay_is_in_layer = false;
        std::vector<Layer*>::iterator itLay = layers.begin();
        for (unsigned int u2 = 0 ; u2 < layers.size(); u2++ ){
            if ( lay->getId() == layers.at(u2)->getId() ) {
                querylay_is_in_layer = true;
                break;
            }
        }
        if (querylay_is_in_layer == false){
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Query_Layer " +queryLayersString.at(u1)+" absent de layer.","wms" ) );
        }
    
        if (lay->isGetFeatureInfoAvailable() == false){
            return new SERDataStream ( new ServiceException ( "",WMS_LAYER_NOT_QUERYABLE,"Query_Layer " +queryLayersString.at(u1)+" non interrogeable.","wms" ) );
        }
        query_layers.push_back ( lay );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Nombre de couches requetées =" << query_layers.size()  ;


    // FEATURE_COUNT (facultative)
    std::string strFeatureCount = request->getParam ( "feature_count" );
    if ( strFeatureCount == "" ){
        feature_count = 1;
    } else {
        feature_count = atoi ( strFeatureCount.c_str() );
        if ( feature_count == 0 || feature_count == INT_MAX || feature_count == INT_MIN )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre FEATURE_COUNT n'est pas une valeur entiere.","wms" ) ) ;
        if ( feature_count<0 )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre FEATURE_COUNT est negative.","wms" ) );
    }


    // I
    std::string strX = request->getParam ( "i" );
    if ( strX == "" ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre I absent.","wms" ) );
    }
    char c;
    if (sscanf(strX.c_str(), "%d%c", &X, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre I n'est pas un entier.","wms" ) );
    }
    if ( X<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre I est negative.","wms" ) );
    if ( X>width )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre I est superieure a la largeur fournie (width).","wms" ) );
    
   

    // J
    std::string strY = request->getParam ( "j" );
    if ( strY == "" ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre J absent.","wms" ) );
    }
    if (sscanf(strY.c_str(), "%d%c", &Y, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre J n'est pas un entier.","wms" ) );
    }
    if ( Y<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre J est negative.","wms" ) );
    if ( Y>height )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre J est superieure a la hauteur fournie (height).","wms" ) );

    
    
    // INFO_FORMAT
    unsigned int k;
    info_format = request->getParam ( "info_format" );
    if ( info_format == "" ){
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre INFO_FORMAT vide.","wms" ) );
    } else {
        if ( containForbiddenChars(info_format)) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMS info_format: " << info_format ;
            return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,"Info_Format non gere par le service.","wms" ) );
        }
        if ( ! servicesConf->isInInfoFormatList(info_format) )
            return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,"Info_Format " +info_format+" non gere par le service.","wms" ) );
    }

    return NULL;
}

DataStream* Rok4Server::getCapParamWMS ( Request* request, std::string& version ) {
    // VERSION
    version = request->getParam ( "version" );
    if ( version == "" ) {
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre VERSION absent.","wms" ) );
    }

    if ( version != "1.3.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.3.0 disponible seulement))","wms" ) );

    return NULL;
}

void Rok4Server::buildWMSCapabilities() {

    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration ( "1.0", "UTF-8", "" );
    doc.LinkEndChild ( decl );
    std::ostringstream os;

    TiXmlElement * capabilitiesEl = new TiXmlElement ( "WMS_Capabilities" );
    capabilitiesEl->SetAttribute ( "version","1.3.0" );
    capabilitiesEl->SetAttribute ( "xmlns","http://www.opengis.net/wms" );
    capabilitiesEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    capabilitiesEl->SetAttribute ( "xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd" );

    // Pour Inspire. Cf. remarque plus bas.
    if ( servicesConf->inspire ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd  http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd http://inspire.ec.europa.eu/schemas/common/1.0 http://inspire.ec.europa.eu/schemas/common/1.0/common.xsd" );
    }

    // Traitement de la partie service
    //----------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "Service" );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "Name",servicesConf->name ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "Title",servicesConf->title ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "Abstract",servicesConf->abstract ) );
    //KeywordList
    if ( servicesConf->keyWords.size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );
        for ( unsigned int i=0; i < servicesConf->keyWords.size(); i++ ) {
            kwlEl->LinkEndChild ( UtilsXML::getXml("Keyword", servicesConf->keyWords.at ( i )) );
        }
        serviceEl->LinkEndChild ( kwlEl );
    }  
    //OnlineResource
    TiXmlElement * onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href", servicesConf->wmsPublicUrl );
    serviceEl->LinkEndChild ( onlineResourceEl );
    // ContactInformation
    TiXmlElement * contactInformationEl = new TiXmlElement ( "ContactInformation" );

    TiXmlElement * contactPersonPrimaryEl = new TiXmlElement ( "ContactPersonPrimary" );
    contactPersonPrimaryEl->LinkEndChild ( UtilsXML::buildTextNode ( "ContactPerson",servicesConf->individualName ) );
    contactPersonPrimaryEl->LinkEndChild ( UtilsXML::buildTextNode ( "ContactOrganization",servicesConf->serviceProvider ) );

    contactInformationEl->LinkEndChild ( contactPersonPrimaryEl );

    contactInformationEl->LinkEndChild ( UtilsXML::buildTextNode ( "ContactPosition",servicesConf->individualPosition ) );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ContactAddress" );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "AddressType",servicesConf->addressType ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "Address",servicesConf->deliveryPoint ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "City",servicesConf->city ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "StateOrProvince",servicesConf->administrativeArea ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "PostCode",servicesConf->postCode ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "Country",servicesConf->country ) );

    contactInformationEl->LinkEndChild ( contactAddressEl );

    contactInformationEl->LinkEndChild ( UtilsXML::buildTextNode ( "ContactVoiceTelephone",servicesConf->voice ) );

    contactInformationEl->LinkEndChild ( UtilsXML::buildTextNode ( "ContactFacsimileTelephone",servicesConf->facsimile ) );

    contactInformationEl->LinkEndChild ( UtilsXML::buildTextNode ( "ContactElectronicMailAddress",servicesConf->electronicMailAddress ) );

    serviceEl->LinkEndChild ( contactInformationEl );

    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "Fees",servicesConf->fee ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "AccessConstraints",servicesConf->accessConstraint ) );

    os << servicesConf->layerLimit;
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "LayerLimit",os.str() ) );
    os.str ( "" );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "MaxWidth",numToStr ( servicesConf->maxWidth ) ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "MaxHeight",numToStr ( servicesConf->maxHeight ) ) );

    capabilitiesEl->LinkEndChild ( serviceEl );



    // Traitement de la partie Capability
    //-----------------------------------
    TiXmlElement * capabilityEl = new TiXmlElement ( "Capability" );
    TiXmlElement * requestEl = new TiXmlElement ( "Request" );
    TiXmlElement * getCapabilitiestEl = new TiXmlElement ( "GetCapabilities" );

    getCapabilitiestEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format","text/xml" ) );
    //DCPType
    TiXmlElement * DCPTypeEl = new TiXmlElement ( "DCPType" );
    TiXmlElement * HTTPEl = new TiXmlElement ( "HTTP" );
    TiXmlElement * GetEl = new TiXmlElement ( "Get" );

    //OnlineResource
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",servicesConf->wmsPublicUrl );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf->postMode ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",servicesConf->wmsPublicUrl );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getCapabilitiestEl->LinkEndChild ( DCPTypeEl );
    requestEl->LinkEndChild ( getCapabilitiestEl );

    TiXmlElement * getMapEl = new TiXmlElement ( "GetMap" );
    for ( unsigned int i=0; i<servicesConf->formatList.size(); i++ ) {
        getMapEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format",servicesConf->formatList.at ( i ) ) );
    }
    DCPTypeEl = new TiXmlElement ( "DCPType" );
    HTTPEl = new TiXmlElement ( "HTTP" );
    GetEl = new TiXmlElement ( "Get" );
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",servicesConf->wmsPublicUrl );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf->postMode ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",servicesConf->wmsPublicUrl );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getMapEl->LinkEndChild ( DCPTypeEl );

    requestEl->LinkEndChild ( getMapEl );
    
    
    TiXmlElement * getFeatureInfoEl = new TiXmlElement ( "GetFeatureInfo" );
    for ( unsigned int i=0; i<servicesConf->infoFormatList.size(); i++ ) {
        getFeatureInfoEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format",servicesConf->infoFormatList.at ( i ) ) );
    }
    DCPTypeEl = new TiXmlElement ( "DCPType" );
    HTTPEl = new TiXmlElement ( "HTTP" );
    GetEl = new TiXmlElement ( "Get" );
    onlineResourceEl = new TiXmlElement ( "OnlineResource" );
    onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    onlineResourceEl->SetAttribute ( "xlink:href",servicesConf->wmsPublicUrl );
    onlineResourceEl->SetAttribute ( "xlink:type","simple" );
    GetEl->LinkEndChild ( onlineResourceEl );
    HTTPEl->LinkEndChild ( GetEl );

    if ( servicesConf->postMode ) {
        TiXmlElement * PostEl = new TiXmlElement ( "Post" );
        onlineResourceEl = new TiXmlElement ( "OnlineResource" );
        onlineResourceEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
        onlineResourceEl->SetAttribute ( "xlink:href",servicesConf->wmsPublicUrl );
        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
        PostEl->LinkEndChild ( onlineResourceEl );
        HTTPEl->LinkEndChild ( PostEl );
    }

    DCPTypeEl->LinkEndChild ( HTTPEl );
    getFeatureInfoEl->LinkEndChild ( DCPTypeEl );

    requestEl->LinkEndChild ( getFeatureInfoEl );

    capabilityEl->LinkEndChild ( requestEl );

    //Exception
    TiXmlElement * exceptionEl = new TiXmlElement ( "Exception" );
    exceptionEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format","XML" ) );
    capabilityEl->LinkEndChild ( exceptionEl );

    // Inspire (extended Capability)
    if ( servicesConf->inspire && servicesConf->mtdWMS ) {
        // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( UtilsXML::buildTextNode ( "inspire_common:URL", servicesConf->mtdWMS->getHRef() ) );
        metadataUrlEl->LinkEndChild ( UtilsXML::buildTextNode ( "inspire_common:MediaType", servicesConf->mtdWMS->getType() ) );
        extendedCapabilititesEl->LinkEndChild ( metadataUrlEl );

        // Languages
        TiXmlElement * supportedLanguagesEl = new TiXmlElement ( "inspire_common:SupportedLanguages" );
        TiXmlElement * defaultLanguageEl = new TiXmlElement ( "inspire_common:DefaultLanguage" );
        TiXmlElement * languageEl = new TiXmlElement ( "inspire_common:Language" );
        TiXmlText * lfre = new TiXmlText ( "fre" );
        languageEl->LinkEndChild ( lfre );
        defaultLanguageEl->LinkEndChild ( languageEl );
        supportedLanguagesEl->LinkEndChild ( defaultLanguageEl );
        extendedCapabilititesEl->LinkEndChild ( supportedLanguagesEl );
        // Responselanguage
        TiXmlElement * responseLanguageEl = new TiXmlElement ( "inspire_common:ResponseLanguage" );
        responseLanguageEl->LinkEndChild ( UtilsXML::buildTextNode ( "inspire_common:Language","fre" ) );
        extendedCapabilititesEl->LinkEndChild ( responseLanguageEl );

        capabilityEl->LinkEndChild ( extendedCapabilititesEl );
    }
    // Layer
    if ( serverConf->layersList.empty() ) {
        BOOST_LOG_TRIVIAL(warning) <<  "Liste de layers vide"  ;
    } else {
        // Parent layer
        TiXmlElement * parentLayerEl = new TiXmlElement ( "Layer" );
        // Title
        parentLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "Title", "cache IGN" ) );
        // Abstract
        parentLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "Abstract", "Cache IGN" ) );
        // Global CRS
        for ( unsigned int i=0; i < servicesConf->globalCRSList.size(); i++ ) {
            parentLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "CRS", servicesConf->globalCRSList.at ( i )->getRequestCode() ) );
        }
        // Child layers
        std::map<std::string, Layer*>::iterator it;
        for ( it=serverConf->layersList.begin(); it!=serverConf->layersList.end(); it++ ) {
            //Look if the layer is published in WMS
            if (it->second->getWMSAuthorized()) {
                TiXmlElement * childLayerEl = new TiXmlElement ( "Layer" );
                Layer* childLayer = it->second;
                // queryable
                if (childLayer->isGetFeatureInfoAvailable()){
                 childLayerEl->SetAttribute ( "queryable","1" ); 
                }
                // Name
                childLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "Name", childLayer->getId() ) );
                // Title
                childLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "Title", childLayer->getTitle() ) );
                // Abstract
                childLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "Abstract", childLayer->getAbstract() ) );
                // KeywordList
                if ( childLayer->getKeyWords()->size() != 0 ) {
                    TiXmlElement * kwlEl = new TiXmlElement ( "KeywordList" );
                    for ( unsigned int i=0; i < childLayer->getKeyWords()->size(); i++ ) {
                        kwlEl->LinkEndChild ( UtilsXML::getXml("Keyword", childLayer->getKeyWords()->at ( i )) );
                    }
                    childLayerEl->LinkEndChild ( kwlEl );
                }

                // CRS
                std::vector<CRS*> vectorCRS = childLayer->getWMSCRSList();
                int layerSize = vectorCRS.size();
                for (int i=0; i < layerSize; i++ ) {
                    childLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "CRS", vectorCRS[i]->getRequestCode() ) );
                }

                // GeographicBoundingBox
                TiXmlElement * gbbEl = new TiXmlElement ( "EX_GeographicBoundingBox" );

                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().xmin;
                gbbEl->LinkEndChild ( UtilsXML::buildTextNode ( "westBoundLongitude", os.str() ) );
                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().xmax;
                gbbEl->LinkEndChild ( UtilsXML::buildTextNode ( "eastBoundLongitude", os.str() ) );
                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().ymin;
                gbbEl->LinkEndChild ( UtilsXML::buildTextNode ( "southBoundLatitude", os.str() ) );
                os.str ( "" );
                os<<childLayer->getGeographicBoundingBox().ymax;
                gbbEl->LinkEndChild ( UtilsXML::buildTextNode ( "northBoundLatitude", os.str() ) );
                os.str ( "" );
                childLayerEl->LinkEndChild ( gbbEl );


                // BoundingBox
                if ( servicesConf->inspire ) {
                    for ( unsigned int i=0; i < childLayer->getWMSCRSList().size(); i++ ) {
                        CRS* crs = childLayer->getWMSCRSList() [i];
                        BoundingBox<double> bbox ( 0,0,0,0 );
                        if ( childLayer->getGeographicBoundingBox().isInAreaOfCRS(crs)) {
                            bbox = childLayer->getGeographicBoundingBox();
                        } else {
                            bbox = childLayer->getGeographicBoundingBox().cropToAreaOfCRS(crs);
                        }

                        bbox.reproject(CRS::getEpsg4326(), crs);

                        //Switch lon lat for EPSG longlat CRS
                        if ( crs->getAuthority() =="EPSG" && crs->isLongLat() ) {
                            double doubletmp;
                            doubletmp = bbox.xmin;
                            bbox.xmin = bbox.ymin;
                            bbox.ymin = doubletmp;
                            doubletmp = bbox.xmax;
                            bbox.xmax = bbox.ymax;
                            bbox.ymax = doubletmp;
                        }

                        TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                        bbEl->SetAttribute ( "CRS", crs->getRequestCode() );
                        int floatprecision = GetDecimalPlaces ( bbox.xmin );
                        floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                        floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                        floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                        floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.

                        os.str ( "" );
                        os<< std::fixed << std::setprecision ( floatprecision );
                        os<<bbox.xmin;
                        bbEl->SetAttribute ( "minx",os.str() );
                        os.str ( "" );
                        os<<bbox.ymin;
                        bbEl->SetAttribute ( "miny",os.str() );
                        os.str ( "" );
                        os<<bbox.xmax;
                        bbEl->SetAttribute ( "maxx",os.str() );
                        os.str ( "" );
                        os<<bbox.ymax;
                        bbEl->SetAttribute ( "maxy",os.str() );
                        os.str ( "" );
                        childLayerEl->LinkEndChild ( bbEl );
                    }
                    for ( unsigned int i=0; i < servicesConf->globalCRSList.size(); i++ ) {
                        CRS* crs = servicesConf->globalCRSList.at ( i );
                        BoundingBox<double> bbox ( 0,0,0,0 );
                        if ( childLayer->getGeographicBoundingBox().isInAreaOfCRS(crs)) {
                            bbox = childLayer->getGeographicBoundingBox();
                        } else {
                            bbox = childLayer->getGeographicBoundingBox().cropToAreaOfCRS(crs);
                        }

                        bbox.reproject(CRS::getEpsg4326(), crs);
                        
                        //Switch lon lat for EPSG longlat CRS
                        if ( crs->getAuthority() =="EPSG" && crs->isLongLat() ) {
                            double doubletmp;
                            doubletmp = bbox.xmin;
                            bbox.xmin = bbox.ymin;
                            bbox.ymin = doubletmp;
                            doubletmp = bbox.xmax;
                            bbox.xmax = bbox.ymax;
                            bbox.ymax = doubletmp;
                        }

                        TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );
                        bbEl->SetAttribute ( "CRS", crs->getRequestCode() );
                        int floatprecision = GetDecimalPlaces ( bbox.xmin );
                        floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                        floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                        floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                        floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.
                        os.str ( "" );
                        os<< std::fixed << std::setprecision ( floatprecision );
                        os<<bbox.xmin;
                        bbEl->SetAttribute ( "minx",os.str() );
                        os.str ( "" );
                        os<<bbox.ymin;
                        bbEl->SetAttribute ( "miny",os.str() );
                        os.str ( "" );
                        os<<bbox.xmax;
                        bbEl->SetAttribute ( "maxx",os.str() );
                        os.str ( "" );
                        os<<bbox.ymax;
                        bbEl->SetAttribute ( "maxy",os.str() );
                        os.str ( "" );
                        childLayerEl->LinkEndChild ( bbEl );
                    }
                } else {
                    TiXmlElement * bbEl = new TiXmlElement ( "BoundingBox" );

                    bbEl->SetAttribute ( "CRS",childLayer->getBoundingBox().crs );
                    BoundingBox<double> bbox = childLayer->getBoundingBox();
                    CRS* crs = childLayer->getDataPyramid()->getTms()->getCrs();
                    // Switch lon lat for EPSG longlat CRS
                    if ( crs->getAuthority() =="EPSG" && crs->isLongLat() ) {
                        double doubletmp;
                        doubletmp = bbox.xmin;
                        bbox.xmin = bbox.ymin;
                        bbox.ymin = doubletmp;
                        doubletmp = bbox.xmax;
                        bbox.xmax = bbox.ymax;
                        bbox.ymax = doubletmp;
                    }

                    os.str ( "" );
                    os<< std::fixed << std::setprecision ( 9 );
                    os<<bbox.xmin;
                    bbEl->SetAttribute ( "minx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymin;
                    bbEl->SetAttribute ( "miny",os.str() );
                    os.str ( "" );
                    os<<bbox.xmax;
                    bbEl->SetAttribute ( "maxx",os.str() );
                    os.str ( "" );
                    os<<bbox.ymax;
                    bbEl->SetAttribute ( "maxy",os.str() );

                    childLayerEl->LinkEndChild ( bbEl );
                }
                //MetadataURL
                if ( childLayer->getMetadataURLs().size() != 0 ) {
                    for ( unsigned int i=0; i < childLayer->getMetadataURLs().size(); ++i ) {
                        TiXmlElement * mtdURLEl = new TiXmlElement ( "MetadataURL" );
                        MetadataURL mtdUrl = childLayer->getMetadataURLs().at ( i );
                        mtdURLEl->SetAttribute ( "type", mtdUrl.getType() );
                        mtdURLEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format",mtdUrl.getFormat() ) );

                        TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                        onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                        onlineResourceEl->SetAttribute ( "xlink:href", mtdUrl.getHRef() );
                        mtdURLEl->LinkEndChild ( onlineResourceEl );
                        childLayerEl->LinkEndChild ( mtdURLEl );
                    }
                }
                //Attribution
                if ( childLayer->getAttribution() != NULL ) {
                    childLayerEl->LinkEndChild ( childLayer->getAttribution()->getWmsXml() );
                }

                // Style
                BOOST_LOG_TRIVIAL(debug) <<  "Nombre de styles : " <<childLayer->getStyles().size()  ;
                if ( childLayer->getStyles().size() != 0 ) {
                    for ( unsigned int i=0; i < childLayer->getStyles().size(); i++ ) {
                        TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                        Style* style = childLayer->getStyles() [i];
                        styleEl->LinkEndChild ( UtilsXML::buildTextNode ( "Name", style->getIdentifier().c_str() ) );
                        int j;
                        for ( j=0 ; j < style->getTitles().size(); ++j ) {
                            styleEl->LinkEndChild ( UtilsXML::buildTextNode ( "Title", style->getTitles() [j].c_str() ) );
                        }
                        for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                            styleEl->LinkEndChild ( UtilsXML::buildTextNode ( "Abstract", style->getAbstracts() [j].c_str() ) );
                        }
                        for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                            BOOST_LOG_TRIVIAL(debug) <<  "LegendURL " << style->getId()  ;
                            LegendURL legendURL = style->getLegendURLs() [j];
                            TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );

                            TiXmlElement* onlineResourceEl = new TiXmlElement ( "OnlineResource" );
                            onlineResourceEl->SetAttribute ( "xlink:type","simple" );
                            onlineResourceEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                            legendURLEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format", legendURL.getFormat() ) );
                            legendURLEl->LinkEndChild ( onlineResourceEl );

                            if ( legendURL.getWidth() !=0 )
                                legendURLEl->SetAttribute ( "width", legendURL.getWidth() );
                            if ( legendURL.getHeight() !=0 )
                                legendURLEl->SetAttribute ( "height", legendURL.getHeight() );
                            styleEl->LinkEndChild ( legendURLEl );
                            BOOST_LOG_TRIVIAL(debug) <<  "LegendURL OK" << style->getId()  ;
                        }

                        BOOST_LOG_TRIVIAL(debug) <<  "Style fini : " << style->getId()  ;
                        childLayerEl->LinkEndChild ( styleEl );
                    }
                }

                // Scale denominators
                os.str ( "" );
                os<<childLayer->getMinRes() *1000/0.28;
                childLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "MinScaleDenominator", os.str() ) );
                os.str ( "" );
                os<<childLayer->getMaxRes() *1000/0.28;
                childLayerEl->LinkEndChild ( UtilsXML::buildTextNode ( "MaxScaleDenominator", os.str() ) );

                // TODO : gerer le cas des CRS avec des unites en degres

                BOOST_LOG_TRIVIAL(debug) <<  "Layer Fini"  ;
                parentLayerEl->LinkEndChild ( childLayerEl );
            }
        }// for layer

        BOOST_LOG_TRIVIAL(debug) <<  "Layers Fini"  ;
        capabilityEl->LinkEndChild ( parentLayerEl );
    }

    capabilitiesEl->LinkEndChild ( capabilityEl );
    doc.LinkEndChild ( capabilitiesEl );

    wmsCapabilities.clear();
    wmsCapabilities << doc;  // ecriture non formatée dans un std::string
    doc.Clear();
    BOOST_LOG_TRIVIAL(debug) <<  "WMS 1.3.0 fini"  ;
}

DataStream* Rok4Server::WMSGetCapabilities ( Request* request ) {

    std::string version;
    DataStream* errorResp = getCapParamWMS ( request, version );
    if ( errorResp ) {
        BOOST_LOG_TRIVIAL(error) << "GetCapabilities request : invalid version"  ;
        return errorResp;
    }

    return new MessageDataStream ( wmsCapabilities,"text/xml" );
}
