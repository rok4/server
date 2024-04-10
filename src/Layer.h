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
 * \file Layer.h
 * \~french
 * \brief Définition de la classe Layer modélisant les couches de données.
 * \~english
 * \brief Define the Layer Class handling data layer.
 */

class Layer;

#ifndef LAYER_H_
#define LAYER_H_

#include <vector>
#include <string>

#include <rok4/utils/Pyramid.h>
#include <rok4/utils/CRS.h>
#include <rok4/style/Style.h>
#include "ServerConf.h"
#include "ServicesConf.h"
#include "MetadataURL.h"
#include "AttributionURL.h"
#include <rok4/enums/Interpolation.h>
#include <rok4/utils/Keyword.h>
#include <rok4/utils/BoundingBox.h>
#include <rok4/utils/Configuration.h>

struct WmtsTmsInfos {
    TileMatrixSet* tms;
    std::string wmts_id;
    std::string top_level;
    std::string bottom_level;
    std::vector<TileMatrixLimits> limits;
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Layer représente une couche du service WMS, WMTS ou TMS.
 * Une couche est défini par :
 * \li Des pyramides source
 * \li Les styles diponibles
 * \li Les systèmes de coordonnées authorisées
 * \li Une emprise
 *
 * Exemple de fichier de layer complet :
 * \brief Gestion des couches
 * \~english
 * A Layer represent a service layer either WMS, WMTS or TMS
 * The layer contain reference to :
 * \li source pyramids
 * \li availlable styles
 * \li availlable coordinates systems
 * \li a bounding box
 *
 */
class Layer : public Configuration {
private:
    /**
     * \~french \brief Identifiant WMS/WMTS de la couche
     * \~english \brief WMS/WMTS layer identifier
     */
    std::string id;
    /**
     * \~french \brief Titre
     * \~english \brief Title
     */
    std::string title;
    /**
     * \~french \brief Résumé
     * \~english \brief abstract
     */
    std::string abstract;
    /**
     * \~french \brief Autorisé le WMS pour ce layer
     * \~english \brief Authorized WMS for this layer
     */
    bool WMSAuthorized;
    /**
     * \~french \brief Autorisé le WMTS pour ce layer
     * \~english \brief Authorized WMTS for this layer
     */
    bool WMTSAuthorized;
    /**
     * \~french \brief Autorisé le TMS pour ce layer
     * \~english \brief Authorized TMS for this layer
     */
    bool TMSAuthorized;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keyWords;
    /**
     * \~french \brief Pyramide de tuiles
     * \~english \brief Tile pyramid
     */
    Pyramid* dataPyramid;

    /**
     * \~french \brief Nom de l'entité propriétaire de la couche
     * \~english \brief Oo
     */
    std::string authority;
    /**
     * \~french \brief Emprise des données en coordonnées géographique (WGS84)
     * \~english \brief Data bounding box in geographic coordinates (WGS84)
     */
    BoundingBox<double> geographicBoundingBox;
    /**
     * \~french \brief Emprise des données dans le système de coordonnées natif
     * \~english \brief Data bounding box in native coordinates system
     */
    BoundingBox<double> boundingBox;
    /**
     * \~french \brief Liste des métadonnées associées
     * \~english \brief Linked metadata list
     */
    std::vector<MetadataURL> metadataURLs;
    /**
     * \~french \brief Attribution
     * \~english \brief Attribution
     */
    AttributionURL* attribution;
    
    /******************* PYRAMIDE RASTER *********************/

    /**
     * \~french \brief Liste des styles associés
     * \~english \brief Linked styles list
     */
    std::vector<Style*> styles;
    
    /**
     * \~french \brief Liste des systèmes de coordonnées authorisées pour le WMS
     * \~english \brief Authorised coordinates systems list for WMS
     */
    std::vector<CRS*> WMSCRSList;
    /**
     * \~french \brief Liste des TMS d'interrogation autorisés en WMTS
     * \~english \brief TMS list for WMTS requests
     */
    std::vector<WmtsTmsInfos> WMTSTMSList;

    /**
     * \~french \brief Interpolation utilisée pour reprojeter ou recadrer les tuiles
     * \~english \brief Interpolation used for resizing and reprojecting tiles
     */
    Interpolation::KernelType resampling;
    /**
     * \~french \brief GetFeatureInfo autorisé
     * \~english \brief Authorized GetFeatureInfo
     */
    bool getFeatureInfoAvailability;
    /**
     * \~french \brief Source du GetFeatureInfo
     * \~english \brief Source of GetFeatureInfo
     */
    std::string getFeatureInfoType;
    /**
     * \~french \brief URL du service WMS à utiliser pour le GetFeatureInfo
     * \~english \brief WMS-V service URL to use for getFeatureInfo
     */
    std::string getFeatureInfoBaseURL;
    /**
     * \~french \brief Type de service (WMS ou WMTS)
     * \~english \brief Type of service (WMS or WMTS)
     */
    std::string GFIService;
    /**
     * \~french \brief Version du service
     * \~english \brief Version of service
     */
    std::string GFIVersion;
    /**
     * \~french \brief Paramètre query_layers à fournir au service
     * \~english \brief Parameter query_layers for the service
     */
    std::string GFIQueryLayers;
    /**
     * \~french \brief Paramètre layers à fournir au service
     * \~english \brief Parameter layers for the service
     */
    std::string GFILayers;
    /**
     * \~french \brief Paramètres de requête additionnels à fournir au service
     * \~english \brief Additionnal query parameters for the service
     */
    std::string GFIExtraParams;
    /**
     * \~french \brief Modification des EPSG autorisé (pour Geoserver)
     * \~english \brief Modification of EPSG is authorized (for Geoserver)
     */
    bool GFIForceEPSG;

    void calculateBoundingBoxes();
    void calculateNativeTileMatrixLimits();
    void calculateTileMatrixLimits();

    bool parse(json11::Json& doc, ServicesConf* servicesConf);

public:
    /**
    * \~french
    * Crée un Layer à partir d'un fichier JSON
    * \brief Constructeur
    * \param[in] path Chemin vers le descripteur de couche
    * \param[in] servicesConf Configuration des services
    * \~english
    * Create a Layer from a JSON file
    * \brief Constructor
    * \param[in] path Path to layer descriptor
    * \param[in] servicesConf Services configuration
    */
    Layer(std::string path, ServicesConf* servicesConf );
    /**
    * \~french
    * Crée un Layer à partir d'un contenu JSON
    * \brief Constructeur
    * \param[in] layerName Identifiant de la couche
    * \param[in] content Contenu JSON
    * \param[in] servicesConf Configuration des services
    * \~english
    * Create a Layer from JSON content
    * \brief Constructor
    * \param[in] layerName Layer identifier
    * \param[in] content JSON content
    * \param[in] servicesConf Services configuration
    */
    Layer(std::string layerName, std::string content, ServicesConf* servicesConf );

    /**
     * \~french
     * \brief Retourne l'indentifiant de la couche
     * \return identifiant
     * \~english
     * \brief Return the layer's identifier
     * \return identifier
     */
    std::string getId();
    /**
     * \~french
     * L'image résultante est découpé sur l'emprise de définition du système de coordonnées demandé.
     * Code d'erreur possible :
     *  - \b 0 pas d'erreur
     *  - \b 1 erreur de reprojection de l'emprise demandé dans le système de coordonnées de la pyramide
     *  - \b 2 l'emprise demandée nécessite plus de tuiles que le nombre authorisé.
     * \brief Retourne une l'image correspondant à l'emprise demandée
     * \param [in] servicesConf paramètre de configuration du service WMS
     * \param [in] bbox rectangle englobant demandé
     * \param [in] width largeur de l'image demandé
     * \param [in] height hauteur de l'image demandé
     * \param [in] dst_crs système de coordonnées du rectangle englobant
     * \param [in,out] error code de retour d'erreur
     * \return une image ou un poiteur nul
     * \~english
     * The resulting image is cropped on the coordinates system definition area.
     * \brief
     * \param [in] servicesConf WMS service configuration
     * \param [in] bbox requested bounding box
     * \param [in] width requested image widht
     * \param [in] height requested image height
     * \param [in] dst_crs bounding box coordinate system
     * \param [in,out] error error code
     * \return an image or a null pointer
     */
    Image* getbbox (ServicesConf* servicesConf, BoundingBox<double> bbox, int width, int height, CRS* dst_crs, int dpi, int& error );
    /**
    * \~french
    * \brief Retourne le résumé
    * \return résumé
    * \~english
    * \brief Return the abstract
    * \return abstract
    */
    std::string getAbstract() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service WMS
     * \return WMSAuthorized
     * \~english
     * \brief Return the right to use WMS
     * \return WMSAuthorized
     */
    bool getWMSAuthorized() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service TMS
     * \return TMSAuthorized
     * \~english
     * \brief Return the right to use TMS
     * \return TMSAuthorized
     */
    bool getTMSAuthorized() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service WMTS
     * \return WMTSAuthorized
     * \~english
     * \brief Return the right to use WMTS
     * \return WMTSAuthorized
     */
    bool getWMTSAuthorized() ;
    /**
     * \~french
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    std::vector<Keyword>* getKeyWords() ;
    /**
     * \~french
     * \brief Retourne l'attribution
     * \return attribution
     * \~english
     * \brief Return the attribution
     * \return attribution
     */
    AttributionURL* getAttribution() ;
    /**
     * \~french
     * \brief Retourne l'échelle maximum
     * \return échelle maximum
     * \~english
     * \brief Return the maximum scale
     * \return maximum scale
     */
    double getMaxRes() ;
    /**
     * \~french
     * \brief Retourne l'échelle minimum
     * \return échelle minimum
     * \~english
     * \brief Return the minimum scale
     * \return minimum scale
     */
    double getMinRes() ;
    /**
     * \~french
     * \brief Retourne la pyramide de données associée
     * \return pyramide
     * \~english
     * \brief Return the associated data pyramid
     * \return pyramid
     */
    Pyramid* getDataPyramid() ;
    /**
     * \~french
     * \brief Retourne l'ID WMTS du TMS natif de la pyramide de données associée
     * \details L'identifiant intègre les niveaux du haut et du bas d'utilisation dans son nom
     * \return Identifiant de TMS
     * \~english
     * \brief Return the WMTS ID of associated data pyramid native TMS
     * \details ID contains used top and bottom levels
     * \return TMS id
     */
    std::string getNativeWmtsTmsId() ;
    
    /**
     * \~french
     * \brief Retourne le style par défaut associé à la couche
     * \return style
     * \~english
     * \brief Return the layer's default style
     * \return style
     */
    Style* getDefaultStyle() ;
    /**
     * \~french
     * \brief Retourne la liste des styles associés à la couche
     * \return liste de styles
     * \~english
     * \brief Return the associated styles list
     * \return styles list
     */
    std::vector<Style*> getStyles() ;
    /**
     * \~french
     * \brief Retourne la liste des TMS disponibles
     * \return liste d'informations sur les TMS
     * \~english
     * \brief Return the available TMS list
     * \return TMS infos list
     */
    std::vector<WmtsTmsInfos> getWMTSTMSList() ;

    /**
     * \~french
     * \brief Retourne le style associé à la couche (identifiant interne)
     * \return le style si associé, NULL sinon
     * \~english
     * \brief Return the associated style (internal identifier)
     * \return the style if present, NULL otherwise
     */
    Style* getStyle(std::string id) ;

    /**
     * \~french
     * \brief Retourne le style associé à la couche (identifiant public)
     * \return le style si associé, NULL sinon
     * \~english
     * \brief Return the associated style (public identifier)
     * \return the style if present, NULL otherwise
     */
    Style* getStyleByIdentifier(std::string identifier) ;

    /**
     * \~french
     * \brief Retourne le titre
     * \return titre
     * \~english
     * \brief Return the title
     * \return title
     */
    std::string getTitle() ;

    /**
     * \~french
     * \brief Retourne la liste des systèmes de coordonnées authorisés
     * \return liste des CRS
     * \~english
     * \brief Return the authorised coordinates systems list
     * \return CRS list
     */
    std::vector<CRS*> getWMSCRSList() ;

    /**
     * \~french
     * \brief Teste la présence du CRS dans la liste
     * \return Présent ou non
     * \~english
     * \brief Test if CRS is in the CRS list
     * \return Present or not
     */
    bool isInWMSCRSList(CRS* c) ;

    /**
     * \~french
     * \brief Teste la présence du CRS dans la liste
     * \return Présent ou non
     * \~english
     * \brief Test if CRS is in the CRS list
     * \return Present or not
     */
    bool isInWMSCRSList(std::string c) ;

    /**
     * \~french
     * \brief Récupère le TMS disponible sur la couche par l'identifiant
     * \details L'identifiant peut contenir les niveaux du haut et du bas
     * \return NULL si ce TMS n'est pas disponible
     * \~english
     * \brief Get available TMS for the layer with identifiant
     * \return NULL if not available
     */
    TileMatrixSet* getTms(std::string id) ;

    /**
     * \~french
     * \brief Retourne les limites pour le niveau du TMS fourni
     * \return NULL si le niveau n'est pas valide
     * \~english
     * \brief Return limits for provided Tile Matrix
     * \return NULL if invalid Tile Matrix
     */
    TileMatrixLimits* getTmLimits(TileMatrixSet* tms, TileMatrix* tm) ;

    /**
     * \~french
     * \brief Retourne l'emprise des données en coordonnées géographique (WGS84)
     * \return emprise
     * \~english
     * \brief Return the data bounding box in geographic coordinates (WGS84)
     * \return bounding box
     */
    BoundingBox<double> getGeographicBoundingBox() ;
    /**
     * \~french
     * \brief Retourne l'emprise des données dans le système de coordonnées natif
     * \return emprise
     * \~english
     * \brief Return the data bounding box in the native coordinates system
     * \return bounding box
     */
    BoundingBox<double> getBoundingBox() ;
    /**
     * \~french
     * \brief Retourne la liste des métadonnées associées
     * \return liste de métadonnées
     * \~english
     * \brief Return the associated metadata list
     * \return metadata list
     */
    std::vector<MetadataURL> getMetadataURLs() ;
    /**
     * \~french
     * \brief GFI est-il autorisé
     * \return true si oui
     * \~english
     * \brief Is GFI authorized
     * \return true if it is
     */
    bool isGetFeatureInfoAvailable() ;
    /**
     * \~french
     * \brief Retourne la source du GFI
     * \return source du GFI
     * \~english
     * \brief Return the source used by GFI
     * \return source used by GFI
     */
    std::string getGFIType() ;
    /**
     * \~french
     * \brief Retourne l'URL du service de GFI
     * \return URL de service
     * \~english
     * \brief Return the URL of the service used for GFI
     * \return URL of the service
     */
    std::string getGFIBaseUrl() ;
    /**
     * \~french
     * \brief Retourne le paramètre layers de la requête de GFI
     * \return paramètre layers
     * \~english
     * \brief Return the parameter layers of GFI request
     * \return parameter layers
     */
    std::string getGFILayers() ;
    /**
     * \~french
     * \brief Retourne le paramètre query_layers de la requête de GFI
     * \return paramètre query_layers
     * \~english
     * \brief Return the parameter query_layers of GFI request
     * \return parameter query_layers
     */
    std::string getGFIQueryLayers() ;
    /**
     * \~french
     * \brief Retourne le type du service de GFI
     * \return type du service de GFI
     * \~english
     * \brief Return type of service used for GFI
     * \return type of service used for GFI
     */
    std::string getGFIService() ;
    /**
     * \~french
     * \brief Retourne la version du service de GFI
     * \return version du service de GFI
     * \~english
     * \brief Return version of service used for GFI
     * \return version of service used for GFI
     */
    std::string getGFIVersion() ;
    /**
     * \~french
     * \brief Retourne les paramètres de requête additionnels de la requête de GFI
     * \return paramètres additionnels
     * \~english
     * \brief Return the extra query parameters of GFI request
     * \return extra parameters
     */
    std::string getGFIExtraParams() ;
    /**
     * \~french
     * \brief
     * \return
     * \~english
     * \brief
     * \return
     */
    bool getGFIForceEPSG() ;
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~Layer();

};

#endif /* LAYER_H_ */
