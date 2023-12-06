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
 * \file Rok4Server.cpp
 * \~french
 * \brief Implémentation de la classe Rok4Server et du programme principal
 * \~english
 * \brief Implement the Rok4Server class, handling the event loop
 */

#include "Rok4Server.h"

#include <errno.h>
#include <fcntl.h>
#include <proj.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <cmath>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include <rok4/datastream/AscEncoder.h>
#include <rok4/datastream/BilEncoder.h>
#include <rok4/image/ConvertedChannelsImage.h>
#include <rok4/image/EmptyImage.h>
#include <rok4/enums/Format.h>
#include <rok4/image/Image.h>
#include <rok4/datastream/JPEGEncoder.h>
#include "Layer.h"
#include <rok4/image/MergeImage.h>
#include "Message.h"
#include <rok4/datastream/PNGEncoder.h>
#include <rok4/datasource/PaletteDataSource.h>
#include "ServiceException.h"
#include <rok4/image/StyledImage.h>
#include <rok4/utils/Cache.h>
#include <rok4/datastream/TiffEncoder.h>
#include <rok4/utils/TileMatrixSet.h>
#include "WebService.h"
#include "config.h"
#include <curl/curl.h>
#include <fcgiapp.h>
#include <rok4/utils/Cache.h>
#include "healthcheck/Threads.h"

void hangleSIGALARM(int id) {
    if (id == SIGALRM) {
        exit(0); /* exit on receiving SIGALRM signal */
    }
    signal(SIGALRM, hangleSIGALARM);
}

void* Rok4Server::thread_loop(void* arg) {
    Rok4Server* server = (Rok4Server*)(arg);
    FCGX_Request fcgxRequest;
    if (FCGX_InitRequest(&fcgxRequest, server->sock, FCGI_FAIL_ACCEPT_ON_INTR) != 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Le listener FCGI ne peut etre initialise";
    }

    while (server->isRunning()) {
        std::string content;

        int rc;
        if ((rc = FCGX_Accept_r(&fcgxRequest)) < 0) {
            if (rc == -4) {  // Cas du redémarrage
                BOOST_LOG_TRIVIAL(debug) << "Redémarrage : FCGX_InitRequest renvoie le code d'erreur " << rc;
            } else {
                BOOST_LOG_TRIVIAL(error) << "FCGX_InitRequest renvoie le code d'erreur " << rc;
                std::cerr << "FCGX_InitRequest renvoie le code d'erreur " << rc << std::endl;
            }

            break;
        }

        BOOST_LOG_TRIVIAL(debug) << "Thread " << pthread_self() << " traite une requete";
        Threads::status(ThreadStatus::RUNNING);

        Request* request = new Request(fcgxRequest);
        server->processRequest(request, fcgxRequest);
        delete request;

        FCGX_Finish_r(&fcgxRequest);
        FCGX_Free(&fcgxRequest, 1);

        BOOST_LOG_TRIVIAL(debug) << "Thread " << pthread_self() << " en a fini avec la requete";
        Threads::status(ThreadStatus::AVAILABLE);
    }

    BOOST_LOG_TRIVIAL(debug) << "Extinction du thread";
    return 0;
}

Rok4Server::Rok4Server(ServerConf* svr, ServicesConf* svc) {
    sock = 0;
    servicesConf = svc;
    serverConf = svr;
    
    if (svr->cacheValidity > 0) {
        IndexCache::setValidity(svr->cacheValidity * 60);
    }
    if (svr->cacheSize > 0) {
        IndexCache::setCacheSize(svr->cacheSize);
    }

    threads = std::vector<pthread_t>(serverConf->getNbThreads());

    running = false;

    if (servicesConf->supportWMS) {
        BOOST_LOG_TRIVIAL(debug) << "Build WMS Capabilities 1.3.0";
        buildWMSCapabilities();
    }
    if (servicesConf->supportWMTS) {
        BOOST_LOG_TRIVIAL(debug) << "Build WMTS Capabilities";
        buildWMTSCapabilities();
    }
    if (servicesConf->supportTMS) {
        BOOST_LOG_TRIVIAL(debug) << "Build TMS Capabilities";
        buildTMSCapabilities();
    }
    if (servicesConf->supportOGCTILES) {
        BOOST_LOG_TRIVIAL(debug) << "Build OGC Tiles Capabilities";
        buildOGCTILESCapabilities();
    }
}

Rok4Server::~Rok4Server() {
    delete serverConf;
    delete servicesConf;
}

void Rok4Server::initFCGI() {
    int init = FCGX_Init();
    BOOST_LOG_TRIVIAL(info) << "Listening on " << serverConf->socket;
    sock = FCGX_OpenSocket(serverConf->socket.c_str(), serverConf->backlog);
}

void Rok4Server::run(sig_atomic_t signal_pending) {
    running = true;

    for (int i = 0; i < threads.size(); i++) {
        pthread_create(&(threads[i]), NULL, Rok4Server::thread_loop, (void*)this);
	    Threads::add(threads[i]);
    }

    if (signal_pending != 0) {
        raise(signal_pending);
    }

    for (int i = 0; i < threads.size(); i++)
        pthread_join(threads[i], NULL);
}

void Rok4Server::terminate() {
    running = false;

    // Terminate FCGI Thread
    for (int i = 0; i < threads.size(); i++) {
        pthread_kill(threads[i], SIGQUIT);
    }
}

DataStream* Rok4Server::getMap(Request* request) {
    std::vector<Layer*> layers;
    BoundingBox<double> bbox(0.0, 0.0, 0.0, 0.0);
    int width, height, dpi;
    CRS* crs = NULL;
    std::string format;
    std::vector<Style*> styles;
    std::map<std::string, std::string> format_option;
    std::vector<Image*> images;

    // Récupération des paramètres
    DataStream* errorResp = getMapParamWMS(request, layers, bbox, width, height, crs, format, styles, format_option, dpi);
    if (errorResp) {
        BOOST_LOG_TRIVIAL(error) << "Probleme dans les parametres de la requete getMap";
        if (crs != NULL) delete crs;
        return errorResp;
    }

    int error;
    Image* image;
    for (int i = 0; i < layers.size(); i++) {
        Image* curImage = layers.at(i)->getbbox(servicesConf, bbox, width, height, crs, dpi, error);

        if (curImage == 0) {
            switch (error) {
                case 1: {
                    return new SERDataStream(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "bbox invalide", "wms"));
                }
                case 2: {
                    return new SERDataStream(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "bbox trop grande", "wms"));
                }
                default: {
                    return new SERDataStream(new ServiceException("", OWS_NOAPPLICABLE_CODE, "Impossible de repondre a la requete", "wms"));
                }
            }
        }

        curImage->setBbox(bbox);
        curImage->setCRS(crs);

        Rok4Format::eformat_data pyrType = layers.at(i)->getDataPyramid()->getFormat();
        Style* style = styles.at(i);
        BOOST_LOG_TRIVIAL(debug) << "GetMap de Style : " << styles.at(i)->getId() << " pal size : " << styles.at(i)->getPalette()->getPalettePNGSize();

        Image* image = styleImage(curImage, pyrType, style, format, layers.size(), layers.at(i)->getDataPyramid());

        if (image == 0) {
            return new SERDataStream(new ServiceException("", OWS_NOAPPLICABLE_CODE, "Impossible de repondre a la requete", "wms"));
        }

        images.push_back(image);
    }

    //Use background image format.
    Rok4Format::eformat_data pyrType = layers.at(0)->getDataPyramid()->getFormat();
    Style* style = styles.at(0);

    image = mergeImages(images, pyrType, style, crs, bbox);

    delete crs;

    DataStream* stream = formatImage(image, format, pyrType, format_option, layers.size(), style);

    return stream;
}

Image* Rok4Server::styleImage(Image* curImage, Rok4Format::eformat_data pyrType, Style* style, std::string format, int size, Pyramid* pyr) {
    Image* expandedImage = curImage;

    if (servicesConf->isFullStyleCapable()) {
        if (style && expandedImage->getChannels() == 1 && !(style->getPalette()->getColoursMap()->empty())) {
            if (format == "image/png" && size == 1) {
                switch (pyrType) {
                    case Rok4Format::TIFF_RAW_FLOAT32:
                    case Rok4Format::TIFF_ZIP_FLOAT32:
                    case Rok4Format::TIFF_LZW_FLOAT32:
                    case Rok4Format::TIFF_PKB_FLOAT32:
                        expandedImage = new StyledImage(expandedImage, style->getPalette()->isNoAlpha() ? 3 : 4, style->getPalette());
                    default:
                        break;
                }
            } else {
                expandedImage = new StyledImage(expandedImage, style->getPalette()->isNoAlpha() ? 3 : 4, style->getPalette());
            }
        }
    }

    return expandedImage;
}

Image* Rok4Server::mergeImages(std::vector<Image*> images, Rok4Format::eformat_data& pyrType,
                               Style* style, CRS* crs, BoundingBox<double> bbox) {
    Image* image = images.at(0);
    if (style && !style->getPalette()->getColoursMap()->empty()) {
        switch (pyrType) {
            case Rok4Format::TIFF_RAW_FLOAT32:
                pyrType = Rok4Format::TIFF_RAW_UINT8;
                break;
            case Rok4Format::TIFF_ZIP_FLOAT32:
                pyrType = Rok4Format::TIFF_ZIP_UINT8;
                break;
            case Rok4Format::TIFF_LZW_FLOAT32:
                pyrType = Rok4Format::TIFF_LZW_UINT8;
                break;
            case Rok4Format::TIFF_PKB_FLOAT32:
                pyrType = Rok4Format::TIFF_PKB_UINT8;
                break;
            default:
                break;
        }
    }
    if (images.size() > 1) {
        MergeImageFactory MIF;
        int spp = images.at(0)->getChannels();
        int bg[spp];
        int transparentColor[spp];

        switch (pyrType) {
            case Rok4Format::TIFF_RAW_FLOAT32:
            case Rok4Format::TIFF_ZIP_FLOAT32:
            case Rok4Format::TIFF_LZW_FLOAT32:
            case Rok4Format::TIFF_PKB_FLOAT32:
                switch (spp) {
                    case 1:
                        bg[0] = -99999.0;
                        break;
                    case 2:
                        bg[0] = -99999.0;
                        bg[1] = 0;
                        break;
                    case 3:
                        bg[0] = -99999.0;
                        bg[1] = -99999.0;
                        bg[2] = -99999.0;
                        break;
                    case 4:
                        bg[0] = -99999.0;
                        bg[1] = -99999.0;
                        bg[2] = -99999.0;
                        bg[4] = 0;
                        break;
                    default:
                        memset(bg, 0, sizeof(int) * spp);
                        break;
                }
                memccpy(transparentColor, bg, spp, sizeof(int));
                break;
            case Rok4Format::TIFF_RAW_UINT8:
            case Rok4Format::TIFF_ZIP_UINT8:
            case Rok4Format::TIFF_LZW_UINT8:
            case Rok4Format::TIFF_PKB_UINT8:
            default:
                switch (spp) {
                    case 1:
                        bg[0] = 255;
                        break;
                    case 2:
                        bg[0] = 255;
                        bg[1] = 0;
                        break;
                    case 3:
                        bg[0] = 255;
                        bg[1] = 255;
                        bg[2] = 255;
                        break;
                    case 4:
                        bg[0] = 255;
                        bg[1] = 255;
                        bg[2] = 255;
                        bg[4] = 0;
                        break;
                    default:
                        memset(bg, 0, sizeof(uint8_t) * spp);
                        break;
                }
                break;
        }

        image = MIF.createMergeImage(images, spp, bg, transparentColor, Merge::ALPHATOP);

        if (image == NULL) {
            BOOST_LOG_TRIVIAL(error) << "Impossible de fusionner les images des differentes couches";
            return NULL;
        }
    }

    image->setCRS(crs);
    image->setBbox(bbox);

    return image;
}

DataStream* Rok4Server::formatImage(Image* image, std::string format, Rok4Format::eformat_data pyrType,
                                    std::map<std::string, std::string> format_option,
                                    int size, Style* style) {
    if (format == "image/png") {
        if (size == 1) {
            return new PNGEncoder(image, style->getPalette());
        } else {
            return new PNGEncoder(image, NULL);
        }

    } else if (format == "image/tiff" || format == "image/geotiff") {  // Handle compression option
        bool isGeoTiff = (format == "image/geotiff");

        switch (pyrType) {
            case Rok4Format::TIFF_RAW_FLOAT32:
            case Rok4Format::TIFF_ZIP_FLOAT32:
            case Rok4Format::TIFF_LZW_FLOAT32:
            case Rok4Format::TIFF_PKB_FLOAT32:
                if (getParam(format_option, "compression").compare("lzw") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_LZW_FLOAT32, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("deflate") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_ZIP_FLOAT32, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("raw") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_RAW_FLOAT32, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("packbits") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_PKB_FLOAT32, isGeoTiff);
                }
                return TiffEncoder::getTiffEncoder(image, pyrType, isGeoTiff);
            case Rok4Format::TIFF_RAW_UINT8:
            case Rok4Format::TIFF_ZIP_UINT8:
            case Rok4Format::TIFF_LZW_UINT8:
            case Rok4Format::TIFF_PKB_UINT8:
                if (getParam(format_option, "compression").compare("lzw") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_LZW_UINT8, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("deflate") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_ZIP_UINT8, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("raw") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_RAW_UINT8, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("packbits") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_PKB_UINT8, isGeoTiff);
                }
                return TiffEncoder::getTiffEncoder(image, pyrType, isGeoTiff);
            default:
                if (getParam(format_option, "compression").compare("lzw") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_LZW_UINT8, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("deflate") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_ZIP_UINT8, isGeoTiff);
                }
                if (getParam(format_option, "compression").compare("packbits") == 0) {
                    return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_PKB_UINT8, isGeoTiff);
                }
                return TiffEncoder::getTiffEncoder(image, Rok4Format::TIFF_RAW_UINT8, isGeoTiff);
        }
    } else if (format == "image/jpeg") {
        int quality = 75;
        if (pyrType == Rok4Format::TIFF_JPG90_UINT8 || getParam(format_option, "quality").compare("90") == 0) {
            quality = 90;
        }

        return new JPEGEncoder(image, quality);
    } else if (format == "image/x-bil;bits=32") {
        return new BilEncoder(image);
    } else if (format == "text/asc") {
        // On ne traite le format asc que sur les image à un seul channel
        if (image->getChannels() != 1) {
            BOOST_LOG_TRIVIAL(error) << "Le format " << format << " ne concerne que les images à 1 canal";
        } else {
            return new AscEncoder(image);
        }
    }

    BOOST_LOG_TRIVIAL(error) << "Le format " << format << " ne peut etre traite";

    return new SERDataStream(new ServiceException("", WMS_INVALID_FORMAT, "Le format " + format + " ne peut etre traite", "wms"));
}

DataSource* Rok4Server::getTile(Request* request) {
    Layer* L;
    TileMatrix* tm;
    TileMatrixSet* tms;
    std::string format;
    int tileCol, tileRow;
    Style* style = 0;

    // Récupération des parametres de la requete de type GetTile
    DataSource* errorResp;
    if (request->service == ServiceType::WMTS) {
        errorResp = getTileParamWMTS(request, L, tms, tm, tileCol, tileRow, format, style);
    } else if (request->service == ServiceType::TMS) {
        errorResp = getTileParamTMS(request, L, tms, tm, tileCol, tileRow, format, style);
    } else if (request->service == ServiceType::OGCTILES) {
        errorResp = getTileParamOGCTILES(request, L, tms, tm, tileCol, tileRow, format, style);
    } else {
        // FIXME exception ?
        std::string message = "L'operation GetTile sur ce service n'est pas prise en charge par ce serveur.";
        errorResp = new SERDataSource ( new ServiceException ( "", OWS_OPERATION_NOT_SUPORTED, message, "" ) );
    }

    if (errorResp) {
        return errorResp;
    }
    errorResp = NULL;

    if (tms->getId() == L->getDataPyramid()->getTms()->getId()) {
        // TMS d'interrogation natif
        Level* level = L->getDataPyramid()->getLevel(tm->getId());

        DataSource* d = level->getTile(tileCol, tileRow);
        if (d == NULL) {
            return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND,"No data found", "wmts" ) );
        }

        // Avoid using unnecessary palette
        if (format == "image/png") {
            return new PaletteDataSource(d, style->getPalette());
        } else {
            return d;
        }
    } else {
        // TMS d'interrogation à la demande

        BoundingBox<double> bbox = tm->tileIndicesToBbox(tileCol, tileRow);
        int height = tm->getTileH();
        int width = tm->getTileW();
        CRS* crs = tms->getCrs();
        bbox.crs = crs->getRequestCode();

        std::map<std::string, std::string> format_option;

        int error;
        Image* image = L->getbbox(servicesConf, bbox, width, height, crs, 0, error);

        if (image == NULL) {
            BOOST_LOG_TRIVIAL(warning) << "Cannot process the tile in a non native TMS";
            return new SERDataSource(new ServiceException("", HTTP_NOT_FOUND, "No data found", "wmts"));
        }

        image->setBbox(bbox);
        image->setCRS(crs);

        Image* styledImage = styleImage(image, L->getDataPyramid()->getFormat(), style, format, 1, L->getDataPyramid());

        if (image == 0) {
            BOOST_LOG_TRIVIAL(warning) << "Impossible de styler la tuile demandée dans un TMS non natif";
            return new SERDataSource(new ServiceException("", HTTP_NOT_FOUND, "No data found", "wmts"));
        }

        DataStream* stream = formatImage(styledImage, format, L->getDataPyramid()->getFormat(), format_option, 1, style);
        DataSource* source = new BufferedDataSource(stream);
        delete stream;

        return source;
    }
}

DataStream* Rok4Server::WMSGetFeatureInfo(Request* request) {
    std::vector<Layer*> layers;
    std::vector<Layer*> query_layers;
    BoundingBox<double> bbox(0.0, 0.0, 0.0, 0.0);
    int width, height;
    int X, Y;
    CRS* crs = NULL;
    std::string format;
    std::string info_format;
    int feature_count = 1;
    std::vector<Style*> styles;
    std::map<std::string, std::string> format_option;
    //exception ?

    DataStream* errorResp = getFeatureInfoParamWMS(request, layers, query_layers, bbox, width, height, crs, format, styles, info_format, X, Y, feature_count, format_option);
    if (errorResp) {
        BOOST_LOG_TRIVIAL(error) << "Probleme dans les parametres de la requete getFeatureInfo";
        if (crs != NULL) delete crs;
        return errorResp;
    }

    DataStream* ds = CommonGetFeatureInfo("wms", query_layers.at(0), bbox, width, height, crs, info_format, X, Y, format, feature_count);
    delete crs;
    return ds;
}

DataStream* Rok4Server::WMTSGetFeatureInfo(Request* request) {
    Layer* layer;
    TileMatrix* tm;
    TileMatrixSet* tms;
    std::string format;
    int tileCol, tileRow;
    Style* style = 0;
    int X, Y;
    std::string info_format;

    BOOST_LOG_TRIVIAL(debug) << "WMTSGetFeatureInfo";

    BOOST_LOG_TRIVIAL(debug) << "Verification des parametres de la requete";

    DataStream* errorResp = getFeatureInfoParamWMTS(request, layer, tms, tm, tileCol, tileRow, format, style, info_format, X, Y);

    if (errorResp) {
        BOOST_LOG_TRIVIAL(error) << "Probleme dans les parametres de la requete getFeatureInfo";
        return errorResp;
    }

    BoundingBox<double> bbox = tm->tileIndicesToBbox(tileCol, tileRow);
    int height = tm->getTileH();
    int width = tm->getTileW();
    return CommonGetFeatureInfo("wmts", layer, bbox, width, height, tms->getCrs(), info_format, X, Y, format, 1);
}

DataStream* Rok4Server::OGCTILESGetFeatureInfo(Request* request) {
    // TODO [OGC] process OGC getfeatureinfo...
    BOOST_LOG_TRIVIAL(warning) <<  "Not yet implemented !";
    return new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, "GFI sur l'OGC API Tiles non géré.", "ogctiles"));
}

DataStream* Rok4Server::CommonGetFeatureInfo(std::string service, Layer* layer, BoundingBox<double> bbox, int width, int height, CRS* crs, std::string info_format, int X, int Y, std::string format, int feature_count) {
    std::string getFeatureInfoType = layer->getGFIType();
    if (getFeatureInfoType.compare("PYRAMID") == 0) {
        BOOST_LOG_TRIVIAL(debug) << "GFI sur pyramide";

        BoundingBox<double> pxBbox(0.0, 0.0, 0.0, 0.0);
        pxBbox.xmin = (bbox.xmax - bbox.xmin) / double(width) * double(X) + bbox.xmin;
        pxBbox.xmax = (bbox.xmax - bbox.xmin) / double(width) + pxBbox.xmin;
        pxBbox.ymax = bbox.ymax - (bbox.ymax - bbox.ymin) / double(height) * double(Y);
        pxBbox.ymin = pxBbox.ymax - (bbox.ymax - bbox.ymin) / double(height);

        int error;
        Image* image;
        image = layer->getbbox(servicesConf, pxBbox, 1, 1, crs, 0, error);
        if (image == 0) {
            switch (error) {
                case 1: {
                    return new SERDataStream(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "bbox invalide", service));
                }
                case 2: {
                    return new SERDataStream(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "bbox trop grande", service));
                }
                default: {
                    return new SERDataStream(new ServiceException("", OWS_NOAPPLICABLE_CODE, "Impossible de repondre a la requete", service));
                }
            }
        }

        std::vector<std::string> strData;
        Rok4Format::eformat_data pyrType = layer->getDataPyramid()->getFormat();
        int n = image->getChannels();
        switch (pyrType) {
            case Rok4Format::TIFF_RAW_UINT8:
            case Rok4Format::TIFF_JPG_UINT8:
            case Rok4Format::TIFF_PNG_UINT8:
            case Rok4Format::TIFF_LZW_UINT8:
            case Rok4Format::TIFF_ZIP_UINT8:
            case Rok4Format::TIFF_PKB_UINT8: {
                uint8_t* intbuffer = new uint8_t[n * sizeof(uint8_t)];
                image->getline(intbuffer, 0);
                for (int i = 0; i < n; i++) {
                    std::stringstream ss;
                    ss << (int)intbuffer[i];
                    strData.push_back(ss.str());
                }
                break;
            }
            case Rok4Format::TIFF_RAW_FLOAT32:
            case Rok4Format::TIFF_LZW_FLOAT32:
            case Rok4Format::TIFF_ZIP_FLOAT32:
            case Rok4Format::TIFF_PKB_FLOAT32: {
                float* floatbuffer = new float[n * sizeof(float)];
                image->getline(floatbuffer, 0);
                for (int i = 0; i < n; i++) {
                    std::stringstream ss;
                    ss.setf ( std::ios::fixed,std::ios::floatfield );
                    ss << (float)floatbuffer[i];
                    strData.push_back(ss.str());
                }
                delete[] floatbuffer;
                break;
            }
            default:
                return new SERDataStream(new ServiceException("", OWS_NOAPPLICABLE_CODE, "Erreur interne.", service));
        }
        GetFeatureInfoEncoder gfiEncoder(strData, info_format);
        DataStream* responseDS = gfiEncoder.getDataStream();
        if (responseDS == NULL) {
            return new SERDataStream(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "Info_format non " + info_format + " supporté par la couche " + layer->getId(), service));
        }
        delete image;
        return responseDS;

    } else if (getFeatureInfoType.compare("EXTERNALWMS") == 0) {
        BOOST_LOG_TRIVIAL(debug) << "GFI sur WMS externe";
        WebService* myWMSV = new WebService(layer->getGFIBaseUrl(), 1, 1, 10);
        std::stringstream vectorRequest;
        std::string crsstring = crs->getRequestCode();
        if (layer->getGFIForceEPSG()) {
            // FIXME
            if (crsstring == "IGNF:LAMB93") {
                crsstring = "EPSG:2154";
            }
        }

        vectorRequest << layer->getGFIBaseUrl()
                      << "REQUEST=GetFeatureInfo"
                      << "&SERVICE=" << layer->getGFIService()
                      << "&VERSION=" << layer->getGFIVersion()
                      << "&STYLES=&LAYERS=" << layer->getGFILayers()
                      << "&QUERY_LAYERS=" << layer->getGFIQueryLayers()
                      << "&INFO_FORMAT=" << info_format
                      << "&FORMAT=" << format
                      << "&FEATURE_COUNT=" << feature_count
                      << "&CRS=" << crsstring
                      << "&WIDTH=" << width
                      << "&HEIGHT=" << height
                      << "&I=" << X
                      << "&J=" << Y
                      // compatibilité 1.1.1
                      << "&SRS=" << crsstring
                      << "&X=" << X
                      << "&Y=" << Y;

        // Les params sont ok : on passe maintenant a la recup de l'info
        char xmin[64];
        sprintf(xmin, "%-.*G", 16, bbox.xmin);
        char xmax[64];
        sprintf(xmax, "%-.*G", 16, bbox.xmax);
        char ymin[64];
        sprintf(ymin, "%-.*G", 16, bbox.ymin);
        char ymax[64];
        sprintf(ymax, "%-.*G", 16, bbox.ymax);

        if ((crs->getAuthority() == "EPSG" || layer->getGFIForceEPSG()) && crs->isLongLat() && layer->getGFIVersion() == "1.3.0") {
            vectorRequest << "&BBOX=" << ymin << "," << xmin << "," << ymax << "," << xmax;
        } else {
            vectorRequest << "&BBOX=" << xmin << "," << ymin << "," << xmax << "," << ymax;
        }

        BOOST_LOG_TRIVIAL(debug) << "REQUETE = " << vectorRequest.str();
        RawDataStream* response = myWMSV->performRequestStream(vectorRequest.str());
        if (response == NULL) {
            delete myWMSV;
            return new SERDataStream(new ServiceException("", OWS_NOAPPLICABLE_CODE, "Internal server error", "wms"));
        }

        delete myWMSV;
        return response;
    } else if (getFeatureInfoType.compare("SQL") == 0) {
        BOOST_LOG_TRIVIAL(debug) << "GFI sur SQL";
        // Non géré pour le moment. (nouvelle lib a integrer)
        return new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, "GFI depuis un SQL non géré.", service));
    } else {
        return new SERDataStream(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "ERRORRRRRR !", service));
    }
}

void Rok4Server::processWMTS(Request* request, FCGX_Request& fcgxRequest) {
    if (request->request == RequestType::GETCAPABILITIES) {
        S.sendresponse(WMTSGetCapabilities(request), &fcgxRequest);
    } else if (request->request == RequestType::GETTILE) {
        S.sendresponse(getTile(request), &fcgxRequest);
    } else if (request->request == RequestType::GETFEATUREINFO) {
        S.sendresponse(WMTSGetFeatureInfo(request), &fcgxRequest);
    } else if (request->request == RequestType::GETVERSION) {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, ("L'operation ") + request->getParam("request") + " n'est pas prise en charge par ce serveur.", "wmts")), &fcgxRequest);
    } else if (request->request == RequestType::REQUEST_MISSING) {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_MISSING_PARAMETER_VALUE, ("Le parametre REQUEST n'est pas renseigne."), "wmts")), &fcgxRequest);
    } else {
        S.sendresponse(new SERDataSource(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, "L'operation " + request->getParam("request") + " n'est pas prise en charge par ce serveur.", "wmts")), &fcgxRequest);
    }
}

void Rok4Server::processGlobal(Request* request, FCGX_Request& fcgxRequest) {
    if (request->request == RequestType::GETSERVICES) {
        S.sendresponse(GlobalGetServices(request), &fcgxRequest);
    } else {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, std::string("L'operation n'est pas prise en charge par ce serveur."), "global")), &fcgxRequest);
    }
}

void Rok4Server::processTMS(Request* request, FCGX_Request& fcgxRequest) {
    if (request->request == RequestType::GETCAPABILITIES) {
        S.sendresponse(TMSGetCapabilities(request), &fcgxRequest);
    } else if (request->request == RequestType::GETTILE) {
        S.sendresponse(getTile(request), &fcgxRequest);
    } else if (request->request == RequestType::GETLAYER) {
        S.sendresponse(TMSGetLayer(request), &fcgxRequest);
    } else if (request->request == RequestType::GETLAYERMETADATA) {
        S.sendresponse(TMSGetLayerMetadata(request), &fcgxRequest);
    } else if (request->request == RequestType::GETLAYERGDAL) {
        S.sendresponse(TMSGetLayerGDAL(request), &fcgxRequest);
    } else {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, std::string("L'operation n'est pas prise en charge par ce serveur."), "tms")), &fcgxRequest);
    }
}

void Rok4Server::processAdmin(Request* request, FCGX_Request& fcgxRequest) {
    if (request->request == RequestType::ADDLAYER) {
        S.sendresponse(AdminCreateLayer(request), &fcgxRequest);
    } else if (request->request == RequestType::BUILDCAPABILITIES) {
        S.sendresponse(AdminBuildCapabilities(request), &fcgxRequest);
    } else if (request->request == RequestType::UPDATELAYER) {
        S.sendresponse(AdminUpdateLayer(request), &fcgxRequest);
    } else if (request->request == RequestType::DELETELAYER) {
        S.sendresponse(AdminDeleteLayer(request), &fcgxRequest);
    } else {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, std::string("L'operation n'est pas prise en charge par ce serveur."), "admin")), &fcgxRequest);
    }
}

void Rok4Server::processHealthCheck(Request *request, FCGX_Request &fcgxRequest)
{
    std::ostringstream res;
    if (request->request == RequestType::GETHEALTHSTATUS) {
        res << "{\n";
        res << "  \"status\": \"OK\",\n";
        res << "  \"version\": \"" << VERSION << "\",\n";
        res << "  \"pid\": " << this->getPID() << ",\n";
        res << "  \"time\": " << this->getTime() << "\n";
        res << "}\n";
        
        S.sendresponse(new MessageDataStream(res.str(), "application/json"), &fcgxRequest);
    }
    else if (request->request == RequestType::GETINFOSTATUS)
    {
        res << "{\n";

        // Informations :
        //      layers : []
        //      tms : []
        //      styles : []

        // layers
        auto layers = this->getLayerList();
        std::map<std::string, Layer *>::iterator itl = layers.begin();
        res << "    \"layers\": [\n";
        while(itl != layers.end()) {
            res << "      \"" << itl->first << "\"";
            if (++itl != layers.end()) {
                res << ",";
            }
            res << "\n";
        }
        res << "    ],\n";

        // tms
        auto tms = TmsBook::get_book();
        std::map<std::string, TileMatrixSet *>::iterator itt = tms.begin();
        res << "    \"tms\": [\n";
        while(itt != tms.end()) {
            res << "      \"" << itt->first << "\"";
            if (++itt != tms.end()) {
                res << ",";
            }
            res << "\n";
        }
        res << "    ],\n";

        // styles
        auto styles = StyleBook::get_book();
        std::map<std::string, Style *>::iterator its = styles.begin();
        res << "    \"styles\": [\n";
        while(its != styles.end()) {
            res << "      \"" << its->first << "\"";
            if (++its != styles.end()) {
                res << ",";
            }
            res << "\n";
        }
        res << "    ]\n";

        res << "}\n";
        S.sendresponse(new MessageDataStream(res.str(), "application/json"), &fcgxRequest);
    }
    else if (request->request == RequestType::GETTHREADSTATUS)
    {
        res << "{\n";
        res << "  \"number\": " << this->threads.size() << ",\n";
        res << "  \"threads\": [\n";
        res << Threads::print();
        res << "  ]\n";
        res << "}\n";
        S.sendresponse(new MessageDataStream(res.str(), "application/json"), &fcgxRequest);
    }
    else if (request->request == RequestType::GETDEPENDSTATUS)
    {
        res << "{\n";
        res << "    \"storage\": {\n";

        int file_count, s3_count, ceph_count, swift_count;
        StoragePool::getStorageCounts(file_count, s3_count, ceph_count, swift_count);
        
        res << "      \"file\": " << file_count << ",\n";
        res << "      \"s3\": " << s3_count << ",\n";
        res << "      \"swift\": " << swift_count << ",\n";
        res << "      \"ceph\": " << ceph_count << "\n";

        res << "    }\n";
        res << "}\n";
        S.sendresponse(new MessageDataStream(res.str(), "application/json"), &fcgxRequest);
    }
    else
    {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, std::string("L'operation n'est pas prise en charge par ce serveur."), "healthcheck")), &fcgxRequest);
    }
}

void Rok4Server::processWMS(Request* request, FCGX_Request& fcgxRequest) {
    if (request->request == RequestType::GETCAPABILITIES) {
        S.sendresponse(WMSGetCapabilities(request), &fcgxRequest);
    } else if (request->request == RequestType::GETMAP) {
        S.sendresponse(getMap(request), &fcgxRequest);
    } else if (request->request == RequestType::GETFEATUREINFO) {
        S.sendresponse(WMSGetFeatureInfo(request), &fcgxRequest);
    } else if (request->request == RequestType::GETVERSION) {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, ("L'operation ") + request->getParam("request") + " n'est pas prise en charge par ce serveur.", "wms")), &fcgxRequest);
    } else if (request->request == RequestType::REQUEST_MISSING) {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_MISSING_PARAMETER_VALUE, ("Le parametre REQUEST n'est pas renseigne."), "wms")), &fcgxRequest);
    } else {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, ("L'operation ") + request->getParam("request") + " n'est pas prise en charge par ce serveur.", "wms")), &fcgxRequest);
    }
}

void Rok4Server::processOGCTILES(Request* request, FCGX_Request& fcgxRequest) {
    if (request->request == RequestType::GETCAPABILITIES) {
        S.sendresponse(OGCTILESGetCapabilities(request), &fcgxRequest);
    } else if (request->request == RequestType::GETMAPTILE) {
        S.sendresponse(getTile(request), &fcgxRequest);
    } else if (request->request == RequestType::GETTILE) {
        S.sendresponse(getTile(request), &fcgxRequest);
    } else if (request->request == RequestType::GETFEATUREINFO) {
        S.sendresponse(OGCTILESGetFeatureInfo(request), &fcgxRequest);
    } else {
        S.sendresponse(new SERDataStream(new ServiceException("", OWS_OPERATION_NOT_SUPORTED, std::string("L'operation n'est pas prise en charge par ce serveur."), "ogctiles")), &fcgxRequest);
    }
}

void Rok4Server::processRequest(Request* request, FCGX_Request& fcgxRequest) {
    if (request->service == ServiceType::GLOBAL) {
        processGlobal(request, fcgxRequest);
    } else if (servicesConf->supportWMTS && request->service == ServiceType::WMTS) {
        processWMTS(request, fcgxRequest);
    } else if (servicesConf->supportWMS && request->service == ServiceType::WMS) {
        processWMS(request, fcgxRequest);
    } else if (servicesConf->supportTMS && request->service == ServiceType::TMS) {
        processTMS(request, fcgxRequest);
    } else if (servicesConf->supportOGCTILES && request->service == ServiceType::OGCTILES) {
        processOGCTILES(request, fcgxRequest);
    } else if (serverConf->supportAdmin && request->service == ServiceType::ADMIN) {
        processAdmin(request, fcgxRequest);
    } else if (request->service == ServiceType::HEALTHCHECK) {
        processHealthCheck(request, fcgxRequest);
    } else {
        S.sendresponse(new SERDataSource(new ServiceException("", OWS_INVALID_PARAMETER_VALUE, "Le service est inconnu pour ce serveur.", "global")), &fcgxRequest);
    }
}

/******************* GETTERS / SETTERS *****************/

ServicesConf* Rok4Server::getServicesConf() { return servicesConf; }
ServerConf* Rok4Server::getServerConf() { return serverConf; }
std::map<std::string, Layer*>& Rok4Server::getLayerList() { return serverConf->layersList; }

int Rok4Server::getFCGISocket() { return sock; }
void Rok4Server::setFCGISocket(int sockFCGI) { sock = sockFCGI; }
int Rok4Server::getPID() { return pid; }
void Rok4Server::setPID(int processID) { pid = processID; }
long Rok4Server::getTime() { return time; }
void Rok4Server::setTime(long processTime) { time = processTime; }
bool Rok4Server::isRunning() { return running; }
