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
 * \file services/health/Service.cpp
 ** \~french
 * \brief Implémentation de la classe HealthService
 ** \~english
 * \brief Implements classe HealthService
 */

#include <chrono>

#include "services/health/Service.h"
#include "services/health/Threads.h"
#include "services/health/Exception.h"

#include "Rok4Server.h"

HealthService::HealthService (json11::Json& doc) : Service(doc) {

    if (! isOk()) {
        // Le constructeur du service générique a détecté une erreur, on ajoute simplement le service concerné dans le message
        errorMessage = "HEALTH service: " + errorMessage;
        return;
    }

    if (doc.is_null()) {
        // Le service a déjà été mis comme n'étant pas actif
        return;
    }

    title = "HEALTH service";
    abstract = "HEALTH service";
    keywords.push_back(Keyword ( "health" ));
    keywords.push_back(Keyword ( "check" ));
    root_path = "/healthcheck";
}

DataStream* HealthService::process_request(Request* req, Rok4Server* serv) {
    BOOST_LOG_TRIVIAL(debug) << "HEALTH service";

    if ( match_route( "", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETHEALTH request";
        return get_health(req, serv);
    }
    else if ( match_route( "/info", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETINFOS request";
        return get_infos(req, serv);
    }
    else if ( match_route( "/threads", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETTHREADS request";
        return get_threads(req, serv);
    }
    else if ( match_route( "/depends", {"GET"}, req ) ) {
        BOOST_LOG_TRIVIAL(debug) << "GETDEPENDENCIES request";
        return get_dependencies(req, serv);
    } else {
        throw HealthException::get_error_message("Unknown health request path", 400);
    }
};


DataStream* HealthService::get_health ( Request* req, Rok4Server* serv ) {
    std::ostringstream res;
    
    res << "{\n";
    if (serv->getServerConf()->is_enabled()) {
        res << "  \"status\": \"OK\",\n";
    } else {
        res << "  \"status\": \"DISABLED\",\n";
    }
    res << "  \"version\": \"" << VERSION << "\",\n";
    res << "  \"pid\": " << serv->getPID() << ",\n";
    res << "  \"time\": " << serv->getTime() << "\n";
    res << "}\n";

    return new MessageDataStream(res.str(), "application/json", 200);
}

DataStream* HealthService::get_infos ( Request* req, Rok4Server* serv ) {
    std::ostringstream res;

    res << "{\n";

    // Informations :
    //      layers : []
    //      tms : []
    //      styles : []

    // layers
    auto layers = serv->getServerConf()->get_layers();
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

    return new MessageDataStream(res.str(), "application/json", 200);
}

DataStream* HealthService::get_threads ( Request* req, Rok4Server* serv ) {
    std::ostringstream res;

    res << "{\n";
    res << "  \"number\": " << serv->get_threads().size() << ",\n";
    res << "  \"threads\": [\n";
    res << Threads::print();
    res << "  ]\n";
    res << "}\n";

    return new MessageDataStream(res.str(), "application/json", 200);
}

DataStream* HealthService::get_dependencies ( Request* req, Rok4Server* serv ) {
    std::ostringstream res;

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

    return new MessageDataStream(res.str(), "application/json", 200);
}
