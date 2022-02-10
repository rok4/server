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
 * \file TileMatrix.cpp
 * \~french
 * \brief Implémentation de la classe TileMatrixSet gérant une pyramide de matrices (Cf TileMatrix)
 * \~english
 * \brief Implement the TileMatrixSet Class handling a pyramid of matrix (See TileMatrix)
 */


#include "TileMatrixSet.h"
#include "ConfLoader.h"

#include <cmath>


bool TileMatrixSet::parse(json11::Json& doc) {

    // Récupération du CRS
    if (doc["crs"].is_string()) {
        crs = new CRS( doc["crs"].string_value() );
    } else {
        errorMessage = "crs have to be provided and be a string";
        return false;
    }
    

    // Récupération du titre
    if (doc["title"].is_string()) {
        title = doc["title"].string_value();
    }

    // Récupération du résumé
    if (doc["description"].is_string()) {
        abstract = doc["description"].string_value();
    }

    // Récupération des mots clés
    if (doc["keywords"].is_array()) {
        for (json11::Json kw : doc["keywords"].array_items()) {
            if (kw.is_string()) {
                keyWords.push_back(Keyword ( kw.string_value()));
            }
        }
    }

    if (doc["tileMatrices"].is_array()) {
        for (json11::Json tMat : doc["tileMatrices"].array_items()) {
            if (tMat.is_object()) {
                TileMatrix* tm = new TileMatrix(tMat.object_items());
                if (! tm->isOk()) {
                    errorMessage = "tileMatrices contains an invalid level : " + tm->getErrorMessage();
                    delete tm;
                    return false;
                }
                tmList.insert ( std::pair<std::string, TileMatrix*> ( tm->id, tm ) );
            } else {
                errorMessage = "tileMatrices have to be provided and be an object array";
                return false;
            }
        }
    } else {
        errorMessage = "tileMatrices have to be provided and be an object array";
        return false;
    }

    if ( tmList.size() == 0 ) {
        errorMessage =  "No tile matrix in the Tile Matrix Set " + id ;
        return false;
    }

    return true;
}

TileMatrixSet::TileMatrixSet(std::string path) : Configuration(path) {

    crs = NULL;
    isQTree = true;

    /********************** Id */
    id = Configuration::getFileName(filePath, ".json");

    if ( Request::containForbiddenChars(id) ) {
        errorMessage =  "TileMatrixSet identifier contains forbidden chars" ;
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "Add Tile Matrix Set " << id << " from file ";

    /********************** Read */

    std::ifstream is(filePath);
    std::stringstream ss;
    ss << is.rdbuf();

    std::string err;
    json11::Json doc = json11::Json::parse ( ss.str(), err );
    if ( doc.is_null() ) {
        errorMessage = "Cannot load JSON file "  + filePath + " : " + err ;
        return;
    }

    /********************** Parse */

    if (! parse(doc)) {
        return;
    }
    
    // Détection des TMS Quad tree
    std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> ascTM = getOrderedTileMatrix(true);
    bool first = true;
    double res = 0;
    double x0 = 0;
    double y0 = 0;
    int tileW = 0;
    int tileH = 0;
    for (std::pair<std::string, TileMatrix*> element : ascTM) {
        TileMatrix* tm = element.second;
        if (first) {
            // Niveau du bas, de référence
            res = tm->getRes();
            x0 = tm->getX0();
            y0 = tm->getY0();
            tileW = tm->getTileW();
            tileH = tm->getTileH();
            first = false;
            continue;
        }
        if (abs(res * 2 - tm->getRes()) < 0.0001 * res && tm->getX0() == x0 && tm->getY0() == y0 && tm->getTileW() == tileW && tm->getTileH() == tileH) {
            res = tm->getRes();
        } else {
            isQTree = false;
            break;
        }
    }

    return;
}


ComparatorTileMatrix compTMDesc =
    [](std::pair<std::string, TileMatrix*> elem1 ,std::pair<std::string, TileMatrix*> elem2)
    {
        return elem1.second->getRes() > elem2.second->getRes();
    };

ComparatorTileMatrix compTMAsc =
    [](std::pair<std::string, TileMatrix*> elem1 ,std::pair<std::string, TileMatrix*> elem2)
    {
        return elem1.second->getRes() < elem2.second->getRes();
    };

std::string TileMatrixSet::getId() {
    return id;
}
std::map<std::string, TileMatrix*>* TileMatrixSet::getTmList() {
    return &tmList;
}

std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix> TileMatrixSet::getOrderedTileMatrix(bool asc) {
 
    if (asc) {
        return std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix>(tmList.begin(), tmList.end(), compTMAsc);
    } else {
        return std::set<std::pair<std::string, TileMatrix*>, ComparatorTileMatrix>(tmList.begin(), tmList.end(), compTMDesc);
    }

}

bool TileMatrixSet::operator== ( const TileMatrixSet& other ) const {
    return ( this->keyWords.size() ==other.keyWords.size()
             && this->tmList.size() ==other.tmList.size()
             && this->id.compare ( other.id ) == 0
             && this->title.compare ( other.title ) == 0
             && this->abstract.compare ( other.abstract ) == 0
             && this->crs==other.crs );
}

bool TileMatrixSet::operator!= ( const TileMatrixSet& other ) const {
    return ! ( *this == other );
}

TileMatrixSet::~TileMatrixSet() {
    std::map<std::string, TileMatrix*>::iterator itTM;
    for ( itTM=tmList.begin(); itTM != tmList.end(); itTM++ )
        delete itTM->second;

    if (crs != NULL) {
        delete crs;
    }
}

TileMatrix* TileMatrixSet::getTm(std::string id) {

    std::map<std::string, TileMatrix*>::iterator itTM = tmList.find ( id );

    if ( itTM == tmList.end() ) {
        return NULL;
    }

    return itTM->second;
}

CRS* TileMatrixSet::getCrs() {
    return crs;
}

bool TileMatrixSet::getIsQTree() {
    return isQTree;
}

std::vector<Keyword>* TileMatrixSet::getKeyWords() {
    return &keyWords;
}

std::string TileMatrixSet::getAbstract() {
    return abstract;
}
std::string TileMatrixSet::getTitle() {
    return title;
}

TileMatrix* TileMatrixSet::getCorrespondingTileMatrix(TileMatrix* tmIn, TileMatrixSet* tmsIn) {

    TileMatrix* tm = NULL;

    // on calcule la bbox géographique d'intersection des aires de définition des CRS des deux TMS, que l'on reprojete dans chaque CRS
    BoundingBox<double> bboxThis = getCrs()->getCrsDefinitionArea().getIntersection(tmsIn->getCrs()->getCrsDefinitionArea());
    BoundingBox<double> bboxIn = bboxThis;

    if (bboxThis.reproject(CRS::getEpsg4326(), getCrs()) && bboxIn.reproject(CRS::getEpsg4326(), tmsIn->getCrs())) {

        double ratioX, ratioY, resOutX, resOutY;
        double resIn = tmIn->getRes();

        ratioX = (bboxThis.xmax - bboxThis.xmin) / (bboxIn.xmax - bboxIn.xmin);
        ratioY = (bboxThis.ymax - bboxThis.ymin) / (bboxIn.ymax - bboxIn.ymin);

        resOutX = resIn * ratioX;
        resOutY = resIn * ratioY;

        double resolution = sqrt ( resOutX * resOutY );

        // On cherche le niveau du TMS le plus proche (ratio des résolutions le plus proche de 1)
        // On cherche un ration entre 0.8 et 1.5
        std::map<std::string, TileMatrix*>::iterator it = tmList.begin();
        double ratio = 0;

        for ( ; it != tmList.end(); it++ ) {
            double d = resolution / it->second->getRes();
            if (d < 0.8 || d > 1.5) {continue;}
            if (ratio == 0 || abs(d-1) < abs(ratio-1)) {
                ratio = d;
                tm = it->second;
            }
        }
    }

    return tm;
}