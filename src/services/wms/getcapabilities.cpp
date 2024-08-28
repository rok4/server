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
 * \file services/wms/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe WmsService
 ** \~english
 * \brief Implements classe WmsService
 */

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;

#include "services/wms/Exception.h"
#include "services/wms/Service.h"
#include "Rok4Server.h"

DataStream* WmsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    // IGNGPF-3548 : revoir les formats de GFI pour ne pas proposer ce que geoserver ne veut pas (pas application/xml mais text/xml)
    // Voir la même chose pour les formats d'image : est ce qu'on ne devrait pas les définir pour chaque couche

    // IGNGPF-3556 : ajouter aux URL des opérations ?SERVICE=WMS& : fait


    ServicesConfiguration* services = serv->get_services_configuration();

    // On va mémoriser les TMS utilisés, avec les niveaux du haut et du bas
    // La clé est un triplet : nom du TMS, niveau du haut, niveau du bas
    std::map< std::string, WmtsTmsInfos> used_tms_list;

    ptree tree;

    ptree& root = tree.add("WMS_Capabilities", "");
    root.add("<xmlattr>.version", "1.3.0");
    root.add("<xmlattr>.xmlns","http://www.opengis.net/wms" );
    root.add("<xmlattr>.xmlns:xlink","http://www.w3.org/1999/xlink" );
    root.add("<xmlattr>.xmlns:xsi","http://www.w3.org/2001/XMLSchema-instance" );
    
    if ( req->is_inspire() ) {
        root.add("<xmlattr>.xmlns:inspire_common","http://inspire.ec.europa.eu/schemas/common/1.0" );
        root.add("<xmlattr>.xmlns:inspire_vs","http://inspire.ec.europa.eu/schemas/inspire_vs_ows11/1.0" );
        root.add("<xmlattr>.xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd http://inspire.ec.europa.eu/schemas/inspire_vs/1.0 http://inspire.ec.europa.eu/schemas/inspire_vs/1.0/inspire_vs.xsd http://inspire.ec.europa.eu/schemas/common/1.0 http://inspire.ec.europa.eu/schemas/common/1.0/common.xsd" );
    } else {
        root.add("<xmlattr>.xsi:schemaLocation","http://www.opengis.net/wms http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd" );
    }

    ptree& service_node = root.add("Service", "");
    service_node.add("Name", name);
    service_node.add("Title", title);
    service_node.add("Abstract", abstract);
    if ( keywords.size() != 0 ) {
        ptree& keywords_node = service_node.add("KeywordList", "");
        for ( unsigned int i=0; i < keywords.size(); i++ ) {
            keywords.at(i).add_node(keywords_node, "Keyword");
        }
    }

    service_node.add("OnlineResource.<xmlattr>.xmlns:xlink", "http://www.w3.org/1999/xlink");
    service_node.add("OnlineResource.<xmlattr>.xlink:href", endpoint_uri);

    services->contact->add_node_wms(service_node, services->service_provider);

    service_node.add("Fees", services->fee);
    service_node.add("AccessConstraints", services->access_constraint);
    service_node.add("LayerLimit", max_layers_count);
    service_node.add("MaxWidth", max_width);
    service_node.add("MaxHeight", max_height);

    ptree& capability_node = root.add("Capability", "");

    ptree& op_getcapabilities = capability_node.add("Request.GetCapabilities", "");
    op_getcapabilities.add("Format", "text/xml");
    op_getcapabilities.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xlink:href", endpoint_uri + "?SERVICE=WMS&");
    op_getcapabilities.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xmlns:xlink", "http://www.w3.org/1999/xlink");
    op_getcapabilities.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xlink:type", "simple");

    ptree& op_getmap = capability_node.add("Request.GetMap", "");
    for ( unsigned int i = 0; i < formats.size(); i++ ) {
        op_getmap.add("Format", formats.at(i));
    }
    op_getmap.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xlink:href", endpoint_uri + "?SERVICE=WMS&");
    op_getmap.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xmlns:xlink", "http://www.w3.org/1999/xlink");
    op_getmap.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xlink:type", "simple");

    ptree& op_getfeatureinfo = capability_node.add("Request.GetFeatureInfo", "");
    for ( unsigned int i = 0; i < info_formats.size(); i++ ) {
        op_getfeatureinfo.add("Format", info_formats.at(i));
    }
    op_getfeatureinfo.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xlink:href", endpoint_uri + "?SERVICE=WMS&");
    op_getfeatureinfo.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xmlns:xlink", "http://www.w3.org/1999/xlink");
    op_getfeatureinfo.add("DCPType.HTTP.Get.OnlineResource.<xmlattr>.xlink:type", "simple");

    capability_node.add("Exception.Format", "XML");

    if (req->is_inspire()) {
        ptree& inspire_extension = capability_node.add("inspire_vs:ExtendedCapabilities", "");

        if (metadata) {
            inspire_extension.add("inspire_common:MetadataUrl.inspire_common:URL", metadata->get_href());
            inspire_extension.add("inspire_common:MetadataUrl.inspire_common:MediaType", metadata->get_type());
        }
        
        inspire_extension.add("inspire_common:SupportedLanguages.inspire_common:DefaultLanguage.inspire_common:Language", "fra");
        inspire_extension.add("inspire_common:ResponseLanguage.inspire_common:Language", "fra");
    }

    ptree& contents_node = capability_node.add("Layer", "");

    contents_node.add("Title", root_layer_title);
    contents_node.add("Abstract", root_layer_abstract);

    for ( unsigned int i = 0; i < crss.size(); i++ ) {
        contents_node.add("CRS", crss.at(i)->get_request_code());
    }

    BoundingBox<double> gbbox ( -180.0,-90.0,180.0,90.0 );
    gbbox.add_node(contents_node, true, true);
    gbbox.crs = "CRS:84";
    gbbox.add_node(contents_node, false, false);

    std::map<std::string, Layer*>::iterator layers_iterator ( serv->get_server_configuration()->get_layers().begin() ), layers_end ( serv->get_server_configuration()->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        if (layers_iterator->second->is_wms_enabled()) {
            Layer* layer = layers_iterator->second;

            ptree& layer_node = contents_node.add("Layer", "");
            if (layer->is_gfi_enabled()) {
                layer_node.add("<xmlattr>.queryable", "1");
            }

            layer_node.add("Name", layer->get_id());
            layer_node.add("Title", layer->get_title());
            layer_node.add("Abstract", layer->get_abstract());

            if ( layer->get_keywords()->size() != 0 ) {
                ptree& keywords_node = layer_node.add("KeywordList", "");
                for ( unsigned int i = 0; i < layer->get_keywords()->size(); i++ ) {
                    keywords.at(i).add_node(keywords_node, "Keyword");
                }
            }

            for ( unsigned int i = 0; i < layer->get_wms_crss()->size(); i++ ) {
                layer_node.add("CRS", layer->get_wms_crss()->at(i)->get_request_code());
            }

            layer->get_geographical_bbox().add_node(layer_node, true, true);


            // BoundingBox
            if ( req->is_inspire() ) {
                for ( unsigned int i = 0; i < layer->get_wms_crss()->size(); i++ ) {
                    CRS* crs = layer->get_wms_crss()->at(i);
                    BoundingBox<double> bbox ( 0,0,0,0 );
                    if ( layer->get_geographical_bbox().is_in_crs_area(crs)) {
                        bbox = layer->get_geographical_bbox();
                    } else {
                        bbox = layer->get_geographical_bbox().crop_to_crs_area(crs);
                    }

                    bbox.reproject(CRS::get_epsg4326(), crs);
                    bbox.add_node(layer_node, false, crs->is_lat_lon() );
                }
                for ( unsigned int i = 0; i < crss.size(); i++ ) {
                    CRS* crs = crss.at(i);
                    BoundingBox<double> bbox ( 0,0,0,0 );
                    if ( layer->get_geographical_bbox().is_in_crs_area(crs)) {
                        bbox = layer->get_geographical_bbox();
                    } else {
                        bbox = layer->get_geographical_bbox().crop_to_crs_area(crs);
                    }

                    bbox.reproject(CRS::get_epsg4326(), crs);
                    bbox.add_node(layer_node, false, crs->is_lat_lon() );
                }
            } else {
                BoundingBox<double> bbox = layer->get_native_bbox();
                CRS* crs = layer->get_pyramid()->get_tms()->get_crs();
                bbox.add_node(layer_node, false, crs->is_lat_lon() );
            }

            if (layer->get_attribution() != NULL) {
                layer->get_attribution()->add_node_wms(layer_node);
            }
            
            for ( unsigned int i = 0; i < layer->get_metadata()->size(); ++i ) {
                layer->get_metadata()->at(i).add_node_wms(layer_node);
            }

            for ( unsigned int i = 0; i < layer->get_styles().size(); i++ ) {
                ptree& style_node = layer_node.add("Style", "");
                Style* style = layer->get_styles().at(i);

                style_node.add("Name", style->get_identifier());

                for (int j = 0 ; j < style->get_titles().size(); ++j ) {
                    style_node.add("Title", style->get_titles()[j]);
                }
                for (int j = 0 ; j < style->get_abstracts().size(); ++j ) {
                    style_node.add("Abstract", style->get_abstracts()[j]);
                }

                for (int j = 0 ; j < style->get_legends()->size(); ++j ) {
                    style->get_legends()->at(j).add_node_wms(style_node);
                }
            }

            layer_node.add("MinScaleDenominator", layer->get_pyramid()->get_lowest_level()->get_res() * 1000 / 0.28);
            layer_node.add("MaxScaleDenominator", layer->get_pyramid()->get_highest_level()->get_res() * 1000 / 0.28);
        }
    }

    std::stringstream ss;
    write_xml(ss, tree);
    return new MessageDataStream ( ss.str(), "text/xml", 200 );

}
