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

#include "ServerConf.h"
#include <cmath>

bool ServerConf::parse(json11::Json& doc) {

    // logger
    json11::Json loggerSection = doc["logger"];
    if (loggerSection.is_null()) {
        std::cerr << "No logger section, default values used" << std::endl;
        logOutput = DEFAULT_LOG_OUTPUT;
        logFilePrefix = DEFAULT_LOG_FILE_PREFIX;
        logFilePeriod = DEFAULT_LOG_FILE_PERIOD;
        logLevel = DEFAULT_LOG_LEVEL;
    } else if (! loggerSection.is_object()) {
        errorMessage = "logger have to be an object";
        return false;
    } else {
        // output
        if (loggerSection["output"].is_null()) {
            std::cerr << "No logger.output, default value used" << std::endl;
            logOutput = DEFAULT_LOG_OUTPUT;
        } else if (! loggerSection["output"].is_string()) {
            errorMessage = "logger.output have to be a string";
            return false;
        } else {
            logOutput = loggerSection["output"].string_value();
            if ( logOutput != "rolling_file" && logOutput != "standard_output_stream_for_errors" && logOutput != "static_file" ) {
                errorMessage = "logger.output '" + loggerSection["output"].string_value() + "' is unknown";
                return false;
            }
        }

        // file_prefix
        if (loggerSection["file_prefix"].is_null()) {
            std::cerr << "No logger.file_prefix, default value used" << std::endl;
            logFilePrefix = DEFAULT_LOG_FILE_PREFIX;
        } else if (! loggerSection["file_prefix"].is_string()) {
            errorMessage = "logger.file_prefix have to be a string";
            return false;
        } else {
            logFilePrefix = loggerSection["file_prefix"].string_value();
        }

        // file_period
        if (loggerSection["file_period"].is_null()) {
            std::cerr << "No logger.file_period, default value used" << std::endl;
            logFilePeriod = DEFAULT_LOG_FILE_PERIOD;
        } else if (! loggerSection["file_period"].is_number()) {
            errorMessage = "logger.file_period have to be a number";
            return false;
        } else {
            logFilePeriod = loggerSection["file_period"].int_value();
        }

        // level
        if (loggerSection["level"].is_null()) {
            std::cerr << "No logger.level, default value used" << std::endl;
            logLevel = DEFAULT_LOG_LEVEL;
        } else if (! loggerSection["level"].is_string()) {
            errorMessage = "logger.level have to be a string";
            return false;
        } else {
            std::string strLogLevel = loggerSection["level"].string_value();
            if ( strLogLevel == "fatal" ) logLevel=boost::log::trivial::fatal;
            else if ( strLogLevel == "error" ) logLevel=boost::log::trivial::error;
            else if ( strLogLevel == "warn" ) logLevel=boost::log::trivial::warning;
            else if ( strLogLevel == "info" ) logLevel=boost::log::trivial::info;
            else if ( strLogLevel == "debug" ) logLevel=boost::log::trivial::debug;
            else {
                errorMessage = "logger.level '" + strLogLevel + "' is unknown";
                return false;
            }
        }
    }

    // cache
    cacheSize = -1;
    if (doc["cache"].is_object() && doc["cache"]["size"].is_number() && doc["cache"]["size"].number_value() >= 1) {
        cacheSize = doc["cache"]["size"].number_value();
    }
    cacheValidity = -1;
    if (doc["cache"].is_object() && doc["cache"]["validity"].is_number() && doc["cache"]["validity"].number_value() >= 1) {
        cacheValidity = doc["cache"]["validity"].number_value();
    }

    // threads
    if (doc["threads"].is_null()) {
        std::cerr << "No threads, default value used" << std::endl;
        nbThread = DEFAULT_NB_THREAD;
    } else if (! doc["threads"].is_number()) {
        errorMessage = "threads have to be a number";
        return false;
    } else {
        nbThread = doc["threads"].int_value();
    }

    // port
    if (doc["port"].is_null() || ! doc["port"].is_string() || doc["port"].string_value() == "") {
        std::cerr << "Port have to be provided and have to be a string (example: ':9000')" << std::endl;
        return false;
    } else {
        socket = doc["port"].string_value();
    }

    // backlog
    if (doc["backlog"].is_null()) {
        std::cerr << "No backlog, default value used" << std::endl;
        backlog = 0;
    } else if (! doc["backlog"].is_number()) {
        errorMessage = "backlog have to be a number";
        return false;
    } else {
        backlog = doc["backlog"].int_value();
    }

    // api
    if (doc["api"].is_null()) {
        std::cerr << "No api, default value used" << std::endl;
        supportAdmin = false;
    } else if (! doc["api"].is_bool()) {
        errorMessage = "api have to be a boolean";
        return false;
    } else {
        supportAdmin = doc["api"].bool_value();
    }

    // configurations
    json11::Json configurationsSection = doc["configurations"];
    if (configurationsSection.is_null()) {
        errorMessage = "No configuration section";
        return false;
    } else if (! configurationsSection.is_object()) {
        errorMessage = "configuration have to be an object";
        return false;
    } else {
        // services
        if (configurationsSection["services"].is_null()) {
            std::cerr << "No configurations.services, default value used" << std::endl;
            servicesConfigFile = DEFAULT_SERVICES_CONF_PATH;
        } else if (! configurationsSection["services"].is_string()) {
            errorMessage = "configurations.services have to be a string";
            return false;
        } else {
            servicesConfigFile = configurationsSection["services"].string_value();
        }

        // layers
        if (configurationsSection["layers"].is_null()) {
            std::cerr << "No configurations.layers, default value used" << std::endl;
            layerDir = DEFAULT_LAYER_DIR;
        } else if (! configurationsSection["layers"].is_string()) {
            errorMessage = "configurations.layers have to be a string";
            return false;
        } else {
            layerDir = configurationsSection["layers"].string_value();
        }

        // styles
        if (configurationsSection["styles"].is_null()) {
            std::cerr << "No configurations.styles, default value used" << std::endl;
            styleDir = DEFAULT_STYLE_DIR;
        } else if (! configurationsSection["styles"].is_string()) {
            errorMessage = "configurations.styles have to be a string";
            return false;
        } else {
            styleDir = configurationsSection["styles"].string_value();
        }

        // tile_matrix_sets
        if (configurationsSection["tile_matrix_sets"].is_null()) {
            std::cerr << "No configurations.tile_matrix_sets, default value used" << std::endl;
            tmsDir = DEFAULT_TMS_DIR;
        } else if (! configurationsSection["tile_matrix_sets"].is_string()) {
            errorMessage = "configurations.tile_matrix_sets have to be a string";
            return false;
        } else {
            tmsDir = configurationsSection["tile_matrix_sets"].string_value();
        }
    }

    return true;
}


ServerConf::ServerConf(std::string path) : Configuration(path) {

    std::cout << "Loading server configuration from file " << filePath << std::endl;

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

    return;
}

/*********************** DESTRUCTOR ********************/

ServerConf::~ServerConf(){ 

    // Les TMS
    std::map<std::string, TileMatrixSet*>::iterator itTMS;
    for ( itTMS=tmsList.begin(); itTMS!=tmsList.end(); itTMS++ )
        delete itTMS->second;

    // Les styles
    std::map<std::string, Style*>::iterator itSty;
    for ( itSty=stylesList.begin(); itSty!=stylesList.end(); itSty++ )
        delete itSty->second;

    // Les couches
    std::map<std::string, Layer*>::iterator itLay;
    for ( itLay=layersList.begin(); itLay!=layersList.end(); itLay++ )
        delete itLay->second;

}

/******************* GETTERS / SETTERS *****************/

std::string ServerConf::getLogOutput() {return logOutput;}
int ServerConf::getLogFilePeriod() {return logFilePeriod;}
std::string ServerConf::getLogFilePrefix() {return logFilePrefix;}
boost::log::v2_mt_posix::trivial::severity_level ServerConf::getLogLevel() {return logLevel;}

std::string ServerConf::getServicesConfigFile() {return servicesConfigFile;}

std::string ServerConf::getTmsDir() {return tmsDir;}
std::map<std::string, TileMatrixSet*> ServerConf::getTmsList() {return tmsList;}
void ServerConf::addTMS(TileMatrixSet* t) {
    tmsList.insert ( std::pair<std::string, TileMatrixSet *> ( t->getId(), t ) );
}
int ServerConf::getNbTMS() {
    return tmsList.size();
}
TileMatrixSet* ServerConf::getTMS(std::string id) {
    std::map<std::string, TileMatrixSet*>::iterator tmsIt= tmsList.find ( id );
    if ( tmsIt == tmsList.end() ) {
        return NULL;
    }
    return tmsIt->second;
}
void ServerConf::removeTMS(std::string id) {
    std::map<std::string, TileMatrixSet*>::iterator itTms= tmsList.find ( id );
    if ( itTms != tmsList.end() ) {
        delete itTms->second;
        tmsList.erase(itTms);
    }
}


std::string ServerConf::getStylesDir() {return styleDir;}
std::map<std::string, Style*> ServerConf::getStylesList() {return stylesList;}
void ServerConf::addStyle(Style* s) {
    stylesList.insert ( std::pair<std::string, Style *> ( s->getId(), s ) );
}
int ServerConf::getNbStyles() {
    return stylesList.size();
}
Style* ServerConf::getStyle(std::string id) {
    std::map<std::string, Style*>::iterator styleIt= stylesList.find ( id );
    if ( styleIt == stylesList.end() ) {
        return NULL;
    }
    return styleIt->second;
}
void ServerConf::removeStyle(std::string id) {
    std::map<std::string, Style*>::iterator styleIt= stylesList.find ( id );
    if ( styleIt != stylesList.end() ) {
        delete styleIt->second;
        stylesList.erase(styleIt);
    }
}

std::string ServerConf::getLayersDir() {return layerDir;}
void ServerConf::addLayer(Layer* l) {
    layersList.insert ( std::pair<std::string, Layer *> ( l->getId(), l ) );
}
int ServerConf::getNbLayers() {
    return layersList.size();
}
Layer* ServerConf::getLayer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layersList.find ( id );
    if ( itLay == layersList.end() ) {
        return NULL;
    }
    return itLay->second;
}
void ServerConf::removeLayer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layersList.find ( id );
    if ( itLay != layersList.end() ) {
        delete itLay->second;
        layersList.erase(itLay);
    }
}

int ServerConf::getNbThreads() {return nbThread;}
std::string ServerConf::getSocket() {return socket;}
