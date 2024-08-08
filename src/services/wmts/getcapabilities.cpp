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
 * \file services/wmts/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe WmtsService
 ** \~english
 * \brief Implements classe WmtsService
 */

#include <iostream>

#include "services/wmts/Exception.h"
#include "services/wmts/Service.h"
#include "Rok4Server.h"

DataStream* WmtsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    ServicesConfiguration* services = serv->get_services_configuration();

    // On va mémoriser les TMS utilisés, avec les niveaux du haut et du bas
    // La clé est un triplet : nom du TMS, niveau du haut, niveau du bas
    std::map< std::string, WmtsTmsInfos> used_tms_list;

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
    
    // À activer selon un paramètre de requête
    // if ( "inspire" ) {
    //     capabilitiesEl->SetAttribute ( "xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
    //     capabilitiesEl->SetAttribute ( "xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0" );
    //     capabilitiesEl->SetAttribute ( "xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0/inspire_vs_ows_11.xsd" );
    // }


    //----------------------------------------------------------------------
    // ServiceIdentification
    //----------------------------------------------------------------------
    TiXmlElement * serviceEl = new TiXmlElement ( "ows:ServiceIdentification" );

    serviceEl->LinkEndChild ( Utils::build_text_node ( "ows:Title", title ) );
    serviceEl->LinkEndChild ( Utils::build_text_node ( "ows:Abstract", abstract ) );
    //KeywordList
    if ( keywords.size() != 0 ) {
        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
        for ( unsigned int i=0; i < keywords.size(); i++ ) {
            kwlEl->LinkEndChild ( Utils::get_xml("ows:Keyword", keywords.at ( i )) );
        }
        serviceEl->LinkEndChild ( kwlEl );
    }
    serviceEl->LinkEndChild ( Utils::build_text_node ( "ows:ServiceType", "OGC WMTS" ) );
    serviceEl->LinkEndChild ( Utils::build_text_node ( "ows:ServiceTypeVersion", "1.0.0" ) );
    serviceEl->LinkEndChild ( Utils::build_text_node ( "ows:Fees", services->fee ) );
    serviceEl->LinkEndChild ( Utils::build_text_node ( "ows:AccessConstraints", services->access_constraint ) );


    capabilitiesEl->LinkEndChild ( serviceEl );

    //----------------------------------------------------------------------
    // service_provider (facultatif)
    //----------------------------------------------------------------------
    TiXmlElement * serviceProviderEl = new TiXmlElement ( "ows:ServiceProvider" );

    serviceProviderEl->LinkEndChild ( Utils::build_text_node ( "ows:ProviderName",services->service_provider ) );
    TiXmlElement * providerSiteEl = new TiXmlElement ( "ows:ProviderSite" );
    providerSiteEl->SetAttribute ( "xlink:href",services->provider_site );
    serviceProviderEl->LinkEndChild ( providerSiteEl );

    TiXmlElement * serviceContactEl = new TiXmlElement ( "ows:ServiceContact" );

    serviceContactEl->LinkEndChild ( Utils::build_text_node ( "ows:IndividualName",services->individual_name ) );
    serviceContactEl->LinkEndChild ( Utils::build_text_node ( "ows:PositionName",services->individual_position ) );

    TiXmlElement * contactInfoEl = new TiXmlElement ( "ows:ContactInfo" );
    TiXmlElement * contactInfoPhoneEl = new TiXmlElement ( "ows:Phone" );

    contactInfoPhoneEl->LinkEndChild ( Utils::build_text_node ( "ows:Voice",services->voice ) );
    contactInfoPhoneEl->LinkEndChild ( Utils::build_text_node ( "ows:Facsimile",services->facsimile ) );

    contactInfoEl->LinkEndChild ( contactInfoPhoneEl );

    TiXmlElement * contactAddressEl = new TiXmlElement ( "ows:Address" );
    contactAddressEl->LinkEndChild ( Utils::build_text_node ( "ows:DeliveryPoint",services->delivery_point ) );
    contactAddressEl->LinkEndChild ( Utils::build_text_node ( "ows:City",services->city ) );
    contactAddressEl->LinkEndChild ( Utils::build_text_node ( "ows:AdministrativeArea",services->administrative_area ) );
    contactAddressEl->LinkEndChild ( Utils::build_text_node ( "ows:PostalCode",services->post_code ) );
    contactAddressEl->LinkEndChild ( Utils::build_text_node ( "ows:Country",services->country ) );
    contactAddressEl->LinkEndChild ( Utils::build_text_node ( "ows:ElectronicMailAddress",services->email ) );
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
    getEl->SetAttribute ( "xlink:href", endpoint_uri + "?SERVICE=WMTS&" );
    TiXmlElement * constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    TiXmlElement * allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( Utils::build_text_node ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );

    opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetTile" );
    dcpEl = new TiXmlElement ( "ows:DCP" );
    httpEl = new TiXmlElement ( "ows:HTTP" );
    getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href", endpoint_uri + "?SERVICE=WMTS&" );
    constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( Utils::build_text_node ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );
    
    opEl = new TiXmlElement ( "ows:Operation" );
    opEl->SetAttribute ( "name","GetFeatureInfo" );
    dcpEl = new TiXmlElement ( "ows:DCP" );
    httpEl = new TiXmlElement ( "ows:HTTP" );
    getEl = new TiXmlElement ( "ows:Get" );
    getEl->SetAttribute ( "xlink:href", endpoint_uri + "?SERVICE=WMTS&" );
    constraintEl = new TiXmlElement ( "ows:Constraint" );
    constraintEl->SetAttribute ( "name","GetEncoding" );
    allowedValuesEl = new TiXmlElement ( "ows:AllowedValues" );
    allowedValuesEl->LinkEndChild ( Utils::build_text_node ( "ows:Value", "KVP" ) );
    constraintEl->LinkEndChild ( allowedValuesEl );
    getEl->LinkEndChild ( constraintEl );
    httpEl->LinkEndChild ( getEl );

    dcpEl->LinkEndChild ( httpEl );
    opEl->LinkEndChild ( dcpEl );

    opMtdEl->LinkEndChild ( opEl );


    // Inspire (extended Capability)
    // TODO : en dur. A mettre dans la configuration du service (prevoir differents profils d'application possibles)
    if (metadata) {
        TiXmlElement * extendedCapabilititesEl = new TiXmlElement ( "inspire_vs:ExtendedCapabilities" );

        // Metadata
        TiXmlElement * metadataUrlEl = new TiXmlElement ( "inspire_common:MetadataUrl" );
        metadataUrlEl->LinkEndChild ( Utils::build_text_node ( "inspire_common:URL", metadata->get_href() ) );
        metadataUrlEl->LinkEndChild ( Utils::build_text_node ( "inspire_common:MediaType", metadata->get_type() ) );
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
        responseLanguageEl->LinkEndChild ( Utils::build_text_node ( "inspire_common:Language","fre" ) );
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
    std::map<std::string, Layer*>::iterator itLay ( serv->get_server_configuration()->get_layers().begin() ), itLayEnd ( serv->get_server_configuration()->get_layers().end() );
    for ( ; itLay!=itLayEnd; ++itLay ) {
        //Look if the layer is published in WMTS
        if (itLay->second->is_wmts_enabled()) {
            TiXmlElement * layerEl=new TiXmlElement ( "Layer" );
            Layer* layer = itLay->second;

            layerEl->LinkEndChild ( Utils::build_text_node ( "ows:Title", layer->get_title() ) );
            layerEl->LinkEndChild ( Utils::build_text_node ( "ows:Abstract", layer->get_abstract() ) );
            if ( layer->get_keywords()->size() != 0 ) {
                TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
                for ( unsigned int i=0; i < layer->get_keywords()->size(); i++ ) {
                    kwlEl->LinkEndChild ( Utils::get_xml("ows:Keyword", layer->get_keywords()->at ( i )) );
                }
                layerEl->LinkEndChild ( kwlEl );
            }
            //ows:WGS84BoundingBox (0,n)


            TiXmlElement * wgsBBEl = new TiXmlElement ( "ows:WGS84BoundingBox" );
            std::ostringstream os;
            os.str ( "" );
            os<<layer->get_geographical_bbox().xmin;
            os<<" ";
            os<<layer->get_geographical_bbox().ymin;
            wgsBBEl->LinkEndChild ( Utils::build_text_node ( "ows:LowerCorner", os.str() ) );
            os.str ( "" );
            os<<layer->get_geographical_bbox().xmax;
            os<<" ";
            os<<layer->get_geographical_bbox().ymax;
            wgsBBEl->LinkEndChild ( Utils::build_text_node ( "ows:UpperCorner", os.str() ) );
            os.str ( "" );
            layerEl->LinkEndChild ( wgsBBEl );


            layerEl->LinkEndChild ( Utils::build_text_node ( "ows:Identifier", layer->get_id() ) );

            //Style
            if ( layer->get_styles().size() != 0 ) {
                for ( unsigned int i=0; i < layer->get_styles().size(); i++ ) {
                    TiXmlElement * styleEl= new TiXmlElement ( "Style" );
                    if ( i==0 ) styleEl->SetAttribute ( "isDefault","true" );
                    Style* style = layer->get_styles() [i];
                    int j;
                    for ( j=0 ; j < style->get_titles().size(); ++j ) {
                        BOOST_LOG_TRIVIAL(debug) <<  "Title : " << style->get_titles() [j].c_str()  ;
                        styleEl->LinkEndChild ( Utils::build_text_node ( "ows:Title", style->get_titles() [j].c_str() ) );
                    }
                    for ( j=0 ; j < style->get_abstracts().size(); ++j ) {
                        BOOST_LOG_TRIVIAL(debug) <<  "Abstract : " << style->get_abstracts() [j].c_str()  ;
                        styleEl->LinkEndChild ( Utils::build_text_node ( "ows:Abstract", style->get_abstracts() [j].c_str() ) );
                    }

                    if ( style->get_keywords()->size() != 0 ) {
                        TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
                        for ( unsigned int i=0; i < style->get_keywords()->size(); i++ ) {
                            kwlEl->LinkEndChild ( Utils::get_xml("ows:Keyword", style->get_keywords()->at ( i )) );
                        }
                        styleEl->LinkEndChild ( kwlEl );
                    }

                    styleEl->LinkEndChild ( Utils::build_text_node ( "ows:Identifier", style->get_identifier() ) );
                    for ( j=0 ; j < style->get_legends().size(); ++j ) {
                        LegendURL legendURL = style->get_legends() [j];
                        TiXmlElement* legendURLEl = new TiXmlElement ( "LegendURL" );
                        legendURLEl->SetAttribute ( "format", legendURL.get_format() );
                        legendURLEl->SetAttribute ( "xlink:href", legendURL.get_href() );
                        if ( legendURL.get_width() !=0 )
                            legendURLEl->SetAttribute ( "width", legendURL.get_width() );
                        if ( legendURL.get_height() !=0 )
                            legendURLEl->SetAttribute ( "height", legendURL.get_height() );
                        if ( legendURL.get_min_scale_denominator() !=0.0 )
                            legendURLEl->SetAttribute ( "minScaleDenominator", legendURL.get_min_scale_denominator() );
                        if ( legendURL.get_max_scale_denominator() !=0.0 )
                            legendURLEl->SetAttribute ( "maxScaleDenominator", legendURL.get_max_scale_denominator() );
                        styleEl->LinkEndChild ( legendURLEl );
                    }
                    layerEl->LinkEndChild ( styleEl );
                }
            }

            // Contrainte : 1 layer = 1 pyramide = 1 format
            layerEl->LinkEndChild ( Utils::build_text_node ( "Format",Rok4Format::to_mime_type ( ( layer->get_pyramid()->get_format() ) ) ) );
            if (layer->is_gfi_enabled()){
                for ( unsigned int i = 0; i < info_formats.size(); i++ ) {
                    layerEl->LinkEndChild ( Utils::build_text_node ( "InfoFormat", info_formats.at ( i ) ) );
                }
            }

            if (reprojection) {
                // On ajoute les TMS disponibles avec les tuiles limites
                for ( unsigned int i=0; i < layer->get_wmts_tilematrixsets().size(); i++ ) {
                    TiXmlElement * tmsLinkEl = new TiXmlElement ( "TileMatrixSetLink" );
                    tmsLinkEl->LinkEndChild ( Utils::build_text_node ( "TileMatrixSet", layer->get_wmts_tilematrixsets().at(i).wmts_id ) );
                    
                    TiXmlElement * tmsLimitsEl = new TiXmlElement ( "TileMatrixSetLimits" );

                    // Niveaux
                    for ( unsigned int j = 0; j < layer->get_wmts_tilematrixsets().at(i).limits.size(); j++ ) { 
                        tmsLimitsEl->LinkEndChild ( Utils::get_xml(layer->get_wmts_tilematrixsets().at(i).limits.at(j)) );
                    }

                    tmsLinkEl->LinkEndChild ( tmsLimitsEl );

                    layerEl->LinkEndChild ( tmsLinkEl );
                    used_tms_list.insert ( std::pair<std::string, WmtsTmsInfos> ( layer->get_wmts_tilematrixsets().at(i).wmts_id , layer->get_wmts_tilematrixsets().at(i)) );


                    WmtsTmsInfos origin_infos;
                    origin_infos.tms = layer->get_wmts_tilematrixsets().at(i).tms;
                    origin_infos.top_level = "";
                    origin_infos.bottom_level = "";
                    origin_infos.wmts_id = origin_infos.tms->get_id();
                    used_tms_list.insert ( std::pair<std::string, WmtsTmsInfos> ( origin_infos.wmts_id, origin_infos) );
                }
            }

            contentsEl->LinkEndChild ( layerEl );
        }

    }

    // TileMatrixSet
    //--------------------------------------------------------
    std::map<std::string, WmtsTmsInfos>::iterator itTms ( used_tms_list.begin() ), itTmsEnd ( used_tms_list.end() );
    for ( ; itTms!=itTmsEnd; ++itTms ) {

        TiXmlElement * tmsEl = new TiXmlElement ( "TileMatrixSet" );
        tmsEl->LinkEndChild ( Utils::build_text_node ( "ows:Identifier", itTms->first ) );

        TileMatrixSet* tms = itTms->second.tms;
        if ( ! ( tms->get_title().empty() ) ) {
            tmsEl->LinkEndChild ( Utils::build_text_node ( "ows:Title", tms->get_title().c_str() ) );
        }

        if ( ! ( tms->get_abstract().empty() ) ) {
            tmsEl->LinkEndChild ( Utils::build_text_node ( "ows:Abstract", tms->get_abstract().c_str() ) );
        }

        if ( tms->get_keywords()->size() != 0 ) {

            TiXmlElement * kwlEl = new TiXmlElement ( "ows:Keywords" );
            for ( unsigned int i=0; i < tms->get_keywords()->size(); i++ ) {
                kwlEl->LinkEndChild ( Utils::get_xml("ows:Keyword", tms->get_keywords()->at ( i )) );
            }
            tmsEl->LinkEndChild ( kwlEl );

        }

        tmsEl->LinkEndChild ( Utils::build_text_node ( "ows:SupportedCRS",tms->get_crs()->get_request_code() ) );
        
        // TileMatrix
        std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> orderedTM = tms->get_ordered_tm(false);
        bool keep = false;
        if (itTms->second.top_level == "") {
            // On est sur un TMS d'origine, on l'exporte en entier
            keep = true;
        }
        for (std::pair<std::string, TileMatrix*> element : orderedTM) {
            TileMatrix* tm = element.second;

            if (! keep && tm->get_id() != itTms->second.top_level) {
                continue;
            } else {
                keep = true;
            }

            TiXmlElement * tmEl = new TiXmlElement ( "TileMatrix" );
            tmEl->LinkEndChild ( Utils::build_text_node ( "ows:Identifier",tm->get_id() ) );
            tmEl->LinkEndChild ( Utils::build_text_node ( "ScaleDenominator",Utils::double_to_string ( ( long double ) ( tm->get_res() * tms->get_crs()->gte_meters_per_unit() ) /0.00028 ) ) );
            if (tms->get_crs()->get_authority() == "EPSG" && tms->get_crs()->is_lon_lat()) {
                tmEl->LinkEndChild ( Utils::build_text_node ( "TopLeftCorner",Utils::double_to_string ( tm->get_y0() ) + " " + Utils::double_to_string ( tm->get_x0() ) ) );
            } else {
                tmEl->LinkEndChild ( Utils::build_text_node ( "TopLeftCorner",Utils::double_to_string ( tm->get_x0() ) + " " + Utils::double_to_string ( tm->get_y0() ) ) );
            }
            tmEl->LinkEndChild ( Utils::build_text_node ( "TileWidth",Utils::int_to_string ( tm->get_tile_width() ) ) );
            tmEl->LinkEndChild ( Utils::build_text_node ( "TileHeight",Utils::int_to_string ( tm->get_tile_height() ) ) );
            tmEl->LinkEndChild ( Utils::build_text_node ( "MatrixWidth",Utils::int_to_string ( tm->get_matrix_width() ) ) );
            tmEl->LinkEndChild ( Utils::build_text_node ( "MatrixHeight",Utils::int_to_string ( tm->get_matrix_height() ) ) );
            tmsEl->LinkEndChild ( tmEl );

            if (tm->get_id() == itTms->second.bottom_level) {
                break;
            }
        }

        contentsEl->LinkEndChild ( tmsEl );
    }

    capabilitiesEl->LinkEndChild ( contentsEl );
    doc.LinkEndChild ( capabilitiesEl );

    std::string res;
    res << doc;  // ecriture non formatée dans un std::string
    doc.Clear();

    return new MessageDataStream ( res, "application/xml", 200 );
}
