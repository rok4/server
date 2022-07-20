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

class ServicesConf;

#ifndef SERVICESXML_H
#define SERVICESXML_H

#include <vector>
#include <string>

#include "Rok4Server.h"
#include "utils/Keyword.h"
#include "ConfLoader.h"
#include "MetadataURL.h"
#include "utils/CRS.h"
#include "utils/Configuration.h"

#include "config.h"

class ServicesConf : public Configuration
{
    friend class Rok4Server;
    
    public:
        ServicesConf(std::string path);
        ~ServicesConf();

        // WMS
        unsigned int getMaxTileX() const ;
        unsigned int getMaxTileY() const ;

        bool isInFormatList(std::string f) ;
        bool isInInfoFormatList(std::string f) ;
        bool isInGlobalCRSList(CRS* c) ;
        bool isInGlobalCRSList(std::string c) ;
        bool isFullStyleCapable() ;

        bool isInspire() ;
 
        bool getDoWeUseListOfEqualsCRS();
        bool getReprojectionCapability() ;

        bool are_the_two_CRS_equal( std::string crs1, std::string crs2 );
    
        /**
         * \~french
         * \brief Vérifie que le CRS ou un équivalent se trouve dans la liste des CRS autorisés
         * \~english
         * \brief Check if the CRS or an equivalent is in the allowed CRS list
         */
        bool isCRSAllowed(std::string crs);
        
        /**
         * \~french
         * \brief Retourne la liste des CRS équivalents et valable dans PROJ
         * \~english
         * \brief Return the list of the equivalents CRS who are PROJ compatible
         */
        std::vector<CRS*> getEqualsCRS(std::string crs);

    protected:

        // ----------------------- Global 

        std::string title;
        std::string abstract;
        std::vector<Keyword> keyWords;
        std::string serviceProvider;
        std::string providerSite;
        std::string fee;
        std::string accessConstraint;

        // ----------------------- Contact 
        std::string individualName;
        std::string individualPosition;
        std::string voice;
        std::string facsimile;
        std::string addressType;
        std::string deliveryPoint;
        std::string city;
        std::string administrativeArea;
        std::string postCode;
        std::string country;
        std::string electronicMailAddress;

        // ----------------------- Commun à plusieurs services 

        bool postMode;
        std::vector<std::string> infoFormatList;
        bool fullStyling;
        bool inspire;
        /**
         * \~french \brief Définit si le serveur doit permettre les reprojections (en WMS et WMTS)
         * \~english \brief Define whether WMS and WMTS reprojections should be honored
         */
        bool reprojectionCapability;
        bool doweuselistofequalsCRS;
        bool dowerestrictCRSList;
        std::vector<std::vector<CRS*> > listofequalsCRS;
        std::vector<CRS*> restrictedCRSList;

        // ----------------------- WMTS 

        /**
         * \~french \brief Définit si le serveur doit honorer les requêtes WMTS
         * \~english \brief Define whether WMTS request should be honored
         */
        bool supportWMTS;
        std::string wmtsPublicUrl;
        MetadataURL* mtdWMTS;

        // ----------------------- TMS 

        /**
         * \~french \brief Définit si le serveur doit honorer les requêtes TMS
         * \~english \brief Define whether TMS request should be honored
         */
        bool supportTMS;
        std::string tmsPublicUrl;
        MetadataURL* mtdTMS;

        // ----------------------- OGC Tiles

        /**
         * \~french \brief Définit si le serveur doit honorer les requêtes OGC
         * \~english \brief Define whether OGC request should be honored
         */
        bool supportOGCTILES;
        std::string ogctilesPublicUrl;

        // ----------------------- WMS 
        /**
         * \~french \brief Définit si le serveur doit honorer les requêtes WMS
         * \~english \brief Define whether WMS request should be honored
         */
        bool supportWMS;
        std::string wmsPublicUrl;
        MetadataURL* mtdWMS;
        std::string name;
        unsigned int maxWidth;
        unsigned int maxHeight;
        unsigned int maxTileX;
        unsigned int maxTileY;
        unsigned int layerLimit;
        std::vector<std::string> formatList;
        std::vector<CRS*> globalCRSList;


    private:

        /**
         * \~french
         * \brief Chargement de la liste des CRS équivalents
         * \~english
         * \brief Load equals CRS
         */
        bool loadEqualsCRSList(std::string file);

        /**
         * \~french
         * \brief Chargement de la liste restreinte des CRS
         * \~english
         * \brief Load restricted CRS list
         */
        bool loadRestrictedCRSList(std::string file);

        bool parse(json11::Json& doc);
};

#endif // SERVICESXML_H

