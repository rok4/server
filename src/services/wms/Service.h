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
 * \file services/wms/Service.h
 ** \~french
 * \brief Définition de la classe WmsService
 ** \~english
 * \brief Define classe WmsService
 */

class WmsService;

#ifndef WMSSERVICE_H_
#define WMSSERVICE_H_

#include "services/Service.h"
#include "configurations/Metadata.h"
#include "configurations/Services.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Gestion du service WMS du serveur
 */
class WmsService : public Service {  

private:
    DataStream* get_capabilities ( Request* req, Rok4Server* serv );
    DataStream* get_feature_info ( Request* req, Rok4Server* serv );
    DataStream* get_map ( Request* req, Rok4Server* serv );

    std::string name;
    Metadata* metadata;
    bool reprojection;
    std::vector<std::string> info_formats;
    std::vector<std::string> formats;

    std::string root_layer_title;
    std::string root_layer_abstract;

    int max_layers_count;
    int max_width;
    int max_height;
    int max_tile_x;
    int max_tile_y;

    std::vector<CRS*> crss;

public:
    DataStream* process_request(Request* req, Rok4Server* serv);

    /**
     * \~french
     * \brief Constructeur du service 'wms'
     * \~english
     * \brief Service constructor
     */
    WmsService (json11::Json& doc, ServicesConfiguration* svc);

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~WmsService() {
        if (metadata) delete metadata;
    };

    /**
     * \~french
     * \brief Teste la présence du CRS dans la liste
     * \return Présent ou non
     * \~english
     * \brief Test if CRS is in the CRS list
     * \return Present or not
     */
    bool is_available_crs(CRS* c) ;

    /**
     * \~french
     * \brief Teste la présence du CRS dans la liste
     * \return Présent ou non
     * \~english
     * \brief Test if CRS is in the CRS list
     * \return Present or not
     */
    bool is_available_crs(std::string c) ;

    /**
     * \~french
     * \brief Teste la validité du format
     * \~english
     * \brief Test if format is valid
     */
    bool is_available_format(std::string f) ;

    /**
     * \~french
     * \brief Teste la validité du info format
     * \~english
     * \brief Test if info format is valid
     */
    bool is_available_infoformat(std::string f) ;

    /**
     * \~french
     * \brief La reprojection est-elle activée
     * \~english
     * \brief Is reprojection enabled
     */
    bool reprojection_enabled() {
        return reprojection;
    };

};

#endif /* WMSSERVICE_H_ */
