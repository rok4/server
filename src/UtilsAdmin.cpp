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
 * \file UtilsAdmin.cpp
 * \~french
 * \brief Implémentation des fonctions de générations des GetCapabilities
 * \~english
 * \brief Implement the GetCapabilities generation function
 */

#include "Rok4Server.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cmath>
#include "utils/TileMatrixSet.h"
#include "utils/Pyramid.h"
#include "config.h"


DataStream* Rok4Server::AdminCreateLayer ( Request* request ) {

    std::string str_layer = request->pathParts.at(2);

    Layer* layer = serverConf->getLayer(str_layer);

    if ( layer != NULL )
        return new SERDataStream ( new ServiceException ( "",ADMIN_CONFLICT,"Layer " +str_layer+" already exists.","admin", "application/json" ) );

    layer = new Layer( str_layer, request->body, serverConf, servicesConf );
    if ( ! layer->isOk() ) {
        std::string msg = layer->getErrorMessage();
        delete layer;
        return new SERDataStream ( new ServiceException ( "",ADMIN_BAD_REQUEST, msg,"admin", "application/json" ) );
    }

    serverConf->addLayer ( layer );

    // On recalcule les GetCapabilities
    if ( servicesConf->supportWMS ) {
        buildWMSCapabilities();
    }
    if ( servicesConf->supportWMTS ) {
        buildWMTSCapabilities();
    }
    if ( servicesConf->supportTMS ) {
        buildTMSCapabilities();
    }
    if ( servicesConf->supportOGCTILES ) {
        buildOGCTILESCapabilities();
    }

    if (! layer->writeToFile(request->body, serverConf)) {
        serverConf->removeLayer ( layer->getId() );
        return new SERDataStream ( new ServiceException ( "",INTERNAL_SERVER_ERROR, "Cannot write file to persist data", "admin", "application/json" ) );
    }

    return new EmptyResponseDataStream ();
}



DataStream* Rok4Server::AdminDeleteLayer ( Request* request ) {

    std::string str_layer = request->pathParts.at(2);

    Layer* layer = serverConf->getLayer(str_layer);

    if ( layer == NULL )
        return new SERDataStream ( new ServiceException ( "",HTTP_NOT_FOUND,"Layer " +str_layer+" does not exists.","admin", "application/json" ) );

    layer->removeFile(serverConf);
    serverConf->removeLayer ( layer->getId() );

    // On recalcule les GetCapabilities
    if ( servicesConf->supportWMS ) {
        buildWMSCapabilities();
    }
    if ( servicesConf->supportWMTS ) {
        buildWMTSCapabilities();
    }
    if ( servicesConf->supportTMS ) {
        buildTMSCapabilities();
    }
    if ( servicesConf->supportOGCTILES ) {
        buildOGCTILESCapabilities();
    }

    return new EmptyResponseDataStream ();
}


DataStream* Rok4Server::AdminUpdateLayer ( Request* request ) {

    std::string str_layer = request->pathParts.at(2);

    Layer* layer = serverConf->getLayer(str_layer);

    if ( layer == NULL )
        return new SERDataStream ( new ServiceException ( "",HTTP_NOT_FOUND,"Layer " +str_layer+" does not exists.","admin", "application/json" ) );

    Layer* newLayer = new Layer( str_layer, request->body, serverConf, servicesConf );
    if ( ! newLayer->isOk() ) {
        std::string msg = newLayer->getErrorMessage();
        delete newLayer;
        return new SERDataStream ( new ServiceException ( "",ADMIN_BAD_REQUEST, msg,"admin", "application/json" ) );
    }

    serverConf->removeLayer ( layer->getId() );
    serverConf->addLayer ( newLayer );

    // On recalcule les GetCapabilities
    if ( servicesConf->supportWMS ) {
        buildWMSCapabilities();
    }
    if ( servicesConf->supportWMTS ) {
        buildWMTSCapabilities();
    }
    if ( servicesConf->supportTMS ) {
        buildTMSCapabilities();
    }
    if ( servicesConf->supportOGCTILES ) {
        buildOGCTILESCapabilities();
    }

    if (! newLayer->writeToFile(request->body, serverConf)) {
        serverConf->removeLayer ( newLayer->getId() );
        return new SERDataStream ( new ServiceException ( "",INTERNAL_SERVER_ERROR, "Cannot write file to persist data", "admin", "application/json" ) );
    }

    return new EmptyResponseDataStream ();
}