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
 * \file services/Service.cpp
 ** \~french
 * \brief Implémentation de la classe Service
 ** \~english
 * \brief Implements classe Service
 */

#include "services/Service.h"
#include "Request.h"

bool Service::match_route(std::string path, std::vector<std::string> methods, Request* req) {

    if (std::find(methods.begin(), methods.end(), req->method) == methods.end()) {
        return false;
    }

    std::smatch m;
    if (std::regex_match(req->path, m, std::regex(root_path + path))) {

        for(int i = 1; i < m.size(); i++) {
            req->path_params.push_back(m[i]);
            BOOST_LOG_TRIVIAL(debug) << "Path param : " << m[i];
        }

        return true;
    } else {
        return false;
    }
};

Service::Service (json11::Json& doc) {

    if (doc.is_null()) {
        enabled = false;
        return;
    } else if(! doc.is_object()) {
        error_message = "have to be an object";
        return;
    }

    if (doc["enabled"].is_bool()) {
        enabled = doc["enabled"].bool_value();
    } else if (! doc["enabled"].is_null()) {
        error_message = "'enabled' have to be a boolean";
        return;
    } else {
        enabled = false;
    }
};

bool Service::match_request(Request* req) {
    return enabled && req->path.rfind(root_path, 0) == 0;
};


