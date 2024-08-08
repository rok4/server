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

#include "configurations/Server.h"
#include <cmath>
#include <fstream>

bool ServerConfiguration::parse(json11::Json& doc) {

    // logger
    json11::Json loggerSection = doc["logger"];
    if (loggerSection.is_null()) {
        std::cerr << "No logger section, default values used" << std::endl;
        log_output = DEFAULT_LOG_OUTPUT;
        log_file_prefix = DEFAULT_LOG_FILE_PREFIX;
        log_file_period = DEFAULT_LOG_FILE_PERIOD;
        log_level = DEFAULT_LOG_LEVEL;
    } else if (! loggerSection.is_object()) {
        error_message = "logger have to be an object";
        return false;
    } else {
        // output
        if (loggerSection["output"].is_null()) {
            std::cerr << "No logger.output, default value used" << std::endl;
            log_output = DEFAULT_LOG_OUTPUT;
        } else if (! loggerSection["output"].is_string()) {
            error_message = "logger.output have to be a string";
            return false;
        } else {
            log_output = loggerSection["output"].string_value();
            if ( log_output != "rolling_file" && log_output != "standard_output" && log_output != "static_file" ) {
                error_message = "logger.output '" + loggerSection["output"].string_value() + "' is unknown";
                return false;
            }
        }

        // file_prefix
        if (loggerSection["file_prefix"].is_null()) {
            std::cerr << "No logger.file_prefix, default value used" << std::endl;
            log_file_prefix = DEFAULT_LOG_FILE_PREFIX;
        } else if (! loggerSection["file_prefix"].is_string()) {
            error_message = "logger.file_prefix have to be a string";
            return false;
        } else {
            log_file_prefix = loggerSection["file_prefix"].string_value();
        }

        // file_period
        if (loggerSection["file_period"].is_null()) {
            std::cerr << "No logger.file_period, default value used" << std::endl;
            log_file_period = DEFAULT_LOG_FILE_PERIOD;
        } else if (! loggerSection["file_period"].is_number()) {
            error_message = "logger.file_period have to be a number";
            return false;
        } else {
            log_file_period = loggerSection["file_period"].int_value();
        }

        // level
        if (loggerSection["level"].is_null()) {
            std::cerr << "No logger.level, default value used" << std::endl;
            log_level = DEFAULT_LOG_LEVEL;
        } else if (! loggerSection["level"].is_string()) {
            error_message = "logger.level have to be a string";
            return false;
        } else {
            std::string strLogLevel = loggerSection["level"].string_value();
            if ( strLogLevel == "fatal" ) log_level=boost::log::trivial::fatal;
            else if ( strLogLevel == "error" ) log_level=boost::log::trivial::error;
            else if ( strLogLevel == "warn" ) log_level=boost::log::trivial::warning;
            else if ( strLogLevel == "info" ) log_level=boost::log::trivial::info;
            else if ( strLogLevel == "debug" ) log_level=boost::log::trivial::debug;
            else {
                error_message = "logger.level '" + strLogLevel + "' is unknown";
                return false;
            }
        }
    }

    // cache
    cache_size = -1;
    if (doc["cache"].is_object() && doc["cache"]["size"].is_number() && doc["cache"]["size"].number_value() >= 1) {
        cache_size = doc["cache"]["size"].number_value();
    }
    cache_validity = -1;
    if (doc["cache"].is_object() && doc["cache"]["validity"].is_number() && doc["cache"]["validity"].number_value() >= 1) {
        cache_validity = doc["cache"]["validity"].number_value();
    }

    // threads
    if (doc["threads"].is_null()) {
        std::cerr << "No threads, default value used" << std::endl;
        threads_count = DEFAULT_NB_THREAD;
    } else if (! doc["threads"].is_number()) {
        error_message = "threads have to be a number";
        return false;
    } else {
        threads_count = doc["threads"].int_value();
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
        error_message = "backlog have to be a number";
        return false;
    } else {
        backlog = doc["backlog"].int_value();
    }

    // enabled
    if (doc["enabled"].is_null()) {
        enabled = true;
    } else if (! doc["enabled"].is_bool()) {
        error_message = "enabled have to be a boolean";
        return false;
    } else {
        enabled = doc["enabled"].bool_value();
    }

    // configurations
    json11::Json configurationsSection = doc["configurations"];
    if (configurationsSection.is_null()) {
        error_message = "No configuration section";
        return false;
    } else if (! configurationsSection.is_object()) {
        error_message = "configuration have to be an object";
        return false;
    } else {
        // services
        if (configurationsSection["services"].is_null() || ! configurationsSection["services"].is_string()) {
            error_message = "configurations.services have to be provided and be a string";
            return false;
        } else {
            services_configuration_file = configurationsSection["services"].string_value();
        }

        // layers
        if (configurationsSection["layers"].is_null()) {
            layers_list = "";
        } else if (! configurationsSection["layers"].is_string()) {
            error_message = "configurations.layers have to be a string";
            return false;
        } else {
            layers_list = configurationsSection["layers"].string_value();
        }

        // styles
        if (configurationsSection["styles"].is_null() || ! configurationsSection["styles"].is_string()) {
            error_message = "configurations.styles have to be provided and be a string";
            return false;
        } else {
            StyleBook::set_directory(configurationsSection["styles"].string_value());
        }

        // tile_matrix_sets
        if (configurationsSection["tile_matrix_sets"].is_null() || ! configurationsSection["tile_matrix_sets"].is_string()) {
            error_message = "configurations.tile_matrix_sets have to be provided and be a string";
            return false;
        } else {
            TmsBook::set_directory(configurationsSection["tile_matrix_sets"].string_value());
        }
    }

    return true;
}


ServerConfiguration::ServerConfiguration(std::string path) : Configuration(path) {

    std::cout << "Loading server configuration from file " << file_path << std::endl;

    std::ifstream is(file_path);
    std::stringstream ss;
    ss << is.rdbuf();

    std::string err;
    json11::Json doc = json11::Json::parse ( ss.str(), err );
    if ( doc.is_null() ) {
        error_message = "Cannot load JSON file "  + file_path + " : " + err ;
        return;
    }

    /********************** Parse */

    if (! parse(doc)) {
        return;
    }

    return;
}

/*********************** DESTRUCTOR ********************/

ServerConfiguration::~ServerConfiguration(){ 

    // Les couches
    std::map<std::string, Layer*>::iterator itLay;
    for ( itLay = layers.begin(); itLay != layers.end(); itLay++ )
        delete itLay->second;

}

/******************* GETTERS / SETTERS *****************/

std::string ServerConfiguration::get_log_output() {return log_output;}
int ServerConfiguration::get_log_file_period() {return log_file_period;}
std::string ServerConfiguration::get_log_file_prefix() {return log_file_prefix;}
boost::log::v2_mt_posix::trivial::severity_level ServerConfiguration::get_log_level() {return log_level;}

std::string ServerConfiguration::get_services_configuration_file() {return services_configuration_file;}

std::map<std::string, Layer*>& ServerConfiguration::get_layers() {return layers;}
std::string ServerConfiguration::get_layers_list() {return layers_list;}
void ServerConfiguration::add_layer(Layer* l) {
    layers.insert ( std::pair<std::string, Layer *> ( l->get_id(), l ) );
}
int ServerConfiguration::get_layers_count() {
    return layers.size();
}
Layer* ServerConfiguration::get_layer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layers.find ( id );
    if ( itLay == layers.end() ) {
        return NULL;
    }
    return itLay->second;
}
void ServerConfiguration::delete_layer(std::string id) {
    std::map<std::string, Layer*>::iterator itLay = layers.find ( id );
    if ( itLay != layers.end() ) {
        delete itLay->second;
        layers.erase(itLay);
    }
}

int ServerConfiguration::get_threads_count() {return threads_count;}
std::string ServerConfiguration::get_socket() {return socket;}
bool ServerConfiguration::is_enabled() {return enabled;}
