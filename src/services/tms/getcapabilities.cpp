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
 * \file services/tms/getcapabilities.cpp
 ** \~french
 * \brief Implémentation de la classe TmsService
 ** \~english
 * \brief Implements classe TmsService
 */

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;

#include <rok4/thirdparty/json11.hpp>

#include "services/tms/Exception.h"
#include "services/tms/Service.h"
#include "Rok4Server.h"

DataStream* TmsService::get_capabilities ( Request* req, Rok4Server* serv ) {

    if ( req->path_params.at(0) != "1.0.0" ) {
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);
    }

    if (! cache_getcapabilities.empty()) {
        return new MessageDataStream ( cache_getcapabilities, "text/xml", 200 );
    }

    ptree tree;

    ptree& root = tree.add("TileMapService", "");
    root.add("<xmlattr>.version", "1.0.0");
    root.add("<xmlattr>.services", endpoint_uri );
    root.add("Title", title );
    root.add("Abstract", abstract );

    for ( unsigned int i=0; i < keywords.size(); i++ ) {
        root.add("KeywordList", keywords.at(i).get_content() );
    }

    serv->get_services_configuration()->contact->add_node_tms(root, serv->get_services_configuration()->service_provider);

    if (metadata) {
        metadata->add_node_tms(root);
    }

    ptree& contents_node = root.add("TileMaps", "");

    std::map<std::string, Layer*>::iterator layers_iterator ( serv->get_server_configuration()->get_layers().begin() ), layers_end ( serv->get_server_configuration()->get_layers().end() );
    for ( ; layers_iterator != layers_end; ++layers_iterator ) {
        layers_iterator->second->add_node_tms(contents_node, this);
    }

    std::stringstream ss;
    write_xml(ss, tree);
    cache_mtx.lock();
    cache_getcapabilities = ss.str();
    cache_mtx.unlock();
    return new MessageDataStream ( ss.str(), "text/xml", 200 );
}

DataStream* TmsService::get_tiles ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->path_params.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->path_params.at(1);
    if ( contain_chars(str_layer, "<>")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    return new MessageDataStream ( layer->get_description_tms(this), "text/xml", 200 );
}

DataStream* TmsService::get_metadata ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->path_params.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->path_params.at(1);
    if ( contain_chars(str_layer, "<>")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    return new MessageDataStream ( layer->get_description_tilejson(this), "application/json", 200 );
}

DataStream* TmsService::get_gdal ( Request* req, Rok4Server* serv ) {

    // La version
    if ( req->path_params.at(0) != "1.0.0" )
        throw TmsException::get_error_message("Invalid version (only 1.0.0 available)", 400);

    // La couche
    std::string str_layer = req->path_params.at(1);
    if ( contain_chars(str_layer, "<>")) {
        BOOST_LOG_TRIVIAL(warning) <<  "Forbidden char detected in TMS layer: " << str_layer ;
        throw TmsException::get_error_message("Layer unknown", 400);
    }

    Layer* layer = serv->get_server_configuration()->get_layer(str_layer);
    if ( layer == NULL || ! layer->is_tms_enabled() ) {
        throw TmsException::get_error_message("Layer " +str_layer+" unknown", 400);
    }

    if (! Rok4Format::is_raster(layer->get_pyramid()->get_format())) {
        throw TmsException::get_error_message("Layer " +str_layer+" is vector data: cannot describe it with this format", 400);
    }

    if (! layer->get_pyramid()->get_tms()->is_qtree()) {
        throw TmsException::get_error_message("Layer " +str_layer+" data is not a quad tree: cannot describe it with this format", 400);
    }

    return new MessageDataStream ( layer->get_description_gdal(this), "text/xml", 200 );
}
