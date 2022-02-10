/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
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
 * \file TileMatrixLimits.h
 * \~french
 * \brief Définition de la classe TileMatrixLimits gérant les indices extrêmes de tuiles pour un niveau
 * \~english
 * \brief Define the TileMatrixLimits class handling extrems tiles' indices for a level
 */

class TileMatrixLimits;

#ifndef TILEMATRIXLIMITS_H
#define TILEMATRIXLIMITS_H

#include <string>
#include "UtilsXML.h"
#include "utils/Configuration.h"

/**
 * \author Institut national de l'information géographique et forestière
 */
class TileMatrixLimits
{

    friend class Level;
    friend class Layer;

    public:
        /**
         * \~french
         * \brief Crée un TileMatrixLimits à partir des ses éléments constitutifs
         * \param[in] id Identifiant de niveau
         * \param[in] rmin Indice de ligne minimal
         * \param[in] rmax Indice de ligne maximal
         * \param[in] cmin Indice de colonne minimal
         * \param[in] cmax Indice de colonne maximal
         * \~english
         * \param[in] id layer
         * \param[in] rmin Min row indice
         * \param[in] rmax Max row indice
         * \param[in] cmin Min column indice
         * \param[in] cmax Max column indice
         */

        TileMatrixLimits(std::string id, uint32_t rmin, uint32_t rmax, uint32_t cmin, uint32_t cmax) {
            tileMatrixId = id;
            minTileRow = rmin;
            maxTileRow = rmax;
            minTileCol = cmin;
            maxTileCol = cmax;
        };
        
        /**
         * \~french
         * \brief Crée un TileMatrixLimits vide
         * \~english
         * \brief Create an empty TileMatrixLimits
         */

        TileMatrixLimits() {};

        TileMatrixLimits(TileMatrixLimits const& other) {
            tileMatrixId = other.tileMatrixId;
            minTileRow = other.minTileRow;
            maxTileRow = other.maxTileRow;
            minTileCol = other.minTileCol;
            maxTileCol = other.maxTileCol;
        };

        /**
         * \~french
         * \brief Affectation
         * \~english
         * \brief Assignement
         */
        TileMatrixLimits& operator= ( TileMatrixLimits const& other ) {
            if ( this != &other ) {
                this->tileMatrixId = other.tileMatrixId;
                this->minTileRow = other.minTileRow;
                this->maxTileRow = other.maxTileRow;
                this->minTileCol = other.minTileCol;
                this->maxTileCol = other.maxTileCol;
            }
            return *this;
        }

        ~TileMatrixLimits(){};

        uint32_t getMinTileRow() {return minTileRow;}
        uint32_t getMaxTileRow() {return maxTileRow;}
        uint32_t getMinTileCol() {return minTileCol;}
        uint32_t getMaxTileCol() {return maxTileCol;}


        /**
         * \~french \brief Export XML pour le GetCapabilities WMTS
         * \~english \brief XML export for WMTS GetCapabilities
         */
        TiXmlElement* getWmtsXml() {
            TiXmlElement* tmLimitsEl = new TiXmlElement ( "TileMatrixLimits" );
            tmLimitsEl->LinkEndChild ( UtilsXML::buildTextNode ( "TileMatrix", tileMatrixId ) );

            tmLimitsEl->LinkEndChild ( UtilsXML::buildTextNode ( "MinTileRow", std::to_string(minTileRow) ) );
            tmLimitsEl->LinkEndChild ( UtilsXML::buildTextNode ( "MaxTileRow", std::to_string(maxTileRow) ) );
            tmLimitsEl->LinkEndChild ( UtilsXML::buildTextNode ( "MinTileCol", std::to_string(minTileCol) ) );
            tmLimitsEl->LinkEndChild ( UtilsXML::buildTextNode ( "MaxTileCol", std::to_string(maxTileCol) ) );
            
            return tmLimitsEl;
        }

        bool containTile(int col, int row) {
            return (row >= minTileRow && row <= maxTileRow && col >= minTileCol && col <= maxTileCol);
        }

    protected:

        /**
         * \~french \brief Identifiant de niveau
         * \~english \brief Level identifier
         */
        std::string tileMatrixId;
        /**
         * \~french \brief Indice de ligne maximal
         * \~english \brief Max row indice
         */
        uint32_t maxTileRow;
        /**
         * \~french \brief Indice de ligne minimal
         * \~english \brief Min row indice
         */
        uint32_t minTileRow;
        /**
         * \~french \brief Indice de colonne maximal
         * \~english \brief Max column indice
         */
        uint32_t maxTileCol;
        /**
         * \~french \brief Indice de colonne maximal
         * \~english \brief Min column indice
         */
        uint32_t minTileCol;
};

#endif // TILEMATRIXLIMITS_H

