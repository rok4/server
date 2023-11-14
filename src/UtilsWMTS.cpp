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
 * \file UtilsWMTS.cpp
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
#include <set>
#include <cmath>
#include <rok4/utils/TileMatrixSet.h>
#include <rok4/utils/Pyramid.h>
#include <rok4/utils/Utils.h>

DataSource* Rok4Server::getTileParamWMTS ( Request* request, Layer*& layer, TileMatrixSet*& tms, TileMatrix*& tm, int& tileCol, int& tileRow, std::string& format, Style*& style) {
    // VERSION
    std::string version = request->getParam ( "version" );
    if ( version == "" ) {
        version = "1.0.0";
    }

    if ( version != "1.0.0" )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.0.0 disponible seulement))","wmts" ) );

    // LAYER
    std::string str_layer = request->getParam ( "layer" );
    if ( str_layer == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre LAYER absent.","wmts" ) );
    
    if ( containForbiddenChars(str_layer)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS layer: " << str_layer ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer inconnu.","wmts" ) );
    }

    layer = serverConf->getLayer(str_layer);
    if ( layer == NULL || ! layer->getWMTSAuthorized() )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer " +str_layer+" inconnu.","wmts" ) );


    // TILEMATRIXSET
    std::string str_tms = request->getParam ( "tilematrixset" );
    if ( str_tms == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIXSET absent.","wmts" ) );

    if ( containForbiddenChars(str_tms)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS tilematrixset: " << str_tms ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrixSet inconnu.","wmts" ) );
    }

    tms = TmsBook::get_tms(str_tms);
    if ( tms == NULL )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrixSet " +str_tms+" inconnu.","wmts" ) );

    if ( tms->getId() != layer->getDataPyramid()->getTms()->getId() && ! layer->isInWMTSTMSList(tms)) {
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrixSet " +str_tms+" inconnu pour le layer.","wmts" ) );
    }
    
    // TILEMATRIX
    std::string str_tm = request->getParam ( "tilematrix" );
    if ( str_tm == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEMATRIX absent.","wmts" ) );

    if ( containForbiddenChars(str_tm)) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS tilematrix: " << str_tm ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrix inconnu pour le TileMatrixSet " +str_tms,"wmts" ) );
    }

    tm = tms->getTm(str_tm);

    if ( tm == NULL )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"TileMatrix " +str_tm+" inconnu pour le TileMatrixSet " +str_tms,"wmts" ) );

    // TILEROW
    std::string str_TileRow = request->getParam ( "tilerow" );
    if ( str_TileRow == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILEROW absent.","wmts" ) );
    if ( sscanf ( str_TileRow.c_str(),"%d",&tileRow ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILEROW est incorrecte.","wmts" ) );

    // TILECOL
    std::string str_TileCol = request->getParam ( "tilecol" );
    if ( str_TileCol == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre TILECOL absent.","wmts" ) );
    if ( sscanf ( str_TileCol.c_str(),"%d",&tileCol ) !=1 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre TILECOL est incorrecte.","wmts" ) );

    if ( tms->getId() == layer->getDataPyramid()->getTms()->getId()) {
        // TMS natif de la pyramide, les tuiles limites sont stockées dans le niveau
        Level* level = layer->getDataPyramid()->getLevel(tm->getId());
        if (level == NULL) {
            // On est hors niveau -> erreur
            return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );
        }

        if (! level->getTileLimits().containTile(tileCol, tileRow)) {
            // On est hors tuiles -> erreur
            return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );
        }
    } else if (layer->isInWMTSTMSList(tms)) {
        // TMS supplémentaire, les tuiles limites sont stockées dans la couche
        TileMatrixLimits* tml = layer->getTmLimits(tms, tm);
        if (tml == NULL) {
            // On est hors niveau -> erreur
            return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );
        }
        if (! tml->containTile(tileCol, tileRow)) {
            // On est hors tuiles -> erreur
            return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );
        }
    }

    // FORMAT
    format = request->getParam ( "format" );

    BOOST_LOG_TRIVIAL(debug) <<  "format requete : " << format <<" format pyramide : " << Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) )  ;
    if ( format == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre FORMAT absent.","wmts" ) );

    if ( containForbiddenChars(format) ) {
        // On a détecté un caractère interdit, on ne met pas le format fourni dans la réponse pour éviter une injection
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS format: " << format ;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le format n'est pas gere pour la couche " +str_layer,"wmts" ) );
    }

    if ( format.compare ( Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) !=0 )
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le format " +format+" n'est pas gere pour la couche " +str_layer,"wmts" ) );

    //Style

    std::string str_style = request->getParam ( "style" );
    if ( str_style == "" )
        return new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre STYLE absent.","wmts" ) );
    // TODO : Nom de style : inspire_common:DEFAULT en mode Inspire sinon default

    if (Rok4Format::isRaster(layer->getDataPyramid()->getFormat())) {
        style = layer->getStyleByIdentifier(str_style);

        if ( ! ( style ) )
            return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le style " +str_style+" n'est pas gere pour la couche " +str_layer,"wmts" ) );
    }

    return NULL;
}

// Prepare WMTS GetCapabilities fragments
//   Done only 1 time (during server initialization)
void Rok4Server::buildWMTSCapabilities() {

    std::map<std::string,TileMatrixSet*> usedTMSList;

    TiXmlDocument doc;
    TiXmlDeclaration * decl = new TiXmlDeclaration ( "1.0", "UTF-8", "" );
    doc.LinkEndChild ( decl );

    TiXmlElement * capabilitiesEl = new TiXmlElement ( "Capabilities" );
    capabilitiesEl->SetAttribute ( "version","1.0.0" );
    // attribut UpdateSequence à ajouter quand on en aura besoin
    capabilitiesEl->SetAttribute ( "xmlns","http://www.opengis.net/wmts/1.0" );
    capabilitiesEl->SetAttribute ( "xmlns:ows","http://www.opengis.net/ows/1.1" );
    capabilitiesEl->SetAttribute ( "xmlns:xlink","http://www.w3.org/1999/xlink" );
    capabilitiesEl->SetAttribute ( "xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    capabilitiesEl->SetAttribute ( "xmlns:gml","http://www.opengis.net/gml" );
    capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd" );
    if ( servicesConf->inspire ) {
        capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0" );
        capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0/inspire_vs_ows_11.xsd" );
    }


    //----------------------------------------------------------------------
    // ServiceIdentification
    //----------------------------------------------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "ows:ServiceIdentification" );

    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Title", servicesConf->title ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Abstract", servicesConf->abstract ) );
    //KeywordList
    if ( servicesConf->keyWords.size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
        for ( unsigned int i=0; i < servicesConf->keyWords.size(); i++ ) {
            kwlEl->LinkEndChild ( UtilsXML::getXml("ows:Keyword", servicesConf->keyWords.at ( i )) );
        }
        serviceEl->LinkEndChild ( kwlEl );
    }
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:ServiceType", "OGC WMTS" ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:ServiceTypeVersion", "1.0.0" ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Fees", servicesConf->fee ) );
    serviceEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:AccessConstraints", servicesConf->accessConstraint ) );


    capabilitiesEl->LinkEndChild ( serviceEl );

    //----------------------------------------------------------------------
    // serviceProvider (facultatif)
    //----------------------------------------------------------------------
    TiXmlElement * serviceProviderEl = new TiXmlElement ( "ows:ServiceProvider" );

    serviceProviderEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:ProviderName",servicesConf->serviceProvider ) );
    TiXmlElement * providerSiteEl = new TiXmlElement ( "ows:ProviderSite" );
    providerSiteEl->SetAttribute ( "xlink:href",servicesConf->providerSite );
    serviceProviderEl->LinkEndChild ( providerSiteEl );

    TiXmlElement * serviceContactEl = new TiXmlElement ( "ows:ServiceContact" );

    serviceContactEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:IndividualName",servicesConf->individualName ) );
    serviceContactEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:PositionName",servicesConf->individualPosition ) );

    TiXmlElement * contactInfoEl = new TiXmlElement ( "ows:ContactInfo" );
    TiXmlElement * contactInfoPhoneEl = new TiXmlElement ( "ows:Phone" );

    contactInfoPhoneEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Voice",servicesConf->voice ) );
    contactInfoPhoneEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Facsimile",servicesConf->facsimile ) );

    contactInfoEl->LinkEndChild ( contactInfoPhoneEl );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ows:Address" );
    //contactAddressEl->LinkEndChild(UtilsXML::buildTextNode("AddressType","type"));
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:DeliveryPoint",servicesConf->deliveryPoint ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:City",servicesConf->city ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:AdministrativeArea",servicesConf->administrativeArea ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:PostalCode",servicesConf->postCode ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Country",servicesConf->country ) );
    contactAddressEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:ElectronicMailAddress",servicesConf->electronicMailAddress ) );
    contactInfoEl->LinkEndChild ( contactAddressEl );

    serviceContactEl->LinkEndChild ( contactInfoEl );

    serviceProviderEl->LinkEndChild ( serviceContactEl );
    capabilitiesEl->LinkEndChild ( serviceProviderEl );




    //----------------------------------------------------------------------
    // OperationsMetadata
    //----------------------------------------------------------------------
    TiXmlElement * opMtdEl = new TiXmlElement ( "ows:OperationsMetadata" );
    TiXmlElement * opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetCapabilities" );
    TiXmlElement * dcpEl = new TiXmlElement ( "ows:DCP" );
    TiXmlElement * httpEl = new TiXmlElement ( "ows:HTTP" );
    TiXmlElement * getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href", servicesConf->wmtsPublicUrl );
    TiXmlElement * constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    TiXmlElement * allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf->postMode ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href", servicesConf->wmtsPublicUrl );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(UtilsXML::buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );

    opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetTile" );
    dcpEl = new TiXmlElement ( "ows:DCP" );
    httpEl = new TiXmlElement ( "ows:HTTP" );
    getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href", servicesConf->wmtsPublicUrl );
    constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf->postMode ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href", servicesConf->wmtsPublicUrl );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(UtilsXML::buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );
    
    opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetFeatureInfo" );
    dcpEl = new TiXmlElement ( "ows:DCP" );
    httpEl = new TiXmlElement ( "ows:HTTP" );
    getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href", servicesConf->wmtsPublicUrl );
    constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    if ( servicesConf->postMode ) {
        TiXmlElement * postEl = new TiXmlElement ( "ows:Post" );
        postEl->SetAttribute ( "xlink:href", servicesConf->wmtsPublicUrl );
        constraintEl = new TiXmlElement ( "ows:Constraint" );
        constraintEl->SetAttribute ( "name","PostEncoding" );
        allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
        allowedValuesEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Value", "XML" ) );
        //TODO Implement SOAP like request
        //allowedValuesEl->LinkEndChild(UtilsXML::buildTextNode("ows:Value", "SOAP"));
        constraintEl->LinkEndChild ( allowedValuesEl );
        postEl->LinkEndChild ( constraintEl );
        httpEl->LinkEndChild ( postEl );
    }
    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );


    // Inspire (extended Capability)
    // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
    if ( servicesConf->inspire && servicesConf->mtdWMTS) {
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // MetadataURL
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( UtilsXML::buildTextNode ( "inspire_common:URL", servicesConf->mtdWMTS->getHRef() ) );
        metadataUrlEl->LinkEndChild ( UtilsXML::buildTextNode ( "inspire_common:MediaType", servicesConf->mtdWMTS->getType() ) );
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

        opMtdEl->LinkEndChild ( extendedCapabilititesEl );
    }
    capabilitiesEl->LinkEndChild ( opMtdEl );

    //----------------------------------------------------------------------
    // Contents
    //----------------------------------------------------------------------
    TiXmlElement * contentsEl=new TiXmlElement ( "Contents" );

    // Layer
    //------------------------------------------------------------------
    std::map<std::string, Layer*>::iterator itLay ( serverConf->layersList.begin() ), itLayEnd ( serverConf->layersList.end() );
    for ( ; itLay!=itLayEnd; ++itLay ) {
        //Look if the layer is published in WMTS
        if (itLay->second->getWMTSAuthorized()) {
            TiXmlElement * layerEl=new TiXmlElement ( "Layer" );
            Layer* layer = itLay->second;

            layerEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Title", layer->getTitle() ) );
            layerEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Abstract", layer->getAbstract() ) );
            if ( layer->getKeyWords()->size() != 0 ) {
                TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
                for ( unsigned int i=0; i < layer->getKeyWords()->size(); i++ ) {
                    kwlEl->LinkEndChild ( UtilsXML::getXml("ows:Keyword", layer->getKeyWords()->at ( i )) );
                }
                layerEl->LinkEndChild ( kwlEl );
            }
            //ows:WGS84BoundingBox (0,n)


            TiXmlElement * wgsBBEl = new TiXmlElement ( "ows:WGS84BoundingBox" );
            std::ostringstream os;
            os.str ( "" );
            os<<layer->getGeographicBoundingBox().xmin;
            os<<" ";
            os<<layer->getGeographicBoundingBox().ymin;
            wgsBBEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:LowerCorner", os.str() ) );
            os.str ( "" );
            os<<layer->getGeographicBoundingBox().xmax;
            os<<" ";
            os<<layer->getGeographicBoundingBox().ymax;
            wgsBBEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:UpperCorner", os.str() ) );
            os.str ( "" );
            layerEl->LinkEndChild ( wgsBBEl );


            layerEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Identifier", layer->getId() ) );

            //Style
            if ( layer->getStyles().size() != 0 ) {
                for ( unsigned int i=0; i < layer->getStyles().size(); i++ ) {
                    TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                    if ( i==0 ) styleEl->SetAttribute ( "isDefault","true" );
                    Style* style = layer->getStyles() [i];
                    int j;
                    for ( j=0 ; j < style->getTitles().size(); ++j ) {
                        BOOST_LOG_TRIVIAL(debug) <<  "Title : " << style->getTitles() [j].c_str()  ;
                        styleEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Title", style->getTitles() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->getAbstracts().size(); ++j ) {
                        BOOST_LOG_TRIVIAL(debug) <<  "Abstract : " << style->getAbstracts() [j].c_str()  ;
                        styleEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Abstract", style->getAbstracts() [j].c_str() ) );
                    }

                    if ( style->getKeywords()->size() != 0 ) {
                        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
                        for ( unsigned int i=0; i < style->getKeywords()->size(); i++ ) {
                            kwlEl->LinkEndChild ( UtilsXML::getXml("ows:Keyword", style->getKeywords()->at ( i )) );
                        }
                        styleEl->LinkEndChild ( kwlEl );
                    }

                    styleEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Identifier", style->getIdentifier() ) );
                    for ( j=0 ; j < style->getLegendURLs().size(); ++j ) {
                        LegendURL legendURL = style->getLegendURLs() [j];
                        TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );
                        legendURLEl->SetAttribute ( "format", legendURL.getFormat() );
                        legendURLEl->SetAttribute ( "xlink:href", legendURL.getHRef() );
                        if ( legendURL.getWidth() !=0 )
                            legendURLEl->SetAttribute ( "width", legendURL.getWidth() );
                        if ( legendURL.getHeight() !=0 )
                            legendURLEl->SetAttribute ( "height", legendURL.getHeight() );
                        if ( legendURL.getMinScaleDenominator() !=0.0 )
                            legendURLEl->SetAttribute ( "minScaleDenominator", legendURL.getMinScaleDenominator() );
                        if ( legendURL.getMaxScaleDenominator() !=0.0 )
                            legendURLEl->SetAttribute ( "maxScaleDenominator", legendURL.getMaxScaleDenominator() );
                        styleEl->LinkEndChild ( legendURLEl );
                    }
                    layerEl->LinkEndChild ( styleEl );
                }
            }

            // Contrainte : 1 layer = 1 pyramide = 1 format
            layerEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format",Rok4Format::toMimeType ( ( layer->getDataPyramid()->getFormat() ) ) ) );
            if (layer->isGetFeatureInfoAvailable()){
                for ( unsigned int i=0; i<servicesConf->infoFormatList.size(); i++ ) {
                    layerEl->LinkEndChild ( UtilsXML::buildTextNode ( "InfoFormat",servicesConf->infoFormatList.at ( i ) ) );
                }
            }

            // On précise le TMS natif et les limites de la pyramide
            TiXmlElement * tmsLinkEl = new TiXmlElement ( "TileMatrixSetLink" );
            tmsLinkEl->LinkEndChild ( UtilsXML::buildTextNode ( "TileMatrixSet",layer->getDataPyramid()->getTms()->getId() ) );
            usedTMSList.insert ( std::pair<std::string,TileMatrixSet*> ( layer->getDataPyramid()->getTms()->getId() , layer->getDataPyramid()->getTms()) );
            //tileMatrixSetLimits
            TiXmlElement * tmsLimitsEl = new TiXmlElement ( "TileMatrixSetLimits" );

            // Niveaux
            std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = layer->getDataPyramid()->getOrderedLevels(false);
            for (std::pair<std::string, Level*> element : orderedLevels) {
                Level * level = element.second;
                TiXmlElement * tmLimitsEl = UtilsXML::getXml(level->getTileLimits());
                tmsLimitsEl->LinkEndChild ( tmLimitsEl );
            }

            tmsLinkEl->LinkEndChild ( tmsLimitsEl );
            layerEl->LinkEndChild ( tmsLinkEl );

            // On le fait également pour chaque TMS supplémentaire (WMTS à la demande)
            for ( unsigned int i=0; i < layer->getWMTSTMSList().size(); i++ ) {
                TileMatrixSet* tms = layer->getWMTSTMSList().at(i);
                TiXmlElement * tmsLinkEl = new TiXmlElement ( "TileMatrixSetLink" );
                tmsLinkEl->LinkEndChild ( UtilsXML::buildTextNode ( "TileMatrixSet",tms->getId() ) );
                usedTMSList.insert ( std::pair<std::string,TileMatrixSet*> ( tms->getId() , tms) );
                
                TiXmlElement * tmsLimitsEl = new TiXmlElement ( "TileMatrixSetLimits" );

                // Niveaux
                for ( unsigned int j=0; j < layer->getWMTSTMSLimitsList().at(i).size(); j++ ) { 
                    TiXmlElement * tmLimitsEl = UtilsXML::getXml(layer->getWMTSTMSLimitsList().at(i).at(j));
                    tmsLimitsEl->LinkEndChild ( tmLimitsEl );
                }

                tmsLinkEl->LinkEndChild ( tmsLimitsEl );
                layerEl->LinkEndChild ( tmsLinkEl );

            }

            contentsEl->LinkEndChild ( layerEl );
        }

    }

    // TileMatrixSet
    //--------------------------------------------------------
    std::map<std::string,TileMatrixSet*>::iterator itTms ( usedTMSList.begin() ), itTmsEnd ( usedTMSList.end() );
    for ( ; itTms!=itTmsEnd; ++itTms ) {


        TileMatrixSet* tms = itTms->second;

        TiXmlElement * tmsEl=new TiXmlElement ( "TileMatrixSet" );
        tmsEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Identifier",tms->getId() ) );
        if ( ! ( tms->getTitle().empty() ) ) {
            tmsEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Title", tms->getTitle().c_str() ) );
        }

        if ( ! ( tms->getAbstract().empty() ) ) {
            tmsEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Abstract", tms->getAbstract().c_str() ) );
        }

        if ( tms->getKeyWords()->size() != 0 ) {
            TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
            TiXmlElement * kwEl;
            for ( unsigned int i=0; i < tms->getKeyWords()->size(); i++ ) {
                kwEl = new TiXmlElement ( "ows:Keyword" );
                kwEl->LinkEndChild ( new TiXmlText ( tms->getKeyWords()->at ( i ).getContent() ) );
                const std::map<std::string,std::string>* attributes = tms->getKeyWords()->at ( i ).getAttributes();
                for ( std::map<std::string,std::string>::const_iterator it = attributes->begin(); it !=attributes->end(); it++ ) {
                    kwEl->SetAttribute ( ( *it ).first, ( *it ).second );
                }

                kwlEl->LinkEndChild ( kwEl );
            }
            tmsEl->LinkEndChild ( kwlEl );
        }


        tmsEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:SupportedCRS",tms->getCrs()->getRequestCode() ) );
        
        // TileMatrix
        std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> orderedTM = tms->getOrderedTileMatrix(false);
        for (std::pair<std::string, TileMatrix*> element : orderedTM) {
            TileMatrix* tm = element.second;
            TiXmlElement * tmEl = new TiXmlElement ( "TileMatrix" );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "ows:Identifier",tm->getId() ) );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "ScaleDenominator",doubleToStr ( ( long double ) ( tm->getRes() * tms->getCrs()->getMetersPerUnit() ) /0.00028 ) ) );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "TopLeftCorner",doubleToStr ( tm->getX0() ) + " " + doubleToStr ( tm->getY0() ) ) );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "TileWidth",numToStr ( tm->getTileW() ) ) );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "TileHeight",numToStr ( tm->getTileH() ) ) );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "MatrixWidth",numToStr ( tm->getMatrixW() ) ) );
            tmEl->LinkEndChild ( UtilsXML::buildTextNode ( "MatrixHeight",numToStr ( tm->getMatrixH() ) ) );
            tmsEl->LinkEndChild ( tmEl );
        }

        contentsEl->LinkEndChild ( tmsEl );
    }

    capabilitiesEl->LinkEndChild ( contentsEl );
    doc.LinkEndChild ( capabilitiesEl );

    wmtsCapabilities.clear();
    wmtsCapabilities << doc;  // ecriture non formatée dans un std::string
    doc.Clear();
}

DataStream* Rok4Server::WMTSGetCapabilities ( Request* request ) {

    std::string version;
    DataStream* errorResp = getCapParamWMTS ( request, version );
    if ( errorResp ) {
        BOOST_LOG_TRIVIAL(error) <<  "Probleme dans les parametres de la requete getCapabilities"  ;
        return errorResp;
    }

    return new MessageDataStream ( wmtsCapabilities,"application/xml" );
}

// Parameters for WMTS GetCapabilities
DataStream* Rok4Server::getCapParamWMTS ( Request* request, std::string& version ) {

    std::string location = request->getParam("location");
    if (location != "") {
        request->path = location;
    }
    
    version = request->getParam ( "version" );
    if ( version == "" ) {
        version = "1.0.0";
    }

    if ( version != "1.0.0" )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Valeur du parametre VERSION invalide (1.0.0 disponible seulement))","wmts" ) );

    return NULL;
}

DataStream* Rok4Server::getFeatureInfoParamWMTS ( Request* request, Layer*& layer, TileMatrixSet*& tms, TileMatrix*& tm, int &tileCol, int &tileRow, std::string  &format, Style* &style, std::string& info_format, int& X, int& Y) {

    DataSource* getTileError = getTileParamWMTS(request, layer, tms, tm, tileCol, tileRow, format, style);

    
    if (getTileError) {
        return new DataStreamFromDataSource(getTileError);
    }
    if (layer->isGetFeatureInfoAvailable() == false) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Layer " +layer->getId()+" non interrogeable.","wmts" ) );   
    }

    
    // INFO_FORMAT
    info_format = request->getParam ( "infoformat" );
    if ( info_format == "" ){
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre INFOFORMAT vide.","wmts" ) );
    }else{
        if ( containForbiddenChars(info_format)) {
            BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in WMTS info_format: " << info_format ;
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Info_Format non gere par le service.","wmts" ) );
        }

        if ( ! servicesConf->isInInfoFormatList(info_format) )
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Info_Format " +info_format+" non gere par le service.","wmts" ) );
    }

    // X
    std::string strX = request->getParam ( "i" );
    if ( strX == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre I absent.","wmts" ) );
    char c;
    if (sscanf(strX.c_str(), "%d%c", &X, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre I n'est pas un entier.","wmts" ) );
    }
    if ( X<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre I est negative.","wmts" ) );
    if ( X> tm->getTileW()-1 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre I est superieure a la largeur de la tuile (width).","wmts" ) );


    // Y
    std::string strY = request->getParam ( "j" );
    if ( strY == "" )
        return new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,"Parametre J absent.","wmts" ) );

    if (sscanf(strY.c_str(), "%d%c", &Y, &c) != 1) {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre J n'est pas un entier.","wmts" ) );
    }
    if ( Y<0 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre J est negative.","wmts" ) );
    if ( Y> tm->getTileH()-1 )
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"La valeur du parametre J est superieure a la hauteur de la tuile (height).","wmts" ) );

    return NULL;
}
