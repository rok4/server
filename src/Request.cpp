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
 * \file Request.cpp
 * \~french
 * \brief Implémentation de la classe Request, analysant les requêtes HTTP
 * \~english
 * \brief Implement the Request Class analysing HTTP requests
 */

#include "Request.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <rok4/utils/Cache.h>
#include <rok4/utils/LibcurlStruct.h>

#include "Utils.h"
#include "config.h"

Request::Request(FCGX_Request *fcgx) : fcgx_request(fcgx) {
    // Méthode
    method = std::string(FCGX_GetParam("REQUEST_METHOD", fcgx->envp));

    // Body
    if (method == "POST" || method == "PUT") {
        char *contentBuffer = (char *)malloc(sizeof(char) * 200);
        while (FCGX_GetLine(contentBuffer, 200, fcgx->in)) {
            body.append(contentBuffer);
        }
        free(contentBuffer);
        contentBuffer = NULL;
        body.append("\n");
        BOOST_LOG_TRIVIAL(debug) << "Request Content :" << std::endl
                                 << body;
    }

    // Chemin
    path = std::string(FCGX_GetParam("SCRIPT_NAME", fcgx->envp));
    // Suppression du slash final
    if (path.compare(path.size() - 1, 1, "/") == 0) {
        path.pop_back();
    }

    BOOST_LOG_TRIVIAL(debug) << "Request: " << method << " " << path;

    // Query parameters
    char *query = FCGX_GetParam("QUERY_STRING", fcgx->envp);
    BOOST_LOG_TRIVIAL(debug) << "Query parameters: " << query;
    Utils::url_decode(query);

    std::map<std::string, std::string> params = Utils::string_to_map(std::string(query), "&", "=");

    // On stocke les paramètres de requête avec la clé en minuscule
    std::map<std::string, std::string>::iterator it;
    for (it = params.begin(); it != params.end(); it++) {
        std::string key = it->first;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        query_params.insert(std::pair<std::string, std::string>(key, it->second));
    }
}

Request::Request(std::string m, std::string u, std::map<std::string, std::string> qp) : fcgx_request(NULL), method(m), query_params(qp), url(u) {}

Request::~Request() {}

bool Request::has_query_param(std::string paramName) {
    std::map<std::string, std::string>::iterator it = query_params.find(paramName);
    if (it == query_params.end()) {
        return false;
    }
    return true;
}

std::string Request::get_query_param(std::string paramName) {
    std::map<std::string, std::string>::iterator it = query_params.find(paramName);
    if (it == query_params.end()) {
        return "";
    }
    return it->second;
}

std::string Request::to_string() {
    return method + " " + path + "?" + Utils::map_to_string(query_params, "&", "=");
}

bool Request::is_inspire() {
    std::string inspire = get_query_param("inspire");
    return inspire == "true" || inspire == "1";
}

RawDataStream* Request::send() {

    if (fcgx_request != NULL) {
        BOOST_LOG_TRIVIAL(error) << "Input request cannot be sent";
        return NULL;
    }

    BOOST_LOG_TRIVIAL(info) << "Send request " << method << " " << url << "?" << Utils::map_to_string(query_params, "&", "=");

    CURLcode res;
    DataStruct chunk;
    chunk.nbPassage = 0;
    chunk.data = (char *)malloc(1);
    chunk.size = 0;

    CURL *curl = CurlPool::get_curl_env();

    std::string full_url = url + "?" + Utils::map_to_string(query_params, "&", "=");
    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    if (get_ssl_no_verify()) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "identity");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ROK4 server");

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if (CURLE_OK != res) {
        BOOST_LOG_TRIVIAL(error) <<  method << " " << url << " failed" ;
        BOOST_LOG_TRIVIAL(error) << curl_easy_strerror(res);
        return NULL;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code < 200 || http_code > 299) {
        BOOST_LOG_TRIVIAL(error) <<  method << " " << url << " failed" ;
        BOOST_LOG_TRIVIAL(error) << "Response HTTP code : " << http_code;
        return NULL;
    }

    char* content_type = 0;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

    RawDataStream* stream = new RawDataStream((uint8_t*) chunk.data, chunk.size, std::string(content_type), "");

    return stream;
}