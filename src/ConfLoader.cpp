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
 * \file ConfLoader.cpp
 * \~french
 * \brief Implémenation des fonctions de chargement de la configuration
 * \brief pendant l'initialisation du serveur
 * \~english
 * \brief Implements configuration loader functions
 * \brief during server initialization
 */


#include <cstdio>
#include <map>
#include <fcntl.h>
#include <cstddef>
#include <malloc.h>
#include <stdlib.h>
#include <libgen.h>

#include "ConfLoader.h"
#include "Request.h"

#include "utils/Utils.h"


/**********************************************************************************************************/
/***************************************** SERVER & SERVICES **********************************************/
/**********************************************************************************************************/

ServerConf* ConfLoader::buildServerConf ( std::string serverConfigFile ) {

    return new ServerConf( serverConfigFile );
}

ServicesConf* ConfLoader::buildServicesConf ( std::string servicesConfigFile ) {

    return new ServicesConf ( servicesConfigFile );
}

/**********************************************************************************************************/
/********************************************** STYLES ****************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildStylesList ( ServerConf* serverConf, ServicesConf* servicesConf ) {
    BOOST_LOG_TRIVIAL(info) <<  "STYLES LOADING" ;

    // lister les fichier du repertoire styleDir
    std::string styleDir = serverConf->getStylesDir();
    std::vector<std::string> styleFiles = Configuration::listFileFromDir(styleDir, ".json");

    // generer les styles decrits par les fichiers.
    for ( unsigned int i=0; i<styleFiles.size(); i++ ) {
        Style * style = buildStyle ( styleFiles[i], servicesConf );
        if ( style ) {
            serverConf->addStyle ( style );
        } else {
            BOOST_LOG_TRIVIAL(warning) <<  "Cannot load style " << styleFiles[i] ;
        }
    }

    if ( serverConf->getNbStyles() ==0 ) {
        BOOST_LOG_TRIVIAL(fatal) << "No style loaded !" ;
        return false;
    }

    BOOST_LOG_TRIVIAL(info) << serverConf->getNbStyles() << " style(s) loaded" ;

    return true;
}

Style* ConfLoader::buildStyle ( std::string fileName, ServicesConf* servicesConf ) {
    Style* style = new Style(fileName, servicesConf->isInspire() );
    if ( ! style->isOk() ) {
        BOOST_LOG_TRIVIAL(error) << style->getErrorMessage();
        delete style;
        return NULL;
    }

    if ( containForbiddenChars(style->getIdentifier()) ) {
        BOOST_LOG_TRIVIAL(error) << "Style identifier contains forbidden chars" ;
        delete style;
        return NULL;
    }
    if ( ! style->isUsableForBroadcast() ) {
        BOOST_LOG_TRIVIAL(warning) << "Style " << fileName << " not usable for broadcast" ;
        delete style;
        return NULL;
    }

    return style;
}


/**********************************************************************************************************/
/*********************************************** TMS ******************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildTMSList ( ServerConf* serverConf ) {
    BOOST_LOG_TRIVIAL(info) << "TMS LOADING" ;

    // lister les fichier du repertoire tmsDir
    std::string tmsDir = serverConf->getTmsDir();
    std::vector<std::string> tmsFiles = Configuration::listFileFromDir(tmsDir, ".json");

    // generer les TMS decrits par les fichiers.
    for ( unsigned int i=0; i<tmsFiles.size(); i++ ) {
        TileMatrixSet * tms;
        tms = buildTileMatrixSet ( tmsFiles[i] );
        if ( tms ) {
            serverConf->addTMS ( tms );
        } else {
            BOOST_LOG_TRIVIAL(error) << "Cannot load TMS " << tmsFiles[i] ;
        }
    }

    if ( serverConf->getNbTMS() ==0 ) {
        BOOST_LOG_TRIVIAL(fatal) << "No TMS loaded !" ;
        return false;
    }

    BOOST_LOG_TRIVIAL(info) << serverConf->getNbTMS() << " TMS loaded" ;

    return true;
}

TileMatrixSet* ConfLoader::buildTileMatrixSet ( std::string fileName ) {

    TileMatrixSet* tms = new TileMatrixSet(fileName);
    if ( ! tms->isOk() ) {
        BOOST_LOG_TRIVIAL(error) << tms->getErrorMessage();
        delete tms;
        return NULL;
    }

    return tms;
}

/**********************************************************************************************************/
/********************************************* LAYERS *****************************************************/
/**********************************************************************************************************/

bool ConfLoader::buildLayersList ( ServerConf* serverConf, ServicesConf* servicesConf ) {

    BOOST_LOG_TRIVIAL(info) << "LAYERS LOADING" ;
    // lister les fichier du repertoire layerDir
    std::string layerDir = serverConf->getLayersDir();
    std::vector<std::string> layerFiles = Configuration::listFileFromDir(layerDir, ".json");

    if ( layerFiles.empty() ) {
        BOOST_LOG_TRIVIAL(info) << "No .json file in the directory " << layerDir ;
        BOOST_LOG_TRIVIAL(info) << "Server has nothing to serve..." ;
    }

    // generer les Layers decrits par les fichiers.
    for ( unsigned int i = 0; i < layerFiles.size(); i++ ) {
        Layer * layer;
        layer = buildLayer ( layerFiles[i], serverConf, servicesConf );
        if ( layer ) {
            serverConf->addLayer ( layer );
        } else {
            BOOST_LOG_TRIVIAL(error) << "Cannot load layer " << layerFiles[i] ;
        }
    }

    BOOST_LOG_TRIVIAL(info) << serverConf->getNbLayers() << " layer(s) loaded" ;
    return true;
}

Layer * ConfLoader::buildLayer ( std::string fileName, ServerConf* serverConf, ServicesConf* servicesConf ) {

    Layer* layer = new Layer(fileName, serverConf, servicesConf );
    if ( ! layer->isOk() ) {
        BOOST_LOG_TRIVIAL(error) << layer->getErrorMessage();
        delete layer;
        return NULL;
    }

    return layer;
}

/**********************************************************************************************************/
/******************************************** PYRAMIDS ****************************************************/
/**********************************************************************************************************/

Pyramid* ConfLoader::buildPyramid ( Context* context, std::string fileName, ServerConf* serverConf ) {
    
    Pyramid* pyramid = new Pyramid( context, fileName, serverConf->getTmsList() );
    if ( ! pyramid->isOk() ) {
        BOOST_LOG_TRIVIAL(error) << pyramid->getErrorMessage();
        delete pyramid;
        return NULL;
    }

    return pyramid;
}


Pyramid* ConfLoader::buildPyramid ( std::vector<Pyramid*> pyramids, std::vector<std::string> bottomLevels, std::vector<std::string> topLevels ) {
    
    Pyramid* p = new Pyramid(pyramids.at(0));

    BOOST_LOG_TRIVIAL(info) << "pyramide composée de " << pyramids.size() << " pyramide(s)";

    for (int i = 0; i < pyramids.size(); i++) {
        if (! p->addLevels(pyramids.at(i), bottomLevels.at(i), topLevels.at(i))) {
            BOOST_LOG_TRIVIAL(error) << "Cannot compose pyramid to broadcast with input pyramid " << i;
            delete p;
            return NULL;
        }
    }

    return p;
}


