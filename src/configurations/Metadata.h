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
 * \file configurations/Metadata.h
 * \~french
 * \brief Définition de la classe Metadata gérant les liens vers les métadonnées dans les documents de capacités
 * \~english
 * \brief Define the Metadata Class handling capabilities metadata link elements
 */

class Metadata;

#ifndef METADATA_H
#define METADATA_H

#include <rok4/utils/ResourceLocator.h>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Metadata représente un lien vers des métadonnées dans les différents documents de capacités.
 * \brief Gestion des éléments de métadonnées des documents de capacités
 * \~english
 * A Metadata represent a metadata link element in the differents capabilities documents.
 * \brief Metadata handler for the capabilities documents
 */
class Metadata : public ResourceLocator {
private:
    /**
     * \~french \brief Type de la métadonnée
     * \~english \brief Metadata type
     */
    std::string type;

public:
    /**
     * \~french
     * \brief Crée un Metadata à partir d'un élément JSON
     * \param[in] doc Élément JSON
     * \~english
     * \brief Create a Metadata from JSON element
     * \param[in] doc JSON element
     */
    Metadata ( json11::Json doc ) : ResourceLocator () {

        if (! doc["format"].is_string()) {
            missing_field = "format";
            return;
        }
        format = doc["format"].string_value();

        if (! doc["url"].is_string()) {
            missing_field = "url";
            return;
        }
        href = doc["url"].string_value();
        
        if (! doc["type"].is_string()) {
            missing_field = "type";
            return;
        }
        type = doc["type"].string_value();
    };

    /**
     * \~french
     * Crée un Metadata à partir d'un autre
     * \brief Constructeur de copie
     * \param[in] origMtdUrl Metadata à copier
     * \~english
     * Create a Metadata from another
     * \brief Copy Constructor
     * \param[in] origMtdUrl Metadata to copy
     */
    Metadata ( const Metadata & origMtdUrl ) : ResourceLocator ( origMtdUrl ) {
        href = origMtdUrl.href;
        format = origMtdUrl.format;
        type = origMtdUrl.type;
    };
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    Metadata& operator= ( Metadata const& other ) {
        if ( this != &other ) {
            ResourceLocator::operator= ( other );
            this->type = other.type;
        }
        return *this;
    };
    /**
     * \~french
     * \brief Test d'egalite de 2 MetadataURLs
     * \return true si tous les attributs sont identiques, false sinon
     * \~english
     * \brief Test whether 2 MetadataURLs are equals
     * \return true if all their attributes are identical
     */
    bool operator== ( const Metadata& other ) const {
        return ( this->type.compare ( other.type ) == 0
                && this->get_format().compare ( other.get_format() ) == 0
                && this->get_href().compare ( other.get_href() ) == 0 );
    };
    /**
     * \~french
     * \brief Test d'inégalite de 2 MetadataURLs
     * \return true s'ils ont un attribut différent, false sinon
     * \~english
     * \brief Test whether 2 MetadataURLs are different
     * \return true if one of their attributes is different
     */
    bool operator!= ( const Metadata& other ) const {
        return ! ( *this == other );
    };

    /**
     * \~french
     * \brief Retourne le type de la méta donnée
     * \return type
     * \~english
     * \brief Return the metadata type
     * \return type
     */
    inline std::string get_type() {
        return type;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Metadata() {};


    /**
     * \~french \brief Ajoute un noeud TMS correpondant à la métadonnée
     * \param[in] parent Noeud auquel ajouter celui de la métadonnée
     * \~english \brief Add a TMS node corresponding to metadata
     * \param[in] parent Node to whom add the metadata node
     */
    void add_node_tms(ptree& parent) {
        ptree& node = parent.add("Metadata", "");
        node.add("<xmlattr>.type", type);
        node.add("<xmlattr>.mime-type", "text/xml");
        node.add("<xmlattr>.href", href);
    }
};

#endif // METADATA_H
