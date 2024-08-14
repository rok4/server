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
 * \file configurations/Services.h
 ** \~french
 * \brief Définition de la classe ServicesConfiguration
 ** \~english
 * \brief Define classe ServicesConfiguration
 */

#ifndef SERVICESCONF_H
#define SERVICESCONF_H

#include <vector>
#include <string>

#include <rok4/utils/CRS.h>
#include <rok4/utils/Configuration.h>
#include <rok4/utils/Keyword.h>

#include "Rok4Server.h"
#include "configurations/Metadata.h"
#include "configurations/Contact.h"
#include "services/common/Service.h"
#include "services/health/Service.h"
#include "services/tms/Service.h"
#include "services/wmts/Service.h"
#include "services/wms/Service.h"
#include "services/tiles/Service.h"
#include "services/admin/Service.h"

#include "config.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Gestion de la configuration des services assurés par le serveur
 */
class ServicesConfiguration : public Configuration
{
    friend class Rok4Server;
    friend class WmtsService;
    
    public:
        ServicesConfiguration(std::string path);
        ~ServicesConfiguration();

        CommonService* get_common_service() {return common_service;};
        HealthService* get_health_service() {return health_service;};
        TmsService* get_tms_service() {return tms_service;};
        WmtsService* get_wmts_service() {return wmts_service;};
        WmsService* get_wms_service() {return wms_service;};
        AdminService* get_admin_service() {return admin_service;};
        TilesService* get_tiles_service() {return tiles_service;};
        
        /**
         * \~french
         * \brief Retourne la liste des CRS équivalents et valable dans PROJ
         * \~english
         * \brief Return the list of the equivalents CRS who are PROJ compatible
         */
        std::vector<CRS*> get_equals_crs(std::string crs);
        bool handle_crs_equivalences() {
            return ! crs_equivalences.empty();
        };
        bool are_crs_equals( std::string crs1, std::string crs2 );

    protected:

        // ----------------------- Global 

        std::string service_provider;
        std::string provider_site;
        std::string fee;
        std::string access_constraint;

        Contact* contact;

    private:

        /**
         * \~french
         * \brief Chargement de la liste des CRS équivalents
         * \~english
         * \brief Load equals CRS
         */
        bool load_crs_equivalences(std::string file);

        bool parse(json11::Json& doc);

        CommonService* common_service;
        HealthService* health_service;
        TmsService* tms_service;
        WmtsService* wmts_service;
        WmsService* wms_service;
        AdminService* admin_service;
        TilesService* tiles_service;

        std::map<std::string, std::vector<CRS*> > crs_equivalences;
};

#endif // SERVICESCONF_H

