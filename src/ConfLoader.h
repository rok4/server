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
 * \file ConfLoader.h
 * \~french
 * \brief Définition des fonctions de chargement de la configuration
 * \~english
 * \brief Define configuration loader functions
 */

#ifndef CONFLOADER_H_
#define CONFLOADER_H_

#include <vector>
#include <string>

#include "config.h"

#include "ServerConf.h"
#include "ServicesConf.h"

#include "utils/TileMatrixSet.h"

#include "style/Style.h"
#include "Layer.h"

#include "WebService.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Collection de fonctions gérant le chargement des configurations du serveur
 * \brief Chargement des configurations
 * \~english
 * Function library to load server configuration file
 * \brief Load configuration
 */
class  ConfLoader {
public:
#ifdef UNITTEST
    friend class CppUnitConfLoaderStyle;
    friend class CppUnitConfLoaderTMS;
    friend class CppUnitConfLoaderPyramid;
    friend class CppUnitConfLoaderTechnicalParam;
#endif //UNITTEST
    /**
     * \~french
     * \brief Chargement des paramètres du serveur à partir d'un fichier
     * \param[in] serverConfigFile Nom du fichier d'origine, utilisé comme identifiant
     * \return un objet ServerConf, contenant toutes les informations (NULL sinon)
     * \~english
     * \brief Load server parameter from a file
     * \param[in] serverConfigFile original filename, used as identifier
     * \return a ServerConf object, containing all informations, NULL if failure
     */
    static ServerConf* buildServerConf (std::string serverConfigFile);
    /**
     * \~french
     * \brief Charges les différents Styles présent dans le répertoire styleDir
     * \param[in] styleDir chemin du répertoire contenant les fichiers de Style
     * \param[out] stylesList ensemble des Styles disponibles
     * \param[in] inspire définit si les règles de conformité INSPIRE doivent être utilisées
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] styleDir path to Style directory
     * \param[out] stylesList set of available Styles.
     * \param[in] inspire whether INSPIRE validity rules are enforced
     * \return false if something went wrong
     */
    static bool buildStylesList ( ServerConf* serverConf, ServicesConf* servicesConf);

    /**
     * \~french
     * \brief Création d'un Style à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \param[in] inspire définit si les règles de conformité INSPIRE doivent être utilisées
     * \return un pointeur vers le Style nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Style from a file
     * \param[in] fileName filename, used as identifier
     * \param[in] inspire whether INSPIRE validity rules are enforced
     * \return pointer to the newly created Style, NULL if something went wrong
     */
    static Style* buildStyle ( std::string fileName, ServicesConf* servicesConf);

    /**
     * \~french
     * \brief Charges les différents TileMatrixSet présent dans le répertoire tmsDir
     * \param[in] tmsDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[out] tmsList ensemble des TileMatrixSets disponibles
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] tmsDir path to TileMatrixSet directory
     * \param[out] tmsList set of available TileMatrixSets
     * \return false if something went wrong
     */
    static bool buildTMSList ( ServerConf* serverConf);

    /**
     * \~french
     * \brief Création d'un TileMatrixSet à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \return un pointeur vers le TileMatrixSet nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new TileMatrixSet from a file
     * \param[in] fileName filename, used as identifier
     * \return pointer to the newly created TileMatrixSet, NULL if something went wrong
     */
    static TileMatrixSet* buildTileMatrixSet ( std::string fileName );

    /**
     * \~french
     * \brief Charges les différents Layers présent dans le répertoire layerDir
     * \param[in] layerDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[in] tmsList ensemble des TileMatrixSets disponibles
     * \param[in] stylesList ensemble des Styles disponibles
     * \param[out] layers ensemble des Layers disponibles
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales des services
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] layerDir path to TileMatrixSet directory
     * \param[in] tmsList set of available TileMatrixSets
     * \param[in] stylesList set of available Styles
     * \param[out] layers set of available Layers
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global services configuration pointer
     * \return false if something went wrong
     */
    static bool buildLayersList (ServerConf* serverConf, ServicesConf* servicesConf );

    /**
     * \~french
     * \brief Création d'un Layer à partir d'un fichier
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSets connus
     * \param[in] stylesList liste des Styles connus
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales du services
     * \return un pointeur vers le Layer nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Layer from a file
     * \param[in] fileName original filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \param[in] stylesList known Styles
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global service configuration pointer
     * \return pointer to the newly created Layer, NULL if something went wrong
     */
    static Layer * buildLayer (std::string fileName, ServerConf* serverConf, ServicesConf* servicesConf );


    /**
     * \~french
     * \brief Chargement des paramètres des services à partir d'un fichier
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \return un pointeur vers le ServicesConf nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Load service parameters from a file
     * \param[in] fileName original filename, used as identifier
     * \return pointer to the newly created ServicesConf, NULL if something went wrong
     */
    static ServicesConf* buildServicesConf ( std::string servicesConfigFile );

    /**
     * \~french
     * \brief Création d'une Pyramide à partir d'un descripterur de pyramide
     * \param[in] context Contexte de stockage du descripterur de pyramide
     * \param[in] fileName Nom du descripterur de pyramide
     * \param[in] serverConf configuration serveur
     * \return un pointeur vers la Pyramid nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Create a new Pyramid from a pyramid's descriptor
     * \param[in] context Storage context of pyramid's descriptor
     * \param[in] fileName pyramid's descriptor name
     * \param[in] serverConf server configuration
     * \return pointer to the newly created Pyramid, NULL if something went wrong
     */
    static Pyramid* buildPyramid (Context* context, std::string fileName, ServerConf* serverConf);

    /**
     * \~french
     * \brief Création d'une Pyramide à partir de plusieurs pyramides
     * \details Les pyramides en entrée doivent avoir les même caractéristiques, et les niveaux limites ne doivent pas se recouvrir
     * \param[in] pyramids Pyramides source
     * \param[in] bottomLevels niveaux du bas
     * \param[in] topLevels niveaux du haut
     * \return un pointeur vers la Pyramid nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Create a new Pyramid from several others
     * \details Input pyramids have to own same attributes and limit levels have not to overlapse
     * \param[in] pyramids source pyramids
     * \param[in] bottomLevels bottom levels
     * \param[in] topLevels top levels
     * \return pointer to the newly created Pyramid, NULL if something went wrong
     */
    static Pyramid* buildPyramid ( std::vector<Pyramid*> pyramids, std::vector<std::string> bottomLevels, std::vector<std::string> topLevels );


};

#endif /* CONFLOADER_H_ */
