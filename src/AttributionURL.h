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
 * \file AttributionURL.h
 * \~french
 * \brief Définition de la classe AttributionURL gérant les liens vers les métadonnées dans les documents de capacités
 * \~english
 * \brief Define the AttributionURL Class handling capabilities metadata link elements
 */

class AttributionURL;

#ifndef ATTRIBUTIONURL_H
#define ATTRIBUTIONURL_H

#include "utils/ResourceLocator.h"
#include "UtilsXML.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance AttributionURL représente un lien vers une attribution
 * \brief Gestion des éléments d'attribution des documents de capacités
 * \~english
 * A AttributionURL represent a attribution link element in the differents capabilities documents.
 * \brief Attribution handler for the capabilities documents
 */
class AttributionURL : public ResourceLocator {
private:
    /**
     * \~french \brief Titre de l'attribution
     * \~english \brief Attribution title
     */
    std::string title;
    /**
     * \~french \brief Largeur pixel du logo
     * \~english \brief Pixel width of logo
     */
    int width;
    /**
     * \~french \brief Hauteur pixel du logo
     * \~english \brief Pixel hieght of logo
     */
    int height;

    /**
     * \~french \brief Resource du logo
     * \~english \brief Logo resource
     */
    ResourceLocator* logo;

public:
    /**
     * \~french
     * \brief Crée un AttributionURL à partir d'un élément JSON
     * \param[in] doc Élément JSON
     * \~english
     * \brief Create a AttributionURL from JSON element
     * \param[in] doc JSON element
     */
    AttributionURL ( json11::Json doc ) : ResourceLocator (), logo(NULL) {

        if (! doc["title"].is_string()) {
            missingField = "title";
            return;
        }
        title = doc["title"].string_value();

        if (! doc["url"].is_string()) {
            missingField = "url";
            return;
        }
        href = doc["url"].string_value();

        if (doc["logo"].is_object()) {
            if (! doc["logo"]["width"].is_number()) {
                missingField = "logo.width";
                return;
            }
            width = doc["logo"]["width"].number_value();
            if (! doc["logo"]["height"].is_number()) {
                missingField = "logo.height";
                return;
            }
            height = doc["logo"]["height"].number_value();

            if (! doc["logo"]["format"].is_string()) {
                missingField = "logo.format";
                return;
            }
            if (! doc["logo"]["url"].is_string()) {
                missingField = "logo.url";
                return;
            }
            logo = new ResourceLocator(doc["logo"]["format"].string_value(), doc["logo"]["url"].string_value());
        }
    };

    /**
     * \~french \brief Export XML pour le GetCapabilities TMS
     * \param[in] elName Nom de l'élément XML
     * \~english \brief XML export for TMS GetCapabilities
     * \param[in] elName XML element name
     */
    std::string getTmsXml() {
        std::string res = "<Attribution><Title>" + title + "</Title>";
        if (logo != NULL) {
            res += "<Logo width=\"" + std::to_string(width) + "\" height=\"" + std::to_string(height) + "\" href=\"" + logo->getHRef() + "\" mime-type=\"" + logo->getFormat() + "\" />\n";
        }
        res += "</Attribution>";
        return res;
    }

    /**
     * \~french \brief Export XML pour le GetCapabilities WMS
     * \~english \brief XML export for WMS GetCapabilities
     */
    TiXmlElement* getWmsXml() {
        TiXmlElement* el = new TiXmlElement ( "Attribution" );
        el->LinkEndChild ( UtilsXML::buildTextNode ( "Title", title ) );

        TiXmlElement* orEl = new TiXmlElement ( "OnlineResource" );
        orEl->SetAttribute ( "xlink:type","simple" );
        orEl->SetAttribute ( "xlink:href", href );
        el->LinkEndChild ( orEl );

        if (logo != NULL) {
            TiXmlElement* logoEl = new TiXmlElement ( "LogoURL" );
            logoEl->SetAttribute ( "width", width );
            logoEl->SetAttribute ( "height", height );
            logoEl->LinkEndChild ( UtilsXML::buildTextNode ( "Format", logo->getFormat() ) );

            TiXmlElement* logoOrEl = new TiXmlElement ( "OnlineResource" );
            logoOrEl->SetAttribute ( "xlink:type","simple" );
            logoOrEl->SetAttribute ( "xlink:href", logo->getHRef() );
            logoEl->LinkEndChild ( logoOrEl );

            el->LinkEndChild ( logoEl );
        }
        
        return el;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~AttributionURL() {
        if (logo != NULL) delete logo;
    };
};

#endif // ATTRIBUTIONURL_H
