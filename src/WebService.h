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


static size_t WriteInMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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
    std::string userAgent;

    /**
     * \~french \brief responseType, réponse attendue
     * \~english \brief responseType, expected answer
     */
    std::string responseType;

public:

    /**
     * \~french \brief Récupère l'url
     * \return url
     * \~english \brief Get the url
     * \return url
     */
    std::string getUrl(){
        return url;
    }

    /**
     * \~french \brief Modifie l'url
     * \param[in] url
     * \~english \brief Set the url
     * \param[in] url
     */
    void setUrl (std::string u) {
        url = u;
    }

    /**
     * \~french \brief Récupère le user
     * \return user
     * \~english \brief Get the user
     * \return user
     */
    std::string getUser(){
        return user;
    }

    /**
     * \~french \brief Modifie le user
     * \param[in] user
     * \~english \brief Set the user
     * \param[in] user
     */
    void setUser (std::string u) {
        user = u;
    }

    /**
     * \~french \brief Récupère le pwd
     * \return pwd
     * \~english \brief Get the pwd
     * \return pwd
     */
    std::string getPassword(){
        return pwd;
    }

    /**
     * \~french \brief Modifie le pwd
     * \param[in] pwd
     * \~english \brief Set the pwd
     * \param[in] pwd
     */
    void setPassword (std::string u) {
        pwd = u;
    }

    /**
     * \~french \brief Récupère le referer
     * \return referer
     * \~english \brief Get the referer
     * \return referer
     */
    std::string getReferer(){
        return referer;
    }

    /**
     * \~french \brief Modifie le referer
     * \param[in] referer
     * \~english \brief Set the referer
     * \param[in] referer
     */
    void setReferer (std::string u) {
        referer = u;
    }

    /**
     * \~french \brief Récupère le userAgent
     * \return userAgent
     * \~english \brief Get the userAgent
     * \return userAgent
     */
    std::string getUserAgent(){
        return userAgent;
    }

    /**
     * \~french \brief Modifie le userAgent
     * \param[in] userAgent
     * \~english \brief Set the userAgent
     * \param[in] userAgent
     */
    void setUserAgent (std::string u) {
        userAgent = u;
    }

    /**
     * \~french \brief Récupère le timeout
     * \return timeout
     * \~english \brief Get the timeout
     * \return timeout
     */
    int getTimeOut(){
        return timeout;
    }

    /**
     * \~french \brief Modifie le timeout
     * \param[in] timeout
     * \~english \brief Set the timeout
     * \param[in] timeout
     */
    void setTimeOut (int u) {
        timeout = u;
    }

    /**
     * \~french \brief Récupère le retry
     * \return retry
     * \~english \brief Get the retry
     * \return retry
     */
    int getRetry(){
        return retry;
    }

    /**
     * \~french \brief Modifie le retry
     * \param[in] retry
     * \~english \brief Set the retry
     * \param[in] retry
     */
    void setRetry (int u) {
        retry = u;
    }

    /**
     * \~french \brief Récupère le interval
     * \return interval
     * \~english \brief Get the interval
     * \return interval
     */
    int getInterval(){
        return interval;
    }

    /**
     * \~french \brief Modifie le interval
     * \param[in] interval
     * \~english \brief Set the interval
     * \param[in] interval
     */
    void setInterval (int u) {
        interval = u;
    }

    /**
     * \~french \brief Récupère le responseType
     * \return interval
     * \~english \brief Get the responseType
     * \return interval
     */
    std::string getResponseType(){
        return responseType;
    }

    /**
     * \~french \brief Modifie le responseType
     * \param[in] interval
     * \~english \brief Set the responseType
     * \param[in] interval
     */
    void setResponseType (std::string u) {
        responseType = u;
    }

    /**
     * \~french
     * \brief Récupération des données à partir d'une URL
     * \~english
     * \brief Taking Data from an URL
     */
    RawDataSource * performRequest(std::string request);
    
    /**
     * \~french
     * \brief Récupération des données à partir d'une URL
     * \~english
     * \brief Taking Data from an URL
     */
    RawDataStream * performRequestStream(std::string request);


    /**
     * \~french \brief Constructeur
     * \~english \brief Constructor
     */
    WebService(std::string url, int retry, int interval, int timeout);
    
    /**
     * \~french \brief Constructeur à partir d'un autre
     * \~english \brief Constructor from another
     */
    WebService(WebService* obj);

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~WebService();

};

#endif // WEBSERVICE_H
