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
 * \file Attribute.h
 * \~french
 * \brief Définition de la classe Attribute gérant les attributs de couches de tuiles vecteur
 * \~english
 * \brief Define the Attribute class handling vector tiles' layers' attributes
 */

class Attribute;

#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <boost/algorithm/string/join.hpp>
#include <vector>
#include <string>

/**
 * \author Institut national de l'information géographique et forestière
 */
class Attribute
{

    public:
        Attribute(json11::Json doc) {

            missingField = "";
            values = std::vector<std::string>();
            min = "";
            max = "";
            metadataJson = "";

            if (! doc["name"].is_string()) {
                missingField = "name";
                return;
            }
            name = doc["name"].string_value();

            if (! doc["type"].is_string()) {
                missingField = "type";
                return;
            }
            type = doc["type"].string_value();

            if (! doc["count"].is_number()) {
                missingField = "count";
                return;
            }
            count = std::to_string(doc["count"].number_value());

            if (doc["min"].is_number()) {
                min = std::to_string(doc["min"].number_value());
            }
            if (doc["max"].is_number()) {
                max = std::to_string(doc["max"].number_value());
            }
            if (doc["values"].is_array()) {
                for (json11::Json v : doc["values"].array_items()) {
                    values.push_back(v.string_value());
                }
            }

        };
        ~Attribute(){};

        std::string getMissingField() {return missingField;}
        std::string getName() {return name;}
        std::string getType() {return type;}
        std::vector<std::string> getValues() {return values;}
        std::string getCount() {return count;}
        std::string getMin() {return min;}
        std::string getMax() {return max;}

        std::string getMetadataJson() {
            if (metadataJson != "") return metadataJson;
            /*
            {
                "attribute": "gid",
                "count": 1,
                "max": 49,
                "min": 49,
                "type": "number",
                "values": [
                    49
                ]
            }
            */

            std::ostringstream res;
            res << "{\"attribute\":\"" << name << "\"";
            res << ",\"count\":" << count << "";
            res << ",\"type\":\"" << type << "\"";

            if (min != "") {
                res << ",\"min\":" << min;
            }
            if (max != "") {
                res << ",\"max\":" << max;
            }

            if (values.size() != 0) {
                res << ",\"values\":[\"" << boost::algorithm::join(values, "\",\"") << "\"]";
            }

            res << "}";

            metadataJson = res.str();
            return metadataJson;
        }

    private:
        /**
         * \~french \brief Éventuel attribut manquant lors de la construction
         * \~english \brief Constructor missing field
         */
        std::string missingField;

        /**
         * \~french \brief Nom de l'attribut
         * \~english \brief Attribute's name
         */
        std::string name;
        /**
         * \~french \brief Type de l'attribut
         * \~english \brief Attribute's type
         */
        std::string type;
        /**
         * \~french \brief Valeurs prises par l'attribut
         * \~english \brief Attribute's distinct values
         */
        std::vector<std::string> values;
        /**
         * \~french \brief Nombre de valeurs distinctes de l'attribut
         * \~english \brief Attribute's distinct values count
         */
        std::string count;
        /**
         * \~french \brief Valeur minimale prise par l'attribut si numérique
         * \~english \brief Min value of attribute if number
         */
        std::string min;
        /**
         * \~french \brief Valeur maximale prise par l'attribut si numérique
         * \~english \brief Max value of attribute if number
         */
        std::string max;
        /**
         * \~french \brief Formattage JSON des informations
         * \~english \brief Informations JSON string
         */
        std::string metadataJson;
};

#endif // ATTRIBUTE_H

