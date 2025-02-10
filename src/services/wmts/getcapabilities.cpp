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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;

#include "services/wmts/Exception.h"
#include "services/wmts/Service.h"
#include "Rok4Server.h"

DataStream* WmtsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    if ( req->is_inspire(default_inspire) && ! cache_getcapabilities_inspire.empty()) {
        return new MessageDataStream ( cache_getcapabilities_inspire, "text/xml", 200 );
    }
    else if (! req->is_inspire(default_inspire) && ! cache_getcapabilities.empty()) {
        return new MessageDataStream ( cache_getcapabilities, "text/xml", 200 );
    }

    ServicesConfiguration* services = serv->get_services_configuration();

    // On va mémoriser les TMS utilisés, avec les niveaux du haut et du bas
    // La clé est un triplet : nom du TMS, niveau du haut, niveau du bas
    std::map< std::string, TileMatrixSetInfos*> used_tms_list;

    ptree tree;

    ptree& root = tree.add("Capabilities", "");
    root.add("<xmlattr>.version", "1.0.0");
    root.add("<xmlattr>.xmlns","http://www.opengis.net/wmts/1.0" );
    root.add("<xmlattr>.xmlns:ows","http://www.opengis.net/ows/1.1" );
    root.add("<xmlattr>.xmlns:xlink","http://www.w3.org/1999/xlink" );
    root.add("<xmlattr>.xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    root.add("<xmlattr>.xmlns:gml","http://www.opengis.net/gml" );
    
    if ( req->is_inspire(default_inspire) ) {
        root.add("<xmlattr>.xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        root.add("<xmlattr>.xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0" );
        root.add("<xmlattr>.xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0/inspire_vs_ows_11.xsd" );
    } else {
        root.add("<xmlattr>.xsi:schemaLocation","http://www.opengis.net/wmts/1.0 http://schemas.opengis.net/wmts/1.0/wmtsGetCapabilities_response.xsd" );
    }

    ptree& identification_node = root.add("ows:ServiceIdentification", "");
    identification_node.add("ows:Title", title);
    identification_node.add("ows:Abstract", abstract);
    if ( keywords.size() != 0 ) {
        ptree& keywords_node = identification_node.add("ows:Keywords", "");
        for ( unsigned int i=0; i < keywords.size(); i++ ) {
            keywords.at(i).add_node(keywords_node, "ows:Keyword");
        }
    }

    identification_node.add("ows:ServiceType", "OGC WMTS");
    identification_node.add("ows:ServiceTypeVersion", "1.0.0");
    identification_node.add("ows:Fees", services->fee);
    identification_node.add("ows:AccessConstraints", services->access_constraint);

    ptree& provider_node = root.add("ows:ServiceProvider", "");
    provider_node.add("ows:ProviderName", services->service_provider);
    provider_node.add("ows:ProviderSite.<xmlattr>.xlink:href", services->provider_site);

    services->contact->add_node_wmts(provider_node);

    std::string additionnal_params = "?SERVICE=WMTS&";
    if (req->is_inspire(default_inspire)) {
        additionnal_params = "?INSPIRE=1&SERVICE=WMTS&";
    }

    ptree& op_getcapabilities = root.add("ows:OperationsMetadata.ows:Operation", "");
    op_getcapabilities.add("<xmlattr>.name", "GetCapabilities");
    op_getcapabilities.add("ows:DCP.ows:HTTP.ows:Get.<xmlattr>.xlink:href", endpoint_uri + additionnal_params);
    op_getcapabilities.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.<xmlattr>.name", "GetEncoding");
    op_getcapabilities.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.ows:AllowedValues.ows:Value", "KVP");

    ptree& op_gettile = root.add("ows:OperationsMetadata.ows:Operation", "");
    op_gettile.add("<xmlattr>.name", "GetTile");
    op_gettile.add("ows:DCP.ows:HTTP.ows:Get.<xmlattr>.xlink:href", endpoint_uri + additionnal_params);
    op_gettile.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.<xmlattr>.name", "GetEncoding");
    op_gettile.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.ows:AllowedValues.ows:Value", "KVP");

    ptree& op_getfeatureinfo = root.add("ows:OperationsMetadata.ows:Operation", "");
    op_getfeatureinfo.add("<xmlattr>.name", "GetFeatureInfo");
    op_getfeatureinfo.add("ows:DCP.ows:HTTP.ows:Get.<xmlattr>.xlink:href", endpoint_uri + additionnal_params);
    op_getfeatureinfo.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.<xmlattr>.name", "GetEncoding");
    op_getfeatureinfo.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.ows:AllowedValues.ows:Value", "KVP");

    if (req->is_inspire(default_inspire)) {
        ptree& inspire_extension = root.add("ows:OperationsMetadata.inspire_vs:ExtendedCapabilities", "");

        if (metadata) {
            inspire_extension.add("inspire_common:MetadataUrl.inspire_common:URL", metadata->get_href());
            inspire_extension.add("inspire_common:MetadataUrl.inspire_common:MediaType", metadata->get_type());
        }
        
        inspire_extension.add("inspire_common:SupportedLanguages.inspire_common:DefaultLanguage.inspire_common:Language", "fre");
        inspire_extension.add("inspire_common:ResponseLanguage.inspire_common:Language", "fre");
    }

    ptree& contents_node = root.add("Contents", "");

    std::map<std::string, Layer*>::iterator layers_iterator ( serv->get_server_configuration()->get_layers().begin() ), layers_end ( serv->get_server_configuration()->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        layers_iterator->second->add_node_wmts(contents_node, this, req->is_inspire(default_inspire), &used_tms_list);
    }

    std::map<std::string, TileMatrixSetInfos*>::iterator tms_iterator ( used_tms_list.begin() ), tms_end ( used_tms_list.end() );
    for ( ; tms_iterator!=tms_end; ++tms_iterator ) {

        ptree& tms_node = contents_node.add("TileMatrixSet", "");

        tms_node.add( "ows:Identifier", tms_iterator->first );

        TileMatrixSet* tms = tms_iterator->second->tms;

        if ( ! ( tms->get_title().empty() ) ) {
            tms_node.add ( "ows:Title", tms->get_title() );
        }

        if ( ! ( tms->get_abstract().empty() ) ) {
            tms_node.add ( "ows:Abstract", tms->get_abstract() );
        }

        if ( tms->get_keywords()->size() != 0 ) {
            ptree& tms_keywords_node = tms_node.add("ows:Keywords", "");
            for ( unsigned int j = 0; j < tms->get_keywords()->size(); j++ ) {
                tms->get_keywords()->at ( j ).add_node(tms_keywords_node, "ows:Keyword");
            }
        }

        tms_node.add ( "ows:SupportedCRS",tms->get_crs()->get_request_code() );
        
        // TileMatrix
        bool keep = false;
        if (tms_iterator->second->top_level == "") {
            // On est sur un TMS d'origine, on l'exporte en entier
            keep = true;
        }
        for (TileMatrix* tm : tms->get_ordered_tm(false)) {

            if (! keep && tm->get_id() != tms_iterator->second->top_level) {
                continue;
            } else {
                keep = true;
            }

            ptree& tm_node = tms_node.add("TileMatrix", "");

            tm_node.add( "ows:Identifier",tm->get_id() );
            tm_node.add( "ScaleDenominator",Utils::double_to_string ( ( long double ) ( tm->get_res() * tms->get_crs()->get_meters_per_unit() ) /0.00028 ) );
            if (tms->get_crs()->get_authority() == "EPSG" && tms->get_crs()->is_geographic()) {
                tm_node.add ( "TopLeftCorner", Utils::double_to_string ( tm->get_y0() ) + " " + Utils::double_to_string ( tm->get_x0() ) );
            } else {
                tm_node.add ( "TopLeftCorner", Utils::double_to_string ( tm->get_x0() ) + " " + Utils::double_to_string ( tm->get_y0() ) );
            }
            tm_node.add( "TileWidth",Utils::int_to_string ( tm->get_tile_width() ) );
            tm_node.add("TileHeight",Utils::int_to_string ( tm->get_tile_height() ) );
            tm_node.add( "MatrixWidth",Utils::int_to_string ( tm->get_matrix_width() ) );
            tm_node.add( "MatrixHeight",Utils::int_to_string ( tm->get_matrix_height() ) );

            if (tm->get_id() == tms_iterator->second->bottom_level) {
                break;
            }
        }

        if (tms_iterator->second->top_level == "") {
            // On est sur un TMS d'origine, TileMatrixSetInfos créé pour cette occasion, on doit le nettoyer
            delete tms_iterator->second;
        }
    }

    std::stringstream ss;
    write_xml(ss, tree);
    cache_mtx.lock();
    if (req->is_inspire(default_inspire)) {
        cache_getcapabilities_inspire = ss.str();
    } else {
        cache_getcapabilities = ss.str();
    }
    cache_mtx.unlock();
    return new MessageDataStream ( ss.str(), "text/xml", 200 );
}
