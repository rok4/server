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

    ServicesConfiguration* services = serv->get_services_configuration();

    // On va mémoriser les TMS utilisés, avec les niveaux du haut et du bas
    // La clé est un triplet : nom du TMS, niveau du haut, niveau du bas
    std::map< std::string, WmtsTmsInfos> used_tms_list;

    ptree tree;

    ptree& root = tree.add("Capabilities", "");
    root.add("<xmlattr>.version", "1.0.0");
    root.add("<xmlattr>.xmlns","http://www.opengis.net/wmts/1.0" );
    root.add("<xmlattr>.xmlns:ows","http://www.opengis.net/ows/1.1" );
    root.add("<xmlattr>.xmlns:xlink","http://www.w3.org/1999/xlink" );
    root.add("<xmlattr>.xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    root.add("<xmlattr>.xmlns:gml","http://www.opengis.net/gml" );
    
    if ( req->is_inspire() ) {
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
    provider_node.add("ows:ProviderName", services->provider_site);
    provider_node.add("ows:ProviderSite.<xmlattr>.xlink:href", services->provider_site);

    services->contact->add_node_wmts(provider_node);

    ptree& op_getcapabilities = root.add("ows:OperationsMetadata.ows:Operation", "");
    op_getcapabilities.add("<xmlattr>.name", "GetCapabilities");
    op_getcapabilities.add("ows:DCP.ows:HTTP.ows:Get.<xmlattr>.xlink:href", endpoint_uri + "?SERVICE=WMTS&");
    op_getcapabilities.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.<xmlattr>.name", "GetEncoding");
    op_getcapabilities.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.ows:AllowedValues.ows:Value", "KVP");

    ptree& op_gettile = root.add("ows:OperationsMetadata.ows:Operation", "");
    op_gettile.add("<xmlattr>.name", "GetTile");
    op_gettile.add("ows:DCP.ows:HTTP.ows:Get.<xmlattr>.xlink:href", endpoint_uri + "?SERVICE=WMTS&");
    op_gettile.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.<xmlattr>.name", "GetEncoding");
    op_gettile.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.ows:AllowedValues.ows:Value", "KVP");

    ptree& op_getfeatureinfo = root.add("ows:OperationsMetadata.ows:Operation", "");
    op_getfeatureinfo.add("<xmlattr>.name", "GetFeatureInfo");
    op_getfeatureinfo.add("ows:DCP.ows:HTTP.ows:Get.<xmlattr>.xlink:href", endpoint_uri + "?SERVICE=WMTS&");
    op_getfeatureinfo.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.<xmlattr>.name", "GetEncoding");
    op_getfeatureinfo.add("ows:DCP.ows:HTTP.ows:Get.ows:Constraint.ows:AllowedValues.ows:Value", "KVP");

    if (req->is_inspire()) {
        ptree& inspire_extension = root.add("inspire_vs:ExtendedCapabilities", "");

        if (metadata) {
            inspire_extension.add("inspire_common:MetadataUrl.inspire_common:URL", metadata->get_href());
            inspire_extension.add("inspire_common:MetadataUrl.inspire_common:MediaType", metadata->get_type());
        }
        
        inspire_extension.add("inspire_common:SupportedLanguages.inspire_common:DefaultLanguage.inspire_common:Language", "fra");
        inspire_extension.add("inspire_common:ResponseLanguage.inspire_common:Language", "fra");
    }

    ptree& contents_node = root.add("Contents", "");

    std::map<std::string, Layer*>::iterator layers_iterator ( serv->get_server_configuration()->get_layers().begin() ), layers_end ( serv->get_server_configuration()->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        if (layers_iterator->second->is_wmts_enabled()) {
            Layer* layer = layers_iterator->second;
            ptree& layer_node = contents_node.add("Layer", "");
            layer_node.add("ows:Title", layer->title);
            layer_node.add("ows:Abstract", layer->abstract);

            if ( layer->keywords.size() != 0 ) {
                ptree& keywords_node = layer_node.add("ows:Keywords", "");
                for ( unsigned int i = 0; i < layer->keywords.size(); i++ ) {
                    keywords.at(i).add_node(keywords_node, "ows:Keyword");
                }
            }

            std::ostringstream os;
            os << layer->geographic_bbox.xmin << " " << layer->geographic_bbox.ymin;
            layer_node.add("ows:WGS84BoundingBox.ows:LowerCorner", os.str());
            os.str ( "" );
            os << layer->geographic_bbox.xmax << " " << layer->geographic_bbox.ymax;
            layer_node.add("ows:WGS84BoundingBox.ows:UpperCorner", os.str());
            
            layer_node.add("ows:Identifier", layer->id);

            for ( unsigned int i = 0; i < layer->styles.size(); i++ ) {
                ptree& style_node = layer_node.add("Style", "");
                if ( i == 0 ) {
                    style_node.add("<xmlattr>.isDefault", "true");
                }
                
                Style* style = layer->styles.at(i);
                for (int j = 0 ; j < style->get_titles().size(); ++j ) {
                    style_node.add("ows:Title", style->get_titles()[j]);
                }
                for (int j = 0 ; j < style->get_abstracts().size(); ++j ) {
                    style_node.add("ows:Abstract", style->get_abstracts()[j]);
                }

                if ( style->get_keywords()->size() != 0 ) {
                    ptree& style_keywords_node = style_node.add("ows:Keywords", "");
                    for ( unsigned int j = 0; j < style->get_keywords()->size(); j++ ) {
                        style->get_keywords()->at ( j ).add_node(style_keywords_node, "ows:Keyword");
                    }
                }

                style_node.add("ows:Identifier", style->get_identifier());
                for ( int j = 0 ; j < style->get_legends()->size(); j++ ) {
                    style->get_legends()->at(j).add_node_wmts(style_node);
                }
            }

            layer_node.add("Format", Rok4Format::to_mime_type ( layer->pyramid->get_format() ));

            if (layer->gfi_enabled){
                for ( unsigned int j = 0; j < info_formats.size(); j++ ) {
                    layer_node.add("InfoFormat", info_formats.at ( j ));
                }
            }

            if (reprojection) {
                // On ajoute les TMS disponibles avec les tuiles limites
                for ( unsigned int i = 0; i < layer->get_wmts_tilematrixsets().size(); i++ ) {
                    ptree& tms_node = layer_node.add("TileMatrixSetLink", "");
                    tms_node.add("TileMatrixSet", layer->get_wmts_tilematrixsets().at(i).wmts_id);

                    ptree& tms_limits_node = tms_node.add("TileMatrixSetLimits", "");
                    
                    // Niveaux
                    for ( unsigned int j = 0; j < layer->get_wmts_tilematrixsets().at(i).limits.size(); j++ ) { 
                        layer->get_wmts_tilematrixsets().at(i).limits.at(j).add_node(tms_limits_node);
                    }

                    used_tms_list.insert ( std::pair<std::string, WmtsTmsInfos> ( layer->get_wmts_tilematrixsets().at(i).wmts_id , layer->get_wmts_tilematrixsets().at(i)) );
                }
            }

            // On veut ajouter le TMS natif des données dans sa version originale
            WmtsTmsInfos origin_infos;
            origin_infos.tms = layer->get_pyramid()->get_tms();
            origin_infos.top_level = "";
            origin_infos.bottom_level = "";
            origin_infos.wmts_id = origin_infos.tms->get_id();
            used_tms_list.insert ( std::pair<std::string, WmtsTmsInfos> ( origin_infos.wmts_id, origin_infos) );
        }
    }


    std::map<std::string, WmtsTmsInfos>::iterator tms_iterator ( used_tms_list.begin() ), tms_end ( used_tms_list.end() );
    for ( ; tms_iterator!=tms_end; ++tms_iterator ) {

        ptree& tms_node = contents_node.add("TileMatrixSet", "");

        tms_node.add( "ows:Identifier", tms_iterator->first );

        TileMatrixSet* tms = tms_iterator->second.tms;

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
        std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> orderedTM = tms->get_ordered_tm(false);
        bool keep = false;
        if (tms_iterator->second.top_level == "") {
            // On est sur un TMS d'origine, on l'exporte en entier
            keep = true;
        }
        for (std::pair<std::string, TileMatrix*> element : orderedTM) {
            TileMatrix* tm = element.second;

            if (! keep && tm->get_id() != tms_iterator->second.top_level) {
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

            if (tm->get_id() == tms_iterator->second.bottom_level) {
                break;
            }
        }
    }

    std::stringstream ss;
    write_xml(ss, tree);
    return new MessageDataStream ( ss.str(), "text/xml", 200 );
}
