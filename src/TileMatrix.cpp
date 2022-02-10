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

/*
 * TileMatrix.cpp
 */

/**
 * \file TileMatrix.cpp
 * \~french
 * \brief Implémentation de la classe TileMatrix gérant une matrice de tuile représentant un niveau d'une pyramide (Cf TileMatrixSet)
 * \~english
 * \brief Implement the TileMatrix Class handling a matrix of tile reprensenting a level in a pyramid (See TileMatrixSet)
 */

#include "TileMatrix.h"
#include <cmath>

TileMatrix::TileMatrix(json11::Json doc) : Configuration() {

    if (doc["id"].is_string()) {
        id = doc["id"].string_value();
    } else {
        errorMessage = "id have to be provided and be a string";
        return;
    }
    if (doc["cellSize"].is_number()) {
        res = doc["cellSize"].number_value();
    } else {
        errorMessage = "cellSize have to be provided and be a number";
        return;
    }
    if (doc["tileWidth"].is_number()) {
        tileW = doc["tileWidth"].number_value();
    } else {
        errorMessage = "tileWidth have to be provided and be a number";
        return;
    }
    if (doc["tileHeight"].is_number()) {
        tileH = doc["tileHeight"].number_value();
    } else {
        errorMessage = "tileHeight have to be provided and be a number";
        return;
    }
    if (doc["matrixWidth"].is_number()) {
        matrixW = doc["matrixWidth"].number_value();
    } else {
        errorMessage = "matrixWidth have to be provided and be a number";
        return;
    }
    if (doc["matrixHeight"].is_number()) {
        matrixH = doc["matrixHeight"].number_value();
    } else {
        errorMessage = "matrixHeight have to be provided and be a number";
        return;
    }
    if (doc["pointOfOrigin"].is_array() && doc["pointOfOrigin"][0].is_number()) {
        x0 = doc["pointOfOrigin"][0].number_value();
    } else {
        errorMessage = "pointOfOrigin have to be provided and be a number array (2 items)";
        return;
    }
    if (doc["pointOfOrigin"].is_array() && doc["pointOfOrigin"][1].is_number()) {
        y0 = doc["pointOfOrigin"][1].number_value();
    } else {
        errorMessage = "pointOfOrigin have to be provided and be a number array (2 items)";
        return;
    }
}

double   TileMatrix::getRes()    {
    return res;
}
double   TileMatrix::getX0()     {
    return x0;
}
double   TileMatrix::getY0()     {
    return y0;
}
int      TileMatrix::getTileW()  {
    return tileW;
}
int      TileMatrix::getTileH()  {
    return tileH;
}
long int TileMatrix::getMatrixW() {
    return matrixW;
}
long int TileMatrix::getMatrixH() {
    return matrixH;
}

std::string TileMatrix::getId()  {
    return id;
}

TileMatrix& TileMatrix::operator= ( const TileMatrix& other ) {

    this->id = other.id;
    this->res = other.res;
    this->x0 = other.x0;
    this->y0 = other.y0;
    this->tileW = other.tileW;
    this->tileH = other.tileH;
    this->matrixW = other.matrixW;
    this->matrixH = other.matrixH;

    return *this;
}


bool TileMatrix::operator== ( const TileMatrix& other ) const {
    return ( this->res == other.res
             && this->x0 == other.x0
             && this->y0 == other.y0
             && this->tileH == other.tileH
             && this->tileW == other.tileW
             && this->matrixH == other.matrixH
             && this->matrixW == other.matrixW
             && this->id.compare ( other.id ) ==0 );
}

bool TileMatrix::operator!= ( const TileMatrix& other ) const {
    return ! ( *this == other );
}

TileMatrix::~TileMatrix() { }

TileMatrixLimits TileMatrix::bboxToTileLimits(BoundingBox<double> bbox) {

    // On force les indices entre 0 et les max des matrix

    uint32_t minTileCol = std::min(std::max(long((bbox.xmin - x0) / (tileW * res)), 0L), matrixW);
    uint32_t maxTileCol = std::min(std::max(long((bbox.xmax - x0) / (tileW * res)), 0L), matrixW);

    uint32_t minTileRow = std::min(std::max(long((y0 - bbox.ymax) / (tileH * res)), 0L), matrixH);
    uint32_t maxTileRow = std::min(std::max(long((y0 - bbox.ymin) / (tileH * res)), 0L), matrixH);

    return TileMatrixLimits(id, minTileRow, maxTileRow, minTileCol, maxTileCol);
}


BoundingBox<double> TileMatrix::bboxFromTileLimits(TileMatrixLimits limits) {

    // On force les indices entre 0 et les max des matrix

    double xmin = res * limits.getMinTileCol() * tileW + x0;
    double ymax = y0 - res * limits.getMinTileRow() * tileH ;
    double xmax = res * (limits.getMaxTileCol() + 1) * tileW + x0;
    double ymin = y0 - res * (limits.getMaxTileRow() + 1) * tileH;

    return BoundingBox<double>(xmin,ymin,xmax,ymax) ;
}

BoundingBox<double> TileMatrix::tileIndicesToBbox (int col, int row) {

    double xmin = res * col * tileW + x0;
    double ymax = y0 - res * row * tileH ;
    double xmax = xmin + res * tileW;
    double ymin = ymax - res * tileH;

    return BoundingBox<double>(xmin,ymin,xmax,ymax) ;
}
