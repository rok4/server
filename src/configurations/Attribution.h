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
 * \file configurations/Attribution.h
 * \~french
 * \brief Définition de la classe Attribution gérant les liens vers les métadonnées dans les documents de capacités
 * \~english
 * \brief Define the Attribution Class handling capabilities metadata link elements
 */

#pragma once

#include <rok4/utils/ResourceLocator.h>

#include "Utils.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Attribution représente un lien vers une attribution
 * \brief Gestion des éléments d'attribution des documents de capacités
 * \~english
 * A Attribution represent a attribution link element in the differents capabilities documents.
 * \brief Attribution handler for the capabilities documents
 */
class Attribution : public ResourceLocator {
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
     * \brief Crée un Attribution à partir d'un élément JSON
     * \param[in] doc Élément JSON
     * \~english
     * \brief Create a Attribution from JSON element
     * \param[in] doc JSON element
     */
    Attribution ( json11::Json doc ) : ResourceLocator (), logo(NULL) {

        if (! doc["title"].is_string()) {
            missing_field = "title";
            return;
        }
        title = doc["title"].string_value();

        if (! doc["url"].is_string()) {
            missing_field = "url";
            return;
        }
        href = doc["url"].string_value();

        if (doc["logo"].is_object()) {
            if (! doc["logo"]["width"].is_number()) {
                missing_field = "logo.width";
                return;
            }
            width = doc["logo"]["width"].number_value();
            if (! doc["logo"]["height"].is_number()) {
                missing_field = "logo.height";
                return;
            }
            height = doc["logo"]["height"].number_value();

            if (! doc["logo"]["format"].is_string()) {
                missing_field = "logo.format";
                return;
            }
            if (! doc["logo"]["url"].is_string()) {
                missing_field = "logo.url";
                return;
            }
            logo = new ResourceLocator(doc["logo"]["format"].string_value(), doc["logo"]["url"].string_value());
        }
    };

    /**
     * \~french \brief Ajoute un noeud TMS correpondant à l'attribution
     * \param[in] parent Noeud auquel ajouter celui de l'attribution
     * \~english \brief Add a TMS node corresponding to attribution
     * \param[in] parent Node to whom add the attribution node
     */
    void add_node_tms(ptree& parent) {
        ptree& node = parent.add("Attribution", "");
        node.add("<xmlattr>.href", href);
        node.add("Title", title);

        if (logo != NULL) {
            node.add("Logo.<xmlattr>.width", width);
            node.add("Logo.<xmlattr>.height", height);
            node.add("Logo.<xmlattr>.href", logo->get_href());
            node.add("Logo.<xmlattr>.mime-type", logo->get_format());
        }
    }

    /**
     * \~french \brief Ajoute un noeud WMS correpondant à l'attribution
     * \param[in] parent Noeud auquel ajouter celui de l'attribution
     * \~english \brief Add a WMS node corresponding to attribution
     * \param[in] parent Node to whom add the attribution node
     */
    void add_node_wms(ptree& parent) {
        ptree& node = parent.add("Attribution", "");
        node.add("Title", title);
        node.add("OnlineResource.<xmlattr>.xlink:href", href);
        node.add("OnlineResource.<xmlattr>.xlink:type", "simple");

        if (logo != NULL) {
            node.add("LogoURL.<xmlattr>.width", width);
            node.add("LogoURL.<xmlattr>.height", height);
            node.add("LogoURL.Format", logo->get_format());
            node.add("LogoURL.OnlineResource.<xmlattr>.xlink:href", logo->get_href());
            node.add("LogoURL.OnlineResource.<xmlattr>.xlink:type", "simple");
        }
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Attribution() {
        if (logo != NULL) delete logo;
    };
};


