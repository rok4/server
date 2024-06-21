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

#ifndef WEBSERVICE_H
#define WEBSERVICE_H
#include <string>
#include <map>
#include <vector>
#include <rok4/utils/BoundingBox.h>
#include "curl/curl.h"
#include <rok4/image/EstompageImage.h>
#include <rok4/datasource/DataSource.h>
#include <rok4/datastream/DataStream.h>

struct MemoryStruct {
  uint8_t *memory;
  size_t size;
};


static size_t write_in_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (uint8_t*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    BOOST_LOG_TRIVIAL(error) << "not enough memory (realloc returned NULL)\n";
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

/**
* @class WebService
* @brief Implementation de WebService utilisable par le serveur
* Cette classe mère contient des informations de base pour requêter un
* service quelconque. Puis elle se spécifie en WMS.
* D'autres services pourront ainsi être ajouté.
*/

class WebService {

protected:

    /**
     * \~french \brief URL du serveur
     * \~english \brief Server URL
     */
    std::string url;

    /**
     * \~french \brief Temps d'attente lors de l'envoi d'une requête
     * \~english \brief Waiting time for a request
     */
    int timeout;

    /**
     * \~french \brief Nombre d'essais
     * \~english \brief Number of try
     */
    int retry;

    /**
     * \~french \brief Temps d'attente entre chaque essais
     * \~english \brief Time between each try
     */
    int interval;

    /**
     * \~french \brief Utilisateur pour identification
     * \~english \brief User for identification
     */
    std::string user;

    /**
     * \~french \brief Hash du mot de passe
     * \~english \brief Password hash
     */
    std::string pwd;

    /**
     * \~french \brief Referer
     * \~english \brief Referer
     */
    std::string referer;

    /**
     * \~french \brief User-Agent
     * \~english \brief User-Agent
     */
    std::string user_agent;

    /**
     * \~french \brief responseType, réponse attendue
     * \~english \brief responseType, expected answer
     */
    std::string response_type;

public:


    /**
     * \~french
     * \brief Récupération des données à partir d'une URL
     * \~english
     * \brief Taking Data from an URL
     */
    RawDataSource * perform_request(std::string request);
    
    /**
     * \~french
     * \brief Récupération des données à partir d'une URL
     * \~english
     * \brief Taking Data from an URL
     */
    RawDataStream * perform_request_stream(std::string request);


    /**
     * \~french \brief Constructeur
     * \~english \brief Constructor
     */
    WebService(std::string url, int retry, int interval, int timeout);

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~WebService();

};

#endif // WEBSERVICE_H
