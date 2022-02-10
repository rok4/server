/*
 * Copyright © (2011) Institut national de l'information
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

#include "Level.h"
#include "enums/Interpolation.h"
#include "datasource/StoreDataSource.h"
#include "image/CompoundImage.h"
#include "image/ResampledImage.h"
#include "image/ReprojectedImage.h"
#include "image/RawImage.h"
#include "datasource/Decoder.h"
#include "datastream/TiffEncoder.h"
#include "datasource/TiffHeaderDataSource.h"
#include <cmath>
#include <boost/log/trivial.hpp>
#include "processors/Kernel.h"
#include <vector>
#include "Pyramid.h"
#include "storage/Context.h"
#include "storage/FileContext.h"
#include "datasource/PaletteDataSource.h"
#include "enums/Format.h"
#include "config.h"
#include <cstddef>
#include <sys/stat.h>

#if OBJECT_ENABLED
#include "storage/object/CephPoolContext.h"
#include "storage/object/S3Context.h"
#include "storage/object/SwiftContext.h"
#endif


#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.

Level::Level ( json11::Json doc, ServerConf* serverConf, Pyramid* pyramid, std::string path) : Configuration(path) {
    nodataValue = NULL;

    // Copie d'informations depuis la pyramide
    format = pyramid->getFormat();

    if (Rok4Format::isRaster(format)) {
        channels = pyramid->getChannels();        
        nodataValue = new int[channels];
        memcpy ( nodataValue, pyramid->getNodataValue(), channels * sizeof(int) );
    } else {
        channels = 0;
    }

    context = NULL;

    // TM
    if (! doc["id"].is_string()) {
        errorMessage = "id have to be provided and be a string";
        return;
    }
    std::string id = doc["id"].string_value();
    TileMatrixSet* tms = pyramid->getTms();
    tm = tms->getTm(id);
    if ( tm == NULL ) {
        errorMessage = "Level " + id + " not in the pyramid TMS [" + tms->getId() + "]"  ;
        return;
    }

    // STOCKAGE
    if (! doc["storage"].is_object()) {
        errorMessage = "Level " + id +": storage have to be provided and be an object";
        return;
    }
    if (! doc["storage"]["type"].is_string()) {
        errorMessage = "Level " + id +": storage.type have to be provided and be a string";
        return;
    }

    /******************* STOCKAGE FICHIER ? *********************/

    if (doc["storage"]["type"].string_value() == "FILE") {

        if (! doc["storage"]["image_directory"].is_string()) {
            errorMessage = "Level " + id +": storage.image_directory have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["path_depth"].is_number()) {
            errorMessage = "Level " + id +": storage.path_depth have to be provided and be an integer";
            return;
        }

        char * fileNameChar = ( char * ) malloc ( strlen ( path.c_str() ) + 1 );
        strcpy ( fileNameChar, path.c_str() );
        char * parentDirChar = dirname ( fileNameChar );
        std::string parent = std::string ( parentDirChar );
        free ( fileNameChar );

        racine = doc["storage"]["image_directory"].string_value() ;
        //Relative Path
        if ( racine.compare ( 0, 2, "./" ) == 0 ) {
            racine.replace ( 0, 1, parent );
        } else if ( racine.compare ( 0, 1, "/" ) != 0 ) {
            racine.insert ( 0,"/" );
            racine.insert ( 0, parent );
        }

        pathDepth = doc["storage"]["path_depth"].number_value();

        context = serverConf->getContextBook()->addContext(ContextType::FILECONTEXT,"");
        if (context == NULL) {
            errorMessage = "Level " + id +": cannot add file storage context";
            return;
        }
    }

#if OBJECT_ENABLED

    /******************* STOCKAGE OBJET ? *********************/

    else if (doc["storage"]["type"].string_value() == "CEPH") {

        if (! doc["storage"]["pool_name"].is_string()) {
            errorMessage = "Level " + id +": storage.pool_name have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["image_prefix"].is_string()) {
            errorMessage = "Level " + id +": storage.image_prefix have to be provided and be a string";
            return;
        }

        racine = doc["storage"]["image_prefix"].string_value();

        context = serverConf->getContextBook()->addContext(ContextType::CEPHCONTEXT, doc["storage"]["pool_name"].string_value());
        if (context == NULL) {
            errorMessage = "Level " + id +": cannot add ceph storage context";
            return;
        }
    }
    else if (doc["storage"]["type"].string_value() == "SWIFT") {

        if (! doc["storage"]["container_name"].is_string()) {
            errorMessage = "Level " + id +": storage.container_name have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["image_prefix"].is_string()) {
            errorMessage = "Level " + id +": storage.image_prefix have to be provided and be a string";
            return;
        }

        racine = doc["storage"]["image_prefix"].string_value();

        context = serverConf->getContextBook()->addContext(ContextType::SWIFTCONTEXT, doc["storage"]["container_name"].string_value());
        if (context == NULL) {
            errorMessage = "Level " + id +": cannot add swift storage context";
            return;
        }
    }
    else if (doc["storage"]["type"].string_value() == "S3") {

        if (! doc["storage"]["bucket_name"].is_string()) {
            errorMessage = "Level " + id +": storage.bucket_name have to be provided and be a string";
            return;
        }
        if (! doc["storage"]["image_prefix"].is_string()) {
            errorMessage = "Level " + id +": storage.image_prefix have to be provided and be a string";
            return;
        }

        racine = doc["storage"]["image_prefix"].string_value();

        context = serverConf->getContextBook()->addContext(ContextType::S3CONTEXT, doc["storage"]["bucket_name"].string_value());
        if (context == NULL) {
            errorMessage = "Level " + id +": cannot add s3 storage context";
            return;
        }
    }

#endif

    if (context == NULL) {
        errorMessage = "Level " + id + " without valid storage informations" ;
        return;
    }

    /******************* PYRAMIDE VECTEUR *********************/
    if (doc["tables"].is_array()) {
        for (json11::Json t : doc["tables"].array_items()) {
            if (t.is_object()) {
                if (! t["name"].is_string()) {
                    errorMessage = "Level " + id +": tables element have to own a name";
                    return;
                }
                std::string tableName  = t["name"].string_value();

                if (! t["geometry"].is_string()) {
                    errorMessage = "Level " + id +": tables element have to own a geometry";
                    return;
                }
                std::string geometry  = t["geometry"].string_value();

                std::vector<Attribute> atts;

                if (t["attributes"].is_array()) {
                    for (json11::Json a : t["attributes"].array_items()) {
                        if (a.is_object()) {
                            Attribute att = Attribute(a);
                            if (att.getMissingField() != "") {
                                errorMessage = "Level " + id +": tables.attributes have to own a field " + att.getMissingField();
                                return;
                            }
                            atts.push_back(att);
                        } else {
                            errorMessage = "Level " + id +": tables.attributes have to be an object array";
                            return;
                        }
                    }
                } else {
                    errorMessage = "Level " + id +": tables.attributes have to be an object array";
                    return;
                }

                tables.push_back(Table(tableName, geometry, atts));
            } else {
                errorMessage = "Level " + id +": tables have to be provided and be an object array";
                return;
            }
        }
    }

    /******************* PARTIE COMMUNE *********************/

    // TILEPERWIDTH

    if (! doc["tiles_per_width"].is_number()) {
        errorMessage = "Level " + id +": tiles_per_width have to be provided and be an integer";
        return;
    }
    tilesPerWidth = doc["tiles_per_width"].number_value();

    // TILEPERHEIGHT

    if (! doc["tiles_per_height"].is_number()) {
        errorMessage = "Level " + id +": tiles_per_height have to be provided and be an integer";
        return;
    }
    tilesPerHeight = doc["tiles_per_height"].number_value();

    if (tilesPerHeight == 0 || tilesPerWidth == 0) {
        errorMessage = "Level " + id +": slab tiles size have to be non zero integers"  ;
        return;
    }

    // TMSLIMITS

    if (! doc["tile_limits"].is_object()) {
        errorMessage = "Level " + id +": tile_limits have to be provided and be an object";
        return;
    }
    if (! doc["tile_limits"]["min_row"].is_number()) {
        errorMessage = "Level " + id +": tile_limits.min_row have to be provided and be an integer";
        return;
    }
    int minTileRow = doc["tile_limits"]["min_row"].number_value();

    if (! doc["tile_limits"]["min_col"].is_number()) {
        errorMessage = "Level " + id +": tile_limits.min_col have to be provided and be an integer";
        return;
    }
    int minTileCol = doc["tile_limits"]["min_col"].number_value();

    if (! doc["tile_limits"]["max_row"].is_number()) {
        errorMessage = "Level " + id +": tile_limits.max_row have to be provided and be an integer";
        return;
    }
    int maxTileRow = doc["tile_limits"]["max_row"].number_value();

    if (! doc["tile_limits"]["max_col"].is_number()) {
        errorMessage = "Level " + id +": tile_limits.max_col have to be provided and be an integer";
        return;
    }
    int maxTileCol = doc["tile_limits"]["max_col"].number_value();

    if ( minTileCol > tm->getMatrixW() || minTileCol < 0 ) minTileCol = 0;
    if ( minTileRow > tm->getMatrixH() || minTileRow < 0 ) minTileRow = 0;
    if ( maxTileCol > tm->getMatrixW() || maxTileCol < 0 ) maxTileCol = tm->getMatrixW();
    if ( maxTileRow > tm->getMatrixH() || maxTileRow < 0 ) maxTileRow = tm->getMatrixH();

    tmLimits = TileMatrixLimits(id, minTileRow, maxTileRow, minTileCol, maxTileCol);

    return;
}


Level::Level ( Level* obj ) : Configuration(obj->filePath), tmLimits(obj->tmLimits) {
    nodataValue = NULL;
    tm = obj->tm;

    channels = obj->channels;
    racine = obj->racine;

    tilesPerWidth = obj->tilesPerWidth;
    tilesPerHeight = obj->tilesPerHeight;

    pathDepth = obj->pathDepth;
    format = obj->format;

    context = obj->context;

    if (Rok4Format::isRaster(format)) {
        nodataValue = new int[channels];
        memcpy ( nodataValue, obj->nodataValue, channels * sizeof(int) );
    } else {
        tables = obj->tables;
    }
}

Level::~Level() {
    if (nodataValue != NULL) delete[] nodataValue;
}


/*
 * A REFAIRE
 */
Image* Level::getbbox ( ServicesConf* servicesConf, BoundingBox< double > bbox, int width, int height, CRS* src_crs, CRS* dst_crs, Interpolation::KernelType interpolation, int& error ) {

    Grid* grid = new Grid ( width, height, bbox );

    grid->bbox.print();

    if ( ! ( grid->reproject ( dst_crs, src_crs ) ) ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Impossible de reprojeter la grid" ;
        error = 1; // BBox invalid
        delete grid;
        return 0;
    }

    //la reprojection peut marcher alors que la bbox contient des NaN
    //cela arrive notamment lors que la bbox envoyée par l'utilisateur n'est pas dans le crs specifié par ce dernier
    if (grid->bbox.xmin != grid->bbox.xmin || grid->bbox.xmax != grid->bbox.xmax || grid->bbox.ymin != grid->bbox.ymin || grid->bbox.ymax != grid->bbox.ymax ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Bbox de la grid contenant des NaN" ;
        error = 1;
        delete grid;
        return 0;
    }

    grid->bbox.print();

    // Calcul de la taille du noyau
    //Maintain previous Lanczos behaviour : Lanczos_2 for resampling and reprojecting
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_2;

    const Kernel& kk = Kernel::getInstance ( interpolation ); // Lanczos_2
    double ratio_x = ( grid->bbox.xmax - grid->bbox.xmin ) / ( tm->getRes() *double ( width ) );
    double ratio_y = ( grid->bbox.ymax - grid->bbox.ymin ) / ( tm->getRes() *double ( height ) );
    double bufx=kk.size ( ratio_x );
    double bufy=kk.size ( ratio_y );

    // bufx<50?bufx=50:0;
    // bufy<50?bufy=50:0; // Pour etre sur de ne pas regresser
    BoundingBox<int64_t> bbox_int ( floor ( ( grid->bbox.xmin - tm->getX0() ) /tm->getRes() - bufx ),
                                    floor ( ( tm->getY0() - grid->bbox.ymax ) /tm->getRes() - bufy ),
                                    ceil ( ( grid->bbox.xmax - tm->getX0() ) /tm->getRes() + bufx ),
                                    ceil ( ( tm->getY0() - grid->bbox.ymin ) /tm->getRes() + bufy ) );

    Image* image = getwindow ( servicesConf, bbox_int, error );
    if ( !image ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Image invalid !"  ;
        return 0;
    }

    image->setBbox ( BoundingBox<double> ( tm->getX0() + tm->getRes() * bbox_int.xmin, tm->getY0() - tm->getRes() * bbox_int.ymax, tm->getX0() + tm->getRes() * bbox_int.xmax, tm->getY0() - tm->getRes() * bbox_int.ymin ) );

    grid->affine_transform ( 1./image->getResX(), -image->getBbox().xmin/image->getResX() - 0.5,
                             -1./image->getResY(), image->getBbox().ymax/image->getResY() - 0.5 );

    return new ReprojectedImage ( image, bbox, grid, interpolation );
}


Image* Level::getbbox ( ServicesConf* servicesConf, BoundingBox< double > bbox, int width, int height, Interpolation::KernelType interpolation, int& error ) {

    // On convertit les coordonnées en nombre de pixels depuis l'origine X0,Y0
    bbox.xmin = ( bbox.xmin - tm->getX0() ) /tm->getRes();
    bbox.xmax = ( bbox.xmax - tm->getX0() ) /tm->getRes();
    double tmp = bbox.ymin;
    bbox.ymin = ( tm->getY0() - bbox.ymax ) /tm->getRes();
    bbox.ymax = ( tm->getY0() - tmp ) /tm->getRes();

    //A VERIFIER !!!!
    BoundingBox<int64_t> bbox_int ( floor ( bbox.xmin + EPS ),
                                    floor ( bbox.ymin + EPS ),
                                    ceil ( bbox.xmax - EPS ),
                                    ceil ( bbox.ymax - EPS ) );

    if ( bbox_int.xmax - bbox_int.xmin == width && bbox_int.ymax - bbox_int.ymin == height &&
            bbox.xmin - bbox_int.xmin < EPS && bbox_int.xmax - bbox.xmax < EPS &&
            bbox.ymin - bbox_int.ymin < EPS && bbox_int.ymax - bbox.ymax < EPS ) {
        /* L'image demandée est en phase et a les mêmes résolutions que les images du niveau
         *   => pas besoin de réechantillonnage */
        return getwindow ( servicesConf, bbox_int, error );
    }

    // Rappel : les coordonnees de la bbox sont ici en pixels
    double ratio_x = ( bbox.xmax - bbox.xmin ) / width;
    double ratio_y = ( bbox.ymax - bbox.ymin ) / height;

    //Maintain previous Lanczos behaviour : Lanczos_3 for resampling only
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_3;
    const Kernel& kk = Kernel::getInstance ( interpolation ); // Lanczos_3

    // On en prend un peu plus pour ne pas avoir d'effet de bord lors du réechantillonnage
    bbox_int.xmin = floor ( bbox.xmin - kk.size ( ratio_x ) );
    bbox_int.xmax = ceil ( bbox.xmax + kk.size ( ratio_x ) );
    bbox_int.ymin = floor ( bbox.ymin - kk.size ( ratio_y ) );
    bbox_int.ymax = ceil ( bbox.ymax + kk.size ( ratio_y ) );

    Image* imageout = getwindow ( servicesConf, bbox_int, error );
    if ( !imageout ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Image invalid !"  ;
        return 0;
    }

    // On affecte la bonne bbox à l'image source afin que la classe de réechantillonnage calcule les bonnes valeurs d'offset
    if (! imageout->setDimensions ( bbox_int.xmax - bbox_int.xmin, bbox_int.ymax - bbox_int.ymin, BoundingBox<double> ( bbox_int ), 1.0, 1.0 ) ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Dimensions invalid !"  ;
        return 0;
    }

    return new ResampledImage ( imageout, width, height, ratio_x, ratio_y, bbox, interpolation, false );
}

int euclideanDivisionQuotient ( int64_t i, int n ) {
    int q=i/n;  // Division tronquee
    if ( q<0 ) q-=1;
    if ( q==0 && i<0 ) q=-1;
    return q;
}

int euclideanDivisionRemainder ( int64_t i, int n ) {
    int r=i%n;
    if ( r<0 ) r+=n;
    return r;
}

Image* Level::getwindow ( ServicesConf* servicesConf, BoundingBox< int64_t > bbox, int& error ) { 
    int tile_xmin=euclideanDivisionQuotient ( bbox.xmin,tm->getTileW() );
    int tile_xmax=euclideanDivisionQuotient ( bbox.xmax -1,tm->getTileW() );
    int nbx = tile_xmax - tile_xmin + 1;
    if ( nbx >= servicesConf->getMaxTileX() ) {
        BOOST_LOG_TRIVIAL(info) <<  "Too Much Tile on X axis"  ;
        error=2;
        return 0;
    }
    if (nbx == 0) {
        BOOST_LOG_TRIVIAL(info) <<  "nbx = 0" ;
        error=1;
        return 0;
    }

    int tile_ymin=euclideanDivisionQuotient ( bbox.ymin,tm->getTileH() );
    int tile_ymax = euclideanDivisionQuotient ( bbox.ymax-1,tm->getTileH() );
    int nby = tile_ymax - tile_ymin + 1;
    if ( nby >= servicesConf->getMaxTileY() ) {
        BOOST_LOG_TRIVIAL(info) <<  "Too Much Tile on Y axis"  ;
        error=2;
        return 0;
    }
    if (nby == 0) {
        BOOST_LOG_TRIVIAL(info) <<  "nby = 0" ;
        error=1;
        return 0;
    }

    int left[nbx];
    memset ( left,   0, nbx*sizeof ( int ) );
    left[0]=euclideanDivisionRemainder ( bbox.xmin,tm->getTileW() );
    int top[nby];
    memset ( top,    0, nby*sizeof ( int ) );
    top[0]=euclideanDivisionRemainder ( bbox.ymin,tm->getTileH() );
    int right[nbx];
    memset ( right,  0, nbx*sizeof ( int ) );
    right[nbx - 1] = tm->getTileW() - euclideanDivisionRemainder ( bbox.xmax -1,tm->getTileW() ) -1;
    int bottom[nby];
    memset ( bottom, 0, nby*sizeof ( int ) );
    bottom[nby- 1] = tm->getTileH() - euclideanDivisionRemainder ( bbox.ymax -1,tm->getTileH() ) - 1;

    std::vector<std::vector<Image*> > T ( nby, std::vector<Image*> ( nbx ) );
    for ( int y = 0; y < nby; y++ ) {
        for ( int x = 0; x < nbx; x++ ) {
            T[y][x] = getTile ( tile_xmin + x, tile_ymin + y, left[x], top[y], right[x], bottom[y] );
        }
    }

    if ( nbx == 1 && nby == 1 ) return T[0][0];
    else return new CompoundImage ( T );
}


/*
 * Recuperation du nom de la dalle du cache en fonction de son indice
 */
std::string Level::getPath ( int tilex, int tiley) {
    // Cas normalement filtré en amont (exception WMS/WMTS)
    if ( tilex < 0 || tiley < 0 ) {
        BOOST_LOG_TRIVIAL(error) << "Negative tile indices: " << tilex << "," << tiley ;
        return "";
    }

    int x,y;

    x = tilex / tilesPerWidth;
    y = tiley / tilesPerHeight;

    return context->getPath(racine,x,y,pathDepth);

}


/*
 * @return la tuile d'indice (x,y) du niveau
 */
DataSource* Level::getEncodedTile ( int x, int y ) { // TODO: return 0 sur des cas d'erreur..
    
    //on stocke une dalle
    // Index de la tuile (cf. ordre de rangement des tuiles)
    int n = ( y % tilesPerHeight ) * tilesPerWidth + ( x % tilesPerWidth );
    std::string path = getPath ( x, y);
    if (path == "") {
        return NULL;
    }
    BOOST_LOG_TRIVIAL(debug) << path;
    return new StoreDataSource ( n, tilesPerWidth * tilesPerHeight, path, Rok4Format::toMimeType ( format ), context, Rok4Format::toEncoding( format ) );
}

DataSource* Level::getDecodedTile ( int x, int y ) {

    DataSource* encData = getEncodedTile ( x, y );
    if (encData == NULL) return 0;

    size_t size;
    if (encData->getData ( size ) == NULL) {
        delete encData;
        return 0;
    }

    if ( format==Rok4Format::TIFF_RAW_INT8 || format==Rok4Format::TIFF_RAW_FLOAT32 )
        return encData;
    else if ( format==Rok4Format::TIFF_JPG_INT8 || format==Rok4Format::TIFF_JPG90_INT8 )
        return new DataSourceDecoder<JpegDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_PNG_INT8 )
        return new DataSourceDecoder<PngDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_LZW_INT8 || format == Rok4Format::TIFF_LZW_FLOAT32 )
        return new DataSourceDecoder<LzwDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_ZIP_FLOAT32 )
        return new DataSourceDecoder<DeflateDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_PKB_INT8 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        return new DataSourceDecoder<PackBitsDecoder> ( encData );
    BOOST_LOG_TRIVIAL(error) <<  "Type d'encodage inconnu : " <<format  ;
    return 0;
}


DataSource* Level::getTile (int x, int y) {

    DataSource* source = getEncodedTile ( x, y );
    if (source == NULL) return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );

    size_t size;
    if (source->getData ( size ) == NULL) {
        delete source;
        return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );
    }

    if ( format == Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_LZW_INT8 ||
         format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_INT8 ||
         format == Rok4Format::TIFF_PKB_FLOAT32 || format == Rok4Format::TIFF_PKB_INT8
        )
    {
        BOOST_LOG_TRIVIAL(debug) <<  "GetTile Tiff"  ;
        TiffHeaderDataSource* fullTiffDS = new TiffHeaderDataSource ( source,format,channels,tm->getTileW(), tm->getTileH() );
        return fullTiffDS;
    }

    return source;
}

Image* Level::getTile ( int x, int y, int left, int top, int right, int bottom ) {
    int pixel_size=1;
    BOOST_LOG_TRIVIAL(debug) <<  "GetTile Image"  ;
    if ( format==Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;

    DataSource* ds = getDecodedTile ( x,y );

    BoundingBox<double> bb ( 
        tm->getX0() + x * tm->getTileW() * tm->getRes() + left * tm->getRes(),
        tm->getY0() - ( y+1 ) * tm->getTileH() * tm->getRes() + bottom * tm->getRes(),
        tm->getX0() + ( x+1 ) * tm->getTileW() * tm->getRes() - right * tm->getRes(),
        tm->getY0() - y * tm->getTileH() * tm->getRes() - top * tm->getRes()
    );

    if (ds == 0) {
        // On crée une image monochrome (valeur fournie dans la pyramide) de la taille qu'aurait du avoir la tuile demandée
        EmptyImage* ei = new EmptyImage(
            tm->getTileW() - left - right, // width
            tm->getTileH() - top - bottom, // height
            channels,
            nodataValue
        );
        ei->setBbox(bb);
        return ei;
    } else {
        return new ImageDecoder (
            ds, tm->getTileW(), tm->getTileH(), channels, bb,
            left, top, right, bottom, pixel_size
        );
    }
}

TileMatrix* Level::getTm () { return tm; }
Rok4Format::eformat_data Level::getFormat () { return format; }
int Level::getChannels () { return channels; }
TileMatrixLimits Level::getTileLimits () { return tmLimits; }

uint32_t Level::getMaxTileRow() {
    return tmLimits.maxTileRow;
}
uint32_t Level::getMinTileRow() {
    return tmLimits.minTileRow;
}
uint32_t Level::getMaxTileCol() {
    return tmLimits.maxTileCol;
}
uint32_t Level::getMinTileCol() {
    return tmLimits.minTileCol;
}
BoundingBox<double> Level::getBboxFromTileLimits() {
    return tm->bboxFromTileLimits(tmLimits);
}
void Level::setTileLimitsFromBbox(BoundingBox<double> bb) {
    tmLimits = tm->bboxToTileLimits(bb);
}

double Level::getRes () { return tm->getRes(); }
std::string Level::getId () { return tm->getId(); }
uint32_t Level::getTilesPerWidth () { return tilesPerWidth; }
uint32_t Level::getTilesPerHeight () { return tilesPerHeight; }
Context* Level::getContext () { return context; }
std::vector<Table>* Level::getTables() { return &tables; }
