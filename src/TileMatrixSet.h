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
 * \file TileMatrix.h
 * \~french
 * \brief Définition de la classe TileMatrixSet gérant une pyramide de matrices (Cf TileMatrix)
 * \~english
 * \brief Define the TileMatrixSet Class handling a pyramid of matrix (See TileMatrix)
 */

#ifndef TILEMATRIXSET_H_
#define TILEMATRIXSET_H_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>

#include "TileMatrix.h"
#include "utils/CRS.h"
#include "utils/Keyword.h"
#include "utils/Configuration.h"

typedef std::function<bool(std::pair<std::string, TileMatrix*>, std::pair<std::string, TileMatrix*>)> ComparatorTileMatrix;

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance TileMatrixSet représente une pyramide de TileMatrix définie dans un même système de coordonnées.
 *
 * Définition d'un TileMatrixSet en XML :
 * \brief Gestion d'une pyramid de matrices de tuiles
 * \~english
 * A TileMatrixSet represent a pyramid of TileMatrix in the same coordinate system.
 *
 * JSON definition of a TileMatrix :
 * \brief Handle pyramid of matrix of tiles
 * \details \~ \code{.json}
 *  {
 *      "crs": "EPSG:3857",
 *      "orderedAxes": [
 *          "X",
 *          "Y"
 *      ],
 *      "id": "PM",
 *      "tileMatrices": [
 *          {
 *              "id": "0",
 *              "tileWidth": 256,
 *              "scaleDenominator": 559082264.028718,
 *              "matrixWidth": 1,
 *              "cellSize": 156543.033928041,
 *              "matrixHeight": 1,
 *              "tileHeight": 256,
 *              "pointOfOrigin": [
 *                  -20037508.3427892,
 *                  20037508.3427892
 *              ]
 *          }
 *      ]
 *  }
 * \endcode
 */
class TileMatrixSet : public Configuration {
private:
    /**
     * \~french \brief Identifiant
     * \~english \brief Identifier
     */
    std::string id;
    /**
     * \~french \brief Titre
     * \~english \brief Title
     */
    std::string title;
    /**
     * \~french \brief Résumé
     * \~english \brief Abstract
     */
    std::string abstract;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keyWords;
    /**
     * \~french \brief Système de coordonnées associé
     * \~english \brief Linked coordinates system
     */
    CRS* crs;
    /**
     * \~french \brief Est ce que le TMS est un QTree ?
     * \~english \brief Is TMS a QTree ?
     */
    bool isQTree;
    /**
     * \~french \brief Liste des TileMatrix
     * \~english \brief List of TileMatrix
     */
    std::map<std::string, TileMatrix*> tmList;

    bool parse(json11::Json& doc);
public:

    /**
    * \~french
    * Crée un TileMatrixSet à partir d'un fichier XML
    * \brief Constructeur
    * \param[in] path Chemin vers le fichier TMS
    * \~english
    * Create a TileMatrixSet from a XML file
    * \brief Constructor
    * \param[in] path Path to TMS file
    */
    TileMatrixSet(std::string path);

    /**
     * \~french
     * La comparaison ignore les mots-clés et les TileMatrix
     * \brief Test d'egalite de 2 TileMatrixSet
     * \return true si tous les attributs sont identiques et les listes de taille identiques, false sinon
     * \~english
     * Rapid comparison of two TileMatrixSet, Keywords and TileMatrix are not verified
     * \brief Test whether 2 TileMatrixSet are equals
     * \return true if attributes are equal and lists have the same size
     */
    bool operator== ( const TileMatrixSet& other ) const;
    /**
     * \~french
     * La comparaison ignore les mots-clés et les TileMatrix
     * \brief Test d'inégalite de 2 TileMatrixSet
     * \return true si tous les attributs sont identiques et les listes de taille identiques, false sinon
     * \~english
     * Rapid comparison of two TileMatrixSet, Keywords and TileMatrix are not verified
     * \brief Test whether 2 TileMatrixSet are different
     * \return true if one of their attribute is different or lists have different size
     */
    bool operator!= ( const TileMatrixSet& other ) const;
    /**
     * \~french
     * \brief Retourne la liste des TileMatrix
     * \return liste de TileMatrix
     * \~english
     * \brief Return the list of TileMatrix
     * \return liste of TileMatrix
     */
    std::map<std::string, TileMatrix*>* getTmList();
    /**
     * \~french
     * \brief Retourne la TileMatrix
     * \return TileMatrix
     * \~english
     * \brief Return the TileMatrix
     * \return TileMatrix
     */
    TileMatrix* getTm(std::string id);


    /**
     * \~french
     * \brief Récupère les niveaux ordonnés par résolution décroissante
     * \return Liste de level
     * \~english
     * \brief Get the levels ordered
     * \return List of level
     */
    std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> getOrderedTileMatrix(bool asc) ;

    TileMatrix* getCorrespondingTileMatrix(TileMatrix* tmIn, TileMatrixSet* tmsIn);

    /**
     * \~french
     * \brief Retourne l'indentifiant
     * \return identifiant
     * \~english
     * \brief Return the identifier
     * \return identifier
     */
    std::string getId();
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
     * \brief Précise si le TileMatrixSet est un QTree
     * \return isQTree
     * \~english
     * \brief Precise if the TileMatrixSet is a QTree
     * \return isQTree
     */
    bool getIsQTree();
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
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    std::vector<Keyword>* getKeyWords() ;
    /**
     * \~french
     * \brief Retourne le système de coordonnées utilisé
     * \return crs
     * \~english
     * \brief Return the linked coordinates system
     * \return crs
     */
    CRS* getCrs();

    ///\TODO
    int best_scale ( double resolution_x, double resolution_y );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~TileMatrixSet();
};

#endif /* TILEMATRIXSET_H_ */
