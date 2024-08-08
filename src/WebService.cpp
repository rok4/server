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

#include "WebService.h"

#include <rok4/datastream/DataStream.h>
#include <rok4/datasource/Decoder.h>
#include <rok4/enums/Format.h>
#include <rok4/utils/Cache.h>
#include "config.h"

WebService::WebService(std::string url, int retry = DEFAULT_RETRY, int interval = DEFAULT_INTERVAL,
                       int timeout = DEFAULT_TIMEOUT) : url(url), retry(retry), interval(interval), timeout(timeout) {
    response_type = "";
}

WebService::~WebService() {
}

RawDataSource* WebService::perform_request(std::string request) {
    //----variables
    CURL* curl = CurlPool::get_curl_env();
    CURLcode res, resC, resT;
    long responseCode = 0;
    char* rpType;
    std::string fType;
    struct MemoryStruct chunk;
    bool errors = false;
    RawDataSource* raw_data = NULL;
    int nbPerformed = 0;
    //----

    BOOST_LOG_TRIVIAL(info) << "Perform a request";

    //----Perform request
    while (nbPerformed <= retry) {
        nbPerformed++;
        errors = false;

        BOOST_LOG_TRIVIAL(debug) << "Initialization of Curl Handle";
        //it is one handle - just one per thread - that is a whole theory...
        BOOST_LOG_TRIVIAL(debug) << "Initialization of Chunk structure";
        chunk.memory = (uint8_t*)malloc(1); /* will be grown as needed by the realloc above */
        chunk.size = 0;                     /* no data at this point */

        if (curl) {
            //----Set options
            BOOST_LOG_TRIVIAL(debug) << "Setting options of Curl";
            curl_easy_setopt(curl, CURLOPT_URL, request.c_str());
            /* example.com is redirected, so we tell libcurl to follow redirection */
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            /* Switch on full protocol/debug output while testing, set to 0L to disable */
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
            /* disable progress meter, set to 0L to enable and disable debug output */
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            /* time to connect - not to receive answer */
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, long(timeout));
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, long(timeout));
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "identity");
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "ROK4");
            if (user_agent != "") {
                curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
            }
            /* send all data to this function  */
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_in_memory_callback);
            /* we pass our 'chunk' struct to the callback function */
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
            //----

            BOOST_LOG_TRIVIAL(debug) << "Perform the request => (" << nbPerformed << "/" << retry + 1 << ") time";
            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);

            BOOST_LOG_TRIVIAL(debug) << "Checking for errors";
            /* Check for errors */
            if (res == CURLE_OK) {
                resC = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
                resT = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &rpType);
                if ((resC == CURLE_OK) && responseCode) {
                    if (responseCode != 200) {
                        BOOST_LOG_TRIVIAL(error) << "The request returned a " << responseCode << " code";
                        errors = true;
                    }

                } else {
                    BOOST_LOG_TRIVIAL(error) << "curl_easy_getinfo() on response code failed: " << curl_easy_strerror(resC);
                    errors = true;
                }

                if ((resT == CURLE_OK) && rpType) {
                    std::string rType(rpType);
                    fType = rType;
                    if (errors || (this->response_type != "" && this->response_type != rType)) {
                        BOOST_LOG_TRIVIAL(error) << "The request returned with a " << rpType << " content type";
                        std::string text = "text/";
                        std::string application = "application/";

                        if (rType.find(text) != std::string::npos || rType.find(application) != std::string::npos) {
                            BOOST_LOG_TRIVIAL(error) << "Content of the answer: " << chunk.memory;
                        } else {
                            BOOST_LOG_TRIVIAL(error) << "Impossible to read the answer...";
                        }
                        errors = true;
                    }
                } else {
                    BOOST_LOG_TRIVIAL(error) << "curl_easy_getinfo() on response type failed: " << curl_easy_strerror(resT);
                    errors = true;
                }

                if (!errors) {
                    break;
                }

            } else {
                BOOST_LOG_TRIVIAL(error) << "curl_easy_perform() failed: " << curl_easy_strerror(res);
                errors = true;
            }

            //wait before retry - but not the last time
            if (nbPerformed < retry + 1) {
                sleep(interval);
                free(chunk.memory);
            }

        } else {
            BOOST_LOG_TRIVIAL(error) << "Impossible d'initialiser Curl";
            errors = true;
        }
    }
    //----

    /* Convert chunk into a DataSource readable by rok4 */
    if (!errors) {
        BOOST_LOG_TRIVIAL(debug) << "Sauvegarde de la donnee";
        BOOST_LOG_TRIVIAL(debug) << "content-type de la reponse: " + fType;
        raw_data = new RawDataSource(chunk.memory, chunk.size, fType, "");
    }

    free(chunk.memory);

    return raw_data;
}

RawDataStream* WebService::perform_request_stream(std::string request) {
    RawDataSource* raw_data = this->perform_request(request);
    if (raw_data == NULL) {
        return NULL;
    }
    size_t bufferSize = raw_data->get_size();
    const uint8_t* buffer = raw_data->get_data(bufferSize);
    RawDataStream* rawStream = new RawDataStream((uint8_t*)buffer, bufferSize, raw_data->get_type(), raw_data->get_encoding());
    delete raw_data;
    return rawStream;
}
