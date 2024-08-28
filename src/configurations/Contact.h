/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geoservices@ign.fr>
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
 * \file h
 * \~french
 * \brief Définition de la classe Contact regroupant les informations de contact des services.
 * \~english
 * \brief Define the Contact class handling contact informations for services.
 */


#ifndef CONTACT_H_
#define CONTACT_H_

#include <boost/property_tree/ptree.hpp>
using boost::property_tree::ptree;

#include <rok4/utils/Configuration.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Contact regroupe les informations de contact des services
 * \brief Gestion des informations de contact
 * \~english
 * A Contact represent services conact informations
 */
class Contact : public Configuration {

private:
    /**
     * \~french \brief Identifiant de la couche
     * \~english \brief Layer identifier
     */
    std::string individual_name;
    std::string individual_position;
    std::string voice;
    std::string facsimile;
    std::string address_type;
    std::string delivery_point;
    std::string city;
    std::string administrative_area;
    std::string post_code;
    std::string country;
    std::string email;

public:
    /**
    * \~french
    * Crée un Contact à partir d'une section JSON
    * \brief Constructeur
    * \param[in] doc Objet JSON
    * \~english
    * Create a Contact from a JSON section
    * \brief Constructor
    * \param[in] doc JSON object
    */
    Contact(json11::Json& doc) {
        if (doc.is_null()) {
            error_message = "No contact section";
            return;
        } else if (! doc.is_object()) {
            error_message = "Contact configuration: contact have to be an object";
            return;
        }

        if (doc["name"].is_string()) {
            individual_name = doc["name"].string_value();
        } else if (! doc["name"].is_null()) {
            error_message = "Contact configuration: name have to be a string";
            return;
        }
        if (doc["position"].is_string()) {
            individual_position = doc["position"].string_value();
        } else if (! doc["position"].is_null()) {
            error_message = "Contact configuration: position have to be a string";
            return;
        }
        if (doc["voice"].is_string()) {
            voice = doc["voice"].string_value();
        } else if (! doc["voice"].is_null()) {
            error_message = "Contact configuration: voice have to be a string";
            return;
        }
        if (doc["facsimile"].is_string()) {
            facsimile = doc["facsimile"].string_value();
        } else if (! doc["facsimile"].is_null()) {
            error_message = "Contact configuration: facsimile have to be a string";
            return;
        }
        if (doc["address_type"].is_string()) {
            address_type = doc["address_type"].string_value();
        } else if (! doc["address_type"].is_null()) {
            error_message = "Contact configuration: address_type have to be a string";
            return;
        }
        if (doc["delivery_point"].is_string()) {
            delivery_point = doc["delivery_point"].string_value();
        } else if (! doc["delivery_point"].is_null()) {
            error_message = "Contact configuration: delivery_point have to be a string";
            return;
        }
        if (doc["city"].is_string()) {
            city = doc["city"].string_value();
        } else if (! doc["city"].is_null()) {
            error_message = "Contact configuration: city have to be a string";
            return;
        }
        if (doc["administrative_area"].is_string()) {
            administrative_area = doc["administrative_area"].string_value();
        } else if (! doc["administrative_area"].is_null()) {
            error_message = "Contact configuration: administrative_area have to be a string";
            return;
        }
        if (doc["post_code"].is_string()) {
            post_code = doc["post_code"].string_value();
        } else if (! doc["post_code"].is_null()) {
            error_message = "Contact configuration: post_code have to be a string";
            return;
        }
        if (doc["country"].is_string()) {
            country = doc["country"].string_value();
        } else if (! doc["country"].is_null()) {
            error_message = "Contact configuration: country have to be a string";
            return;
        }
        if (doc["email"].is_string()) {
            email = doc["email"].string_value();
        } else if (! doc["email"].is_null()) {
            error_message = "Contact configuration: email have to be a string";
            return;
        }
    }

    /**
     * \~french \brief Ajoute un noeud WMTS correpondant au contact
     * \param[in] parent Noeud auquel ajouter celui du contact
     * \~english \brief Add a WMTS node corresponding to contact
     * \param[in] parent Node to whom add the contact node
     */
    void add_node_wmts(ptree& parent) {
        ptree& node = parent.add("ows:ServiceContact", "");
        node.add("ows:IndividualName", individual_name);
        node.add("ows:PositionName", individual_position);

        node.add("ows:ContactInfo.ows:Phone.ows:Voice", voice);
        node.add("ows:ContactInfo.ows:Phone.ows:Facsimile", facsimile);
        
        node.add("ows:ContactInfo.ows:Address.ows:DeliveryPoint", delivery_point);
        node.add("ows:ContactInfo.ows:Address.ows:City", city);
        node.add("ows:ContactInfo.ows:Address.ows:AdministrativeArea", administrative_area);
        node.add("ows:ContactInfo.ows:Address.ows:PostalCode", post_code);
        node.add("ows:ContactInfo.ows:Address.ows:Country", country);
        node.add("ows:ContactInfo.ows:Address.ows:ElectronicMailAddress", email);
    }

    /**
     * \~french \brief Ajoute un noeud WMS correpondant au contact
     * \param[in] parent Noeud auquel ajouter celui du contact
     * \~english \brief Add a WMS node corresponding to contact
     * \param[in] parent Node to whom add the contact node
     */
    void add_node_wms(ptree& parent, std::string organization) {
        ptree& node = parent.add("ContactInformation", "");
        node.add("ContactPersonPrimary.ContactPerson", individual_name);
        node.add("ContactPersonPrimary.ContactOrganization", organization);
        node.add("ContactPosition", individual_position);

        node.add("ContactAddress.AddressType", address_type);
        node.add("ContactAddress.Address", delivery_point);
        node.add("ContactAddress.City", city);
        node.add("ContactAddress.StateOrProvince", administrative_area);
        node.add("ContactAddress.PostCode", post_code);
        node.add("ContactAddress.Country", country);

        node.add("ContactVoiceTelephone", voice);
        node.add("ContactFacsimileTelephone", facsimile);
        node.add("ContactElectronicMailAddress", email);

    }

    /**
     * \~french \brief Ajoute un noeud TMS correpondant au contact
     * \param[in] parent Noeud auquel ajouter celui du contact
     * \~english \brief Add a TMS node corresponding to contact
     * \param[in] parent Node to whom add the contact node
     */
    void add_node_tms(ptree& parent, std::string organization) {
        ptree& node = parent.add("ContactInformation", "");
        node.add("ContactPersonPrimary.ContactPerson", individual_name);
        node.add("ContactPersonPrimary.ContactOrganization", organization);
        node.add("ContactPosition", individual_position);

        node.add("ContactAddress.AddressType", address_type);
        node.add("ContactAddress.Address", delivery_point);
        node.add("ContactAddress.City", city);
        node.add("ContactAddress.StateOrProvince", administrative_area);
        node.add("ContactAddress.PostCode", post_code);
        node.add("ContactAddress.Country", country);

        node.add("ContactVoiceTelephone", voice);
        node.add("ContactFacsimileTelephone", facsimile);
        node.add("ContactElectronicMailAddress", email);

    }

};

#endif /* CONTACT_H_ */
