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
 * \file CapabilitiesBuilder.cpp
 * \~french
 * \brief Implémentation des fonctions de générations des GetCapabilities
 * \~english
 * \brief Implement the GetCapabilities generation function
 */

#include "Rok4Server.h"
#include "utils/Utils.h"

void Rok4Server::buildOGCTILESCapabilities() {
    // TODO [OGC] build collections...
    BOOST_LOG_TRIVIAL(warning) <<  "Not yet implemented !";
}

DataStream* Rok4Server::OGCTILESGetCapabilities ( Request* request ) {
    // TODO [OGC] get capabilities...
    BOOST_LOG_TRIVIAL(warning) <<  "Not yet implemented !";
    return new SERDataStream ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, "Not yet implemented !", "ogctiles" ) );
}

DataSource* Rok4Server::getTileParamOGCTILES ( Request* request, Layer*& layer, TileMatrixSet*& tms, TileMatrix*& tm, int& tileCol, int& tileRow, std::string& format, Style*& style) {
    // TODO [OGC] get tiles...
    return new SERDataSource ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, "Not yet implemented !", "ogctiles" ) );
    // request->tmpl;
    // request->path;
    // request->getParam("f");
    // request->getParam("collections");

    // pour le template :
    // les 4 derniers groupement sont toujours : tms, tm, tileCol, tileRow
    // si recherche /collections/(.*) isOk -> layer (unique!)
    // sinon param collections
    // si recherche styles/(.*) isOk -> style
    // sinon style par defaut

    // regex group name ? 
    // ex. (?<tms>\d{1,}) puis match->get(tms)
}