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
#include <rok4/enums/Interpolation.h>
#include <rok4/utils/Keyword.h>
#include <rok4/utils/BoundingBox.h>
#include <rok4/utils/Configuration.h>

#include "configurations/Server.h"
#include "configurations/Metadata.h"
#include "configurations/Attribution.h"

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
     * \~french \brief Identifiant de la couche
     * \~english \brief Layer identifier
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
     * \~french \brief Autorisation du WMS pour ce layer
     * \~english \brief Authorized WMS for this layer
     */
    bool wms;
    /**
     * \~french \brief Autorisation du WMTS pour ce layer
     * \~english \brief Authorized WMTS for this layer
     */
    bool wmts;
    /**
     * \~french \brief Autorisation du TMS pour ce layer
     * \~english \brief Authorized TMS for this layer
     */
    bool tms;
    /**
     * \~french \brief Autorisation de API Tiles pour ce layer
     * \~english \brief Authorized API Tiles for this layer
     */
    bool tiles;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keywords;
    /**
     * \~french \brief Pyramide de tuiles
     * \~english \brief Tile pyramid
     */
    Pyramid* pyramid;

    /**
     * \~french \brief Emprise des données en coordonnées géographique (WGS84)
     * \~english \brief Data bounding box in geographic coordinates (WGS84)
     */
    BoundingBox<double> geographic_bbox;
    /**
     * \~french \brief Emprise des données dans le système de coordonnées natif
     * \~english \brief Data bounding box in native coordinates system
     */
    BoundingBox<double> native_bbox;
    /**
     * \~french \brief Liste des métadonnées associées
     * \~english \brief Linked metadata list
     */
    std::vector<Metadata> metadata;
    /**
     * \~french \brief Attribution
     * \~english \brief Attribution
     */
    Attribution* attribution;
    
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
    std::vector<CRS*> wms_crs;
    /**
     * \~french \brief Liste des TMS d'interrogation autorisés en WMTS
     * \~english \brief TMS list for WMTS requests
     */
    std::vector<WmtsTmsInfos> wmts_tilematrixsets;

    /**
     * \~french \brief Interpolation utilisée pour reprojeter ou recadrer les tuiles
     * \~english \brief Interpolation used for resizing and reprojecting tiles
     */
    Interpolation::KernelType resampling;
    /**
     * \~french \brief GetFeatureInfo autorisé
     * \~english \brief Authorized GetFeatureInfo
     */
    bool gfi_enabled;
    /**
     * \~french \brief Source du GetFeatureInfo
     * \~english \brief Source of GetFeatureInfo
     */
    std::string gfi_type;
    /**
     * \~french \brief URL du service WMS à utiliser pour le GetFeatureInfo
     * \~english \brief WMS-V service URL to use for getFeatureInfo
     */
    std::string gfi_url;
    /**
     * \~french \brief Type de service (WMS ou WMTS)
     * \~english \brief Type of service (WMS or WMTS)
     */
    std::string gfi_service;
    /**
     * \~french \brief Version du service
     * \~english \brief Version of service
     */
    std::string gfi_version;
    /**
     * \~french \brief Paramètre query_layers à fournir au service
     * \~english \brief Parameter query_layers for the service
     */
    std::string gfi_query_layers;
    /**
     * \~french \brief Paramètre layers à fournir au service
     * \~english \brief Parameter layers for the service
     */
    std::string gfi_layers;
    /**
     * \~french \brief Paramètres de requête additionnels à fournir au service
     * \~english \brief Additionnal query parameters for the service
     */
    std::string gfi_extra_params;
    /**
     * \~french \brief Modification des EPSG autorisé (pour Geoserver)
     * \~english \brief Modification of EPSG is authorized (for Geoserver)
     */
    bool gfi_force_epsg;

    void calculate_bboxes();
    void calculate_native_tilematrix_limits();
    void calculate_tilematrix_limits();

    bool parse(json11::Json& doc);

public:
    /**
    * \~french
    * Crée un Layer à partir d'un fichier JSON
    * \brief Constructeur
    * \param[in] path Chemin vers le descripteur de couche
    * \~english
    * Create a Layer from a JSON file
    * \brief Constructor
    * \param[in] path Path to layer descriptor
    */
    Layer(std::string path );
    /**
    * \~french
    * Crée un Layer à partir d'un contenu JSON
    * \brief Constructeur
    * \param[in] layer_name Identifiant de la couche
    * \param[in] content Contenu JSON
    * \~english
    * Create a Layer from JSON content
    * \brief Constructor
    * \param[in] layer_name Layer identifier
    * \param[in] content JSON content
    */
    Layer(std::string layer_name, std::string content );

    /**
     * \~french
     * \brief Retourne l'indentifiant de la couche
     * \return identifiant
     * \~english
     * \brief Return the layer's identifier
     * \return identifier
     */
    std::string get_id();
    
    /**
    * \~french
    * \brief Retourne le résumé
    * \return résumé
    * \~english
    * \brief Return the abstract
    * \return abstract
    */
    std::string get_abstract() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service WMS
     * \return WMSAuthorized
     * \~english
     * \brief Return the right to use WMS
     * \return WMSAuthorized
     */
    bool is_wms_enabled() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service TMS
     * \return TMSAuthorized
     * \~english
     * \brief Return the right to use TMS
     * \return TMSAuthorized
     */
    bool is_tms_enabled() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser le service API Tiles
     * \return tiles
     * \~english
     * \brief Return the right to use API Tiles
     * \return tiles
     */
    bool is_tiles_enabled() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service WMTS
     * \return WMTSAuthorized
     * \~english
     * \brief Return the right to use WMTS
     * \return WMTSAuthorized
     */
    bool is_wmts_enabled() ;
    /**
     * \~french
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    std::vector<Keyword>* get_keywords() ;
    /**
     * \~french
     * \brief Retourne l'attribution
     * \return attribution
     * \~english
     * \brief Return the attribution
     * \return attribution
     */
    Attribution* get_attribution() ;

    /**
     * \~french
     * \brief Retourne la pyramide de données associée
     * \return pyramide
     * \~english
     * \brief Return the associated data pyramid
     * \return pyramid
     */
    Pyramid* get_pyramid() ;
    
    /**
     * \~french
     * \brief Retourne le style par défaut associé à la couche
     * \return style
     * \~english
     * \brief Return the layer's default style
     * \return style
     */
    Style* get_default_style() ;
    /**
     * \~french
     * \brief Retourne la liste des styles associés à la couche
     * \return liste de styles
     * \~english
     * \brief Return the associated styles list
     * \return styles list
     */
    std::vector<Style*> get_styles() ;
    /**
     * \~french
     * \brief Retourne la liste des TMS disponibles
     * \return liste d'informations sur les TMS
     * \~english
     * \brief Return the available TMS list
     * \return TMS infos list
     */
    std::vector<WmtsTmsInfos> get_wmts_tilematrixsets() ;

    /**
     * \~french
     * \brief Retourne le style associé à la couche (identifiant public)
     * \return le style si associé, NULL sinon
     * \~english
     * \brief Return the associated style (public identifier)
     * \return the style if present, NULL otherwise
     */
    Style* get_style_by_identifier(std::string identifier) ;

    /**
     * \~french
     * \brief Retourne le titre
     * \return titre
     * \~english
     * \brief Return the title
     * \return title
     */
    std::string get_title() ;

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
    TileMatrixSet* get_tilematrixset(std::string id) ;

    /**
     * \~french
     * \brief Retourne les limites pour le niveau du TMS fourni
     * \return NULL si le niveau n'est pas valide
     * \~english
     * \brief Return limits for provided Tile Matrix
     * \return NULL if invalid Tile Matrix
     */
    TileMatrixLimits* get_tilematrix_limits(TileMatrixSet* tms, TileMatrix* tm) ;

    /**
     * \~french
     * \brief Retourne l'emprise des données en coordonnées géographique (WGS84)
     * \return emprise
     * \~english
     * \brief Return the data bounding box in geographic coordinates (WGS84)
     * \return bounding box
     */
    BoundingBox<double> get_geographical_bbox() ;
    /**
     * \~french
     * \brief Retourne l'emprise des données dans le système de coordonnées natif
     * \return emprise
     * \~english
     * \brief Return the data bounding box in the native coordinates system
     * \return bounding box
     */
    BoundingBox<double> get_native_bbox() ;


    /**
     * \~french
     * \brief Récupère l'interpolation
     * \return interpolation
     * \~english
     * \brief Get resampling
     * \return resampling
     */
    Interpolation::KernelType get_resampling() ;

    /**
     * \~french
     * \brief Retourne la liste des métadonnées associées
     * \return liste de métadonnées
     * \~english
     * \brief Return the associated metadata list
     * \return metadata list
     */
    std::vector<Metadata> get_metadata() ;
    /**
     * \~french
     * \brief GFI est-il autorisé
     * \return true si oui
     * \~english
     * \brief Is GFI authorized
     * \return true if it is
     */
    bool is_gfi_enabled() ;
    /**
     * \~french
     * \brief Retourne la source du GFI
     * \return source du GFI
     * \~english
     * \brief Return the source used by GFI
     * \return source used by GFI
     */
    std::string get_gfi_type() ;
    /**
     * \~french
     * \brief Retourne l'URL du service de GFI
     * \return URL de service
     * \~english
     * \brief Return the URL of the service used for GFI
     * \return URL of the service
     */
    std::string get_gfi_url() ;
    /**
     * \~french
     * \brief Retourne le paramètre layers de la requête de GFI
     * \return paramètre layers
     * \~english
     * \brief Return the parameter layers of GFI request
     * \return parameter layers
     */
    std::string get_gfi_layers() ;
    /**
     * \~french
     * \brief Retourne le paramètre query_layers de la requête de GFI
     * \return paramètre query_layers
     * \~english
     * \brief Return the parameter query_layers of GFI request
     * \return parameter query_layers
     */
    std::string get_gfi_query_layers() ;
    /**
     * \~french
     * \brief Retourne le type du service de GFI
     * \return type du service de GFI
     * \~english
     * \brief Return type of service used for GFI
     * \return type of service used for GFI
     */
    std::string get_gfi_service() ;
    /**
     * \~french
     * \brief Retourne la version du service de GFI
     * \return version du service de GFI
     * \~english
     * \brief Return version of service used for GFI
     * \return version of service used for GFI
     */
    std::string get_gfi_version() ;
    /**
     * \~french
     * \brief Retourne les paramètres de requête additionnels de la requête de GFI
     * \return paramètres additionnels
     * \~english
     * \brief Return the extra query parameters of GFI request
     * \return extra parameters
     */
    std::string get_gfi_extra_params() ;
    /**
     * \~french
     * \brief
     * \return
     * \~english
     * \brief
     * \return
     */
    bool get_gfi_force_epsg() ;
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~Layer();

};

#endif /* LAYER_H_ */
