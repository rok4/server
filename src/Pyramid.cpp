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

#include <cmath>
#include "Pyramid.h"
#include <boost/log/trivial.hpp>
#include "Message.h"
#include "processors/Grid.h"
#include "datasource/Decoder.h"
#include "datastream/JPEGEncoder.h"
#include "datastream/PNGEncoder.h"
#include "datastream/TiffEncoder.h"
#include "datastream/BilEncoder.h"
#include "datastream/AscEncoder.h"
#include "image/ExtendedCompoundImage.h"
#include "enums/Format.h"
#include "Level.h"
#include <cfloat>
#include "config.h"
#include "image/EmptyImage.h"

ComparatorLevel compLevelDesc =
    [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
    {
        return elem1.second->getRes() > elem2.second->getRes();
    };

ComparatorLevel compLevelAsc =
    [](std::pair<std::string, Level*> elem1 ,std::pair<std::string, Level*> elem2)
    {
        return elem1.second->getRes() < elem2.second->getRes();
    };


bool Pyramid::parse(json11::Json& doc, ServerConf* serverConf) {

    // TMS
    std::string tmsName;
    if (doc["tile_matrix_set"].is_string()) {
        tmsName = doc["tile_matrix_set"].string_value();
    } else {
        errorMessage = "tile_matrix_set have to be provided and be a string";
        return false;
    }

    tms = serverConf->getTMS(tmsName);
    if ( tms == NULL ) {
        errorMessage =  "Pyramid use unknown TMS [" + tmsName + "]" ;
        return false;
    }

    // FORMAT
    std::string formatStr;
    if (doc["format"].is_string()) {
        formatStr = doc["format"].string_value();
    } else {
        errorMessage = "format have to be provided and be a string";
        return false;
    }

    format = Rok4Format::fromString ( formatStr );
    if ( ! ( format ) ) {
        errorMessage =  "Le format [" + formatStr + "] n'est pas gere." ;
        return false;
    }

    /******************* PYRAMIDE RASTER *********************/
    
    if (Rok4Format::isRaster(format)) {
        if (! doc["raster_specifications"].is_object()) {
            errorMessage = "raster_specifications have to be provided and be an object for raster format";
            return false;
        }

        // PHOTOMETRIE
        std::string photometricStr;
        if (doc["raster_specifications"]["photometric"].is_string()) {
            photometricStr = doc["raster_specifications"]["photometric"].string_value();
        } else {
            errorMessage = "raster_specifications.photometric have to be provided and be a string";
            return false;
        }

        photo = Photometric::fromString ( photometricStr );
        if ( ! ( photo ) ) {
            errorMessage =  "La photométrie [" + photometricStr + "] n'est pas gere." ;
            return false;
        }

        // CHANNELS
        if (doc["raster_specifications"]["channels"].is_number()) {
            channels = doc["raster_specifications"]["channels"].number_value();
        } else {
            errorMessage = "raster_specifications.channels have to be provided and be an integer";
            return false;
        }

        // NODATAVALUE
        nodataValue = new int[channels];
        if (doc["raster_specifications"]["nodata"].is_string()) {
            std::string nodataValueStr = doc["raster_specifications"]["nodata"].string_value();
            std::size_t found = nodataValueStr.find_first_of(",");
            std::string currentValue = nodataValueStr.substr(0,found);
            std::string endOfValues = nodataValueStr.substr(found+1);
            int curVal = atoi(currentValue.c_str());
            if (currentValue == "") {
                curVal = DEFAULT_NODATAVALUE;
            }
            int i = 0;
            nodataValue[i] = curVal;
            i++;
            while (found!=std::string::npos && i < channels) {
                found = endOfValues.find_first_of(",");
                currentValue = endOfValues.substr(0,found);
                endOfValues = endOfValues.substr(found+1);
                curVal = atoi(currentValue.c_str());
                if (currentValue == "") {
                    curVal = DEFAULT_NODATAVALUE;
                }
                nodataValue[i] = curVal;
                i++;
            }
            if (i < channels) {
                errorMessage =  "channels is greater than the count of value for nodata";
                return false;
            }
        } else {
            errorMessage = "raster_specifications.nodata have to be provided and be a string";
            return false;
        }
    }

    /******************* PARTIE COMMUNE *********************/

    // LEVELS
    if (doc["levels"].is_array()) {
        for (json11::Json l : doc["levels"].array_items()) {
            if (l.is_object()) {
                Level* level = new Level(l, serverConf, this, filePath);
                if ( ! level->isOk() ) {
                    errorMessage = "levels contains an invalid level : " + level->getErrorMessage();
                    delete level;
                    return false;
                }

                //on va vérifier que le level qu'on vient de charger n'a pas déjà été chargé
                std::map<std::string, Level*>::iterator it = levels.find ( level->getId() );
                if ( it != levels.end() ) {
                    errorMessage =  "Level " + level->getId() + " defined twice" ;
                    delete level;
                    return false;
                }

                levels.insert ( std::pair<std::string, Level*> ( level->getId(), level ) );
            } else {
                errorMessage = "levels have to be provided and be an object array";
                return false;
            }
        }
    } else {
        errorMessage = "levels have to be provided and be an object array";
        return false;
    }

    if ( levels.size() == 0 ) {
        errorMessage = "No level loaded";
        return false;
    }

    return true;
}

Pyramid::Pyramid(Context* context, std::string path, ServerConf* serverConf) : Configuration(path) {

    nodataValue = NULL;

    /********************** Read */

    int size = -1;
    uint8_t* data = context->readFull(size, filePath);

    if (size < 0) {
        errorMessage = "Cannot read descriptor "  + path ;
        if (data != NULL) delete[] data;
        return;
    }

    /********************** Parse */

    std::string err;
    json11::Json doc = json11::Json::parse ( std::string((char*) data, size), err );
    if ( doc.is_null() ) {
        errorMessage = "Cannot load JSON file "  + filePath + " : " + err ;
        return;
    }
    if (data != NULL) delete[] data;

    /********************** Parse */

    if (! parse(doc, serverConf)) {
        return;
    }

    std::map<std::string, Level*>::iterator itLevel;
    double minRes= DBL_MAX;
    double maxRes= DBL_MIN;
    for ( itLevel=levels.begin(); itLevel!=levels.end(); itLevel++ ) {

        //Determine Higher and Lower Levels
        double d = itLevel->second->getRes();
        if ( minRes > d ) {
            minRes = d;
            lowestLevel = itLevel->second;
        }
        if ( maxRes < d ) {
            maxRes = d;
            highestLevel = itLevel->second;
        }
    }
}

Pyramid::Pyramid (Pyramid* obj) {
    tms = obj->tms;
    format = obj->format;
    lowestLevel = NULL;
    highestLevel = NULL;
    nodataValue = NULL;

    if (Rok4Format::isRaster(format)) {
        photo = obj->photo;
        channels = obj->channels;

        nodataValue = new int[channels];
        memcpy ( nodataValue, obj->nodataValue, channels * sizeof(int) );
    }
}

bool Pyramid::addLevels (Pyramid* obj, std::string bottomLevel, std::string topLevel) {

    // Caractéristiques globales
    if (tms->getId() != obj->tms->getId()) {
        BOOST_LOG_TRIVIAL(error) << "TMS have to be the same for all used pyramids";
        return false;
    }
    if (format != obj->format) {
        BOOST_LOG_TRIVIAL(error) << "Format have to be the same for all used pyramids";
        return false;
    }

    if (Rok4Format::isRaster(format)) {
        if (photo != obj->photo) {
            BOOST_LOG_TRIVIAL(error) << "Photometric have to be the same for all used pyramids";
            return false;
        }
        if (channels != obj->channels) {
            BOOST_LOG_TRIVIAL(error) << "Channels count have to be the same for all used pyramids";
            return false;
        }
    }

    // Niveaux
    bool begin = false;
    bool end = false;
    std::set<std::pair<std::string, Level*>, ComparatorLevel> orderedLevels = obj->getOrderedLevels(true);
    for (std::pair<std::string, Level*> element : orderedLevels) {
        std::string levelId = element.second->getId();
        if (! begin && levelId != bottomLevel) {
            continue;
        }
        begin = true;

        if (getLevel(levelId) != NULL) {
            BOOST_LOG_TRIVIAL(error) << "Level " << levelId << " is already present"  ;
            return false;
        }

        Level* l = new Level(element.second);
        levels.insert ( std::pair<std::string, Level*> ( levelId, l ) );

        if (lowestLevel == NULL || l->getRes() < lowestLevel->getRes()) {
            lowestLevel = l;
        }
        if (highestLevel == NULL || l->getRes() > highestLevel->getRes()) {
            highestLevel = l;
        }

        if (levelId == topLevel) {
            end = true;
            break;
        }
    }

    if (! begin) {
        BOOST_LOG_TRIVIAL(error) << "Bottom level " << bottomLevel << " not found in the input pyramid"  ;
        return false;
    }

    if (! end) {
        BOOST_LOG_TRIVIAL(error) << "Top level " << topLevel << " not found in the input pyramid or lower than the bottom level"  ;
        return false;
    }

    return true;
}

std::string Pyramid::best_level ( double resolution_x, double resolution_y ) {

    // TODO: A REFAIRE !!!!
    // res_level/resx ou resy ne doit pas exceder une certaine valeur
    double resolution = sqrt ( resolution_x * resolution_y );

    std::map<std::string, Level*>::iterator it ( levels.begin() ), itend ( levels.end() );
    std::string best_h = it->first;
    double best = resolution_x / it->second->getRes();
    ++it;
    for ( ; it!=itend; ++it ) {
        double d = resolution / it->second->getRes();
        if ( ( best < 0.8 && d > best ) ||
                ( best >= 0.8 && d >= 0.8 && d < best ) ) {
            best = d;
            best_h = it->first;
        }
    }
    return best_h;
}


Image* Pyramid::getbbox ( ServicesConf* servicesConf, BoundingBox<double> bbox, int width, int height, CRS* dst_crs, Interpolation::KernelType interpolation, int dpi, int& error ) {

    // On calcule la résolution de la requete dans le crs source selon une diagonale de l'image
    double resolution_x, resolution_y;

    BOOST_LOG_TRIVIAL(debug) << "Reprojection " << tms->getCrs()->getProjCode() << " -> " << dst_crs->getProjCode() ;

    if ( servicesConf->are_the_two_CRS_equal( tms->getCrs()->getProjCode(), dst_crs->getProjCode() ) ) {
        resolution_x = ( bbox.xmax - bbox.xmin ) / width;
        resolution_y = ( bbox.ymax - bbox.ymin ) / height;
    } else {
        BoundingBox<double> tmp = bbox;

        if ( ! tmp.reproject ( dst_crs, tms->getCrs() ) ) {
            // BBOX invalide

            BOOST_LOG_TRIVIAL(warning) << "reproject en erreur" ;
            error = 1;
            return 0;
        }

        resolution_x = ( tmp.xmax - tmp.xmin ) / width;
        resolution_y = ( tmp.ymax - tmp.ymin ) / height;
    }

    if (dpi != 0) {
        //si un parametre dpi a ete donne dans la requete, alors on l'utilise
        resolution_x = resolution_x * dpi / 90.7;
        resolution_y = resolution_y * dpi / 90.7;
        //on teste si on vient d'avoir des NaN
        if (resolution_x != resolution_x || resolution_y != resolution_y) {
            error = 3;
            return 0;
        }
    }

    std::string l = best_level ( resolution_x, resolution_y );
    BOOST_LOG_TRIVIAL(debug) <<  "best_level=" << l <<" resolution requete=" << resolution_x << " " << resolution_y  ;

    if ( servicesConf->are_the_two_CRS_equal( tms->getCrs()->getProjCode(), dst_crs->getProjCode() ) ) {
        return levels[l]->getbbox ( servicesConf, bbox, width, height, interpolation, error );
    } else {
        return createReprojectedImage(l, bbox, dst_crs, servicesConf, width, height, interpolation, error);
    }

}

Image* Pyramid::createReprojectedImage(std::string l, BoundingBox<double> bbox, CRS* dst_crs, ServicesConf* servicesConf, int width, int height, Interpolation::KernelType interpolation, int error) {

    bbox.crs = dst_crs->getRequestCode();

    if (bbox.isInAreaOfCRS(dst_crs)) {
        // La bbox entière de l'image demandée est dans l'aire de définition du CRS cible
        return levels[l]->getbbox ( servicesConf, bbox, width, height, tms->getCrs(), dst_crs, interpolation, error );

    } else if (bbox.intersectAreaOfCRS(dst_crs)) {
        // La bbox n'est pas entièrement dans l'aire du CRS, on doit faire la projection que sur la partie intérieure

        BoundingBox<double> croped = bbox.cropToAreaOfCRS(dst_crs);

        double resx = (bbox.xmax - bbox.xmin) / width;
        double resy = (bbox.ymax - bbox.ymin) / height;
        croped.phase(bbox, resx, resy);

        if (croped.hasNullArea()) {
            BOOST_LOG_TRIVIAL(debug) <<   "BBox decoupée d'aire nulle"  ;
            EmptyImage* fond = new EmptyImage(width, height, channels, nodataValue);
            fond->setBbox(bbox);
            return fond;
        }

        int croped_width = int ( ( croped.xmax - croped.xmin ) / resx + 0.5 );
        int croped_height = int ( ( croped.ymax - croped.ymin ) / resy + 0.5 );

        std::vector<Image*> images;
        Image* tmp = levels[l]->getbbox ( servicesConf, croped, croped_width, croped_height, tms->getCrs(), dst_crs, interpolation, error );
        if ( tmp != 0 ) {
            BOOST_LOG_TRIVIAL(debug) <<   "Image decoupée valide"  ;
            images.push_back ( tmp );
        } else {
            BOOST_LOG_TRIVIAL(error) <<   "Image decoupée non valide"  ;
            EmptyImage* fond = new EmptyImage(width, height, channels, nodataValue);
            fond->setBbox(bbox);
            return fond;
        }

        ExtendedCompoundImageFactory facto;
        return facto.createExtendedCompoundImage ( width, height, channels, bbox, images, nodataValue, 0 );
        
    } else {

        BOOST_LOG_TRIVIAL(error) <<  "La bbox de l'image demandée est totalement en dehors de l'aire de définition du CRS de destination " << dst_crs->getProjCode() ;
        BOOST_LOG_TRIVIAL(error) <<  bbox.toString() ;
        return 0;
    }
}

Pyramid::~Pyramid() {

    if (nodataValue != NULL) delete[] nodataValue;

    std::map<std::string, Level*>::iterator iLevel;
    for ( iLevel=levels.begin(); iLevel!=levels.end(); iLevel++ )
        delete iLevel->second;

}

Compression::eCompression Pyramid::getSampleCompression() {
    return Rok4Format::getCompression(format);
}

SampleFormat::eSampleFormat Pyramid::getSampleFormat() {
    return Rok4Format::getSampleFormat(format);
}

int Pyramid::getBitsPerSample() {
    return Rok4Format::getBitsPerSample(format);
}

Level* Pyramid::getHighestLevel() { return highestLevel; }
Level* Pyramid::getLowestLevel() { return lowestLevel; }
Level * Pyramid::getFirstLevel() {
    std::map<std::string, Level*>::iterator it ( levels.begin() );
    return it->second;
}
TileMatrixSet* Pyramid::getTms() { return tms; }
std::map<std::string, Level*>& Pyramid::getLevels() { return levels; }

std::set<std::pair<std::string, Level*>, ComparatorLevel> Pyramid::getOrderedLevels(bool asc) {
 
    if (asc) {
        return std::set<std::pair<std::string, Level*>, ComparatorLevel>(levels.begin(), levels.end(), compLevelAsc);
    } else {
        return std::set<std::pair<std::string, Level*>, ComparatorLevel>(levels.begin(), levels.end(), compLevelDesc);
    }

}

Level* Pyramid::getLevel(std::string id) {
    std::map<std::string, Level*>::iterator it= levels.find ( id );
    if ( it == levels.end() ) {
        return NULL;
    }
    return it->second;
}

Rok4Format::eformat_data Pyramid::getFormat() { return format; }
Photometric::ePhotometric Pyramid::getPhotometric() { return photo; }
int Pyramid::getChannels() { return channels; }
int* Pyramid::getNodataValue() { return nodataValue; }
int Pyramid::getFirstnodataValue () { return nodataValue[0]; }
