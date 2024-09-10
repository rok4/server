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
 * \file services/health/gets.cpp
 ** \~french
 * \brief Implémentation de la classe HealthService
 ** \~english
 * \brief Implements classe HealthService
 */

#include <chrono>

#include <rok4/thirdparty/json11.hpp>

#include "services/health/Service.h"
#include "services/health/Threads.h"
#include "services/health/Exception.h"

#include "Rok4Server.h"

DataStream* HealthService::get_health ( Request* req, Rok4Server* serv ) {

    json11::Json res = json11::Json::object {
        { "version", VERSION },
        { "pid", serv->get_pid() },
        { "time", (int) serv->get_time() },
        { "status", serv->get_server_configuration()->is_enabled() ? "OK" : "DISABLED" }
    };

    return new MessageDataStream ( res.dump(), "application/json", 200 );
}

DataStream* HealthService::get_infos ( Request* req, Rok4Server* serv ) {

    std::vector<std::string> layers;
    for(auto const& l: serv->get_server_configuration()->get_layers()) {
        layers.push_back(l.first);
    }

    std::vector<std::string> tms;
    for(auto const& t: TmsBook::get_book()) {
        tms.push_back(t.first);
    }

    std::vector<std::string> styles;
    for(auto const& s: StyleBook::get_book()) {
        styles.push_back(s.first);
    }

    json11::Json res = json11::Json::object {
        { "layers", layers },
        { "tms", tms },
        { "styles", styles }
    };

    return new MessageDataStream ( res.dump(), "application/json", 200 );
}

DataStream* HealthService::get_threads ( Request* req, Rok4Server* serv ) {

    json11::Json res = json11::Json::object {
        { "number", (int) serv->get_threads().size() },
        { "threads", Threads::to_json() }
    };

    return new MessageDataStream ( res.dump(), "application/json", 200 );
}

DataStream* HealthService::get_dependencies ( Request* req, Rok4Server* serv ) {

    int file_count, s3_count, ceph_count, swift_count;
    StoragePool::get_storages_count(file_count, s3_count, ceph_count, swift_count);

    json11::Json res = json11::Json::object {
        { "storage", json11::Json::object {
            { "file", file_count },
            { "s3", s3_count},
            { "swift", ceph_count },
            { "ceph", swift_count }
        } }
    };

    return new MessageDataStream ( res.dump(), "application/json", 200 );
}
