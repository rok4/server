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

#pragma once

#include <boost/algorithm/string/join.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::write_xml;
using boost::property_tree::xml_writer_settings;

#include <iomanip>
#include <string>
#include <vector>

#include <rok4/utils/BoundingBox.h>
#include <rok4/utils/Keyword.h>
#include <rok4/utils/TileMatrixLimits.h>
#include <rok4/thirdparty/json11.hpp>

#include "DataStreams.h"

namespace Utils {
    /**
     * \~french
     * \brief Donne le nombre de chiffres après la virgule
     * \details 3.14 -> 2, 1.0001 -> 4, le maximum est 10
     * \param[in] arg un double
     * \return int valeur
     * \~english
     * \brief Give the number of decimal places
     * \details 3.14 -> 2, 1.0001 -> 4, maximum is 10
     * \param[in] arg a double
     * \return int value
     */
    static int get_decimal_places(double value) {
        value = fmod(value, 1);
        static const int MAX_DP = 10;
        double THRES = pow(0.1, MAX_DP);
        if (value == 0.0)
            return 0;
        int nDecimal = 0;
        while (value - floor(value) > THRES && nDecimal < MAX_DP && ceil(value) - value > THRES) {
            value *= 10.0;
            THRES *= 10.0;
            nDecimal++;
        }
        return nDecimal;
    }

    /**
     * \~french Conversion d'un entier en une chaîne de caractère
     * \~english Convert an integer in a character string
     */
    static std::string int_to_string(long int i) {
        std::ostringstream strstr;
        strstr << i;
        return strstr.str();
    }

    /**
     * \~french Conversion d'un flottant en une chaîne de caractères
     * \~english Convert a float in a character string
     */
    static std::string double_to_string(long double d) {
        std::ostringstream strstr;
        strstr.setf(std::ios::fixed, std::ios::floatfield);
        strstr.precision(16);
        strstr << d;
        return strstr.str();
    }

    /**
     * \~french
     * \brief Convertit un caractère héxadécimal (0-9, A-Z, a-z) en décimal
     * \param[in] hex caractère
     * \return 0xFF sur une entrée invalide
     * \~english
     * \brief Converts hex char (0-9, A-Z, a-z) to decimal.
     * \param[in] hex character
     * \return 0xFF on invalid input.
     */
    static char hexadecimal_to_int(unsigned char hex) {
        hex = hex - '0';
        // Si hex <= 9 on a le résultat
        //   Sinon
        if (hex > 9) {
            hex = (hex + '0' - 1) | 0x20;  // Pour le passage des majuscules aux minuscules dans la table ASCII
            hex = hex - 'a' + 11;
        }
        if (hex > 15)  // En cas d'erreur
            hex = 0xFF;

        return hex;
    }

    static std::map<std::string, std::string> string_to_map(std::string s, std::string item_separator, std::string kv_separator) {
        size_t pos_item = 0;
        std::map<std::string, std::string> res;

        std::string item;
        while (true) {
            pos_item = s.find(item_separator);
            item = s.substr(0, pos_item);
            s.erase(0, pos_item + item_separator.length());

            size_t pos_kv = item.find(kv_separator);
            if (pos_kv != std::string::npos) {
                res.insert(std::pair<std::string, std::string>(item.substr(0, pos_kv), item.substr(pos_kv + kv_separator.length(), std::string::npos)));
            }

            if (pos_item == std::string::npos) {
                break;
            }
        }

        return res;
    }

    static std::string map_to_string(std::map<std::string, std::string> m, std::string item_separator, std::string kv_separator) {
        std::vector<std::string> tmp;

        for (auto const& i : m) {
            tmp.push_back(i.first + kv_separator + i.second);
        }

        return boost::algorithm::join(tmp, "&");
    }

    static MessageDataStream* format_get_feature_info(std::vector<std::string> data, std::string format) {
        if (format.compare("text/html") == 0) {
            ptree tree;

            ptree& root = tree.add("html", "");
            root.add("body.p.b", "Pixel value :");
            ptree& list = root.add("ul", "");
            for (std::string v : data) {
                list.add("li", v);
            }

            std::stringstream ss;
            write_xml(ss, tree);
            return new MessageDataStream(ss.str(), "text/html", 200);
        } else if (format.compare("text/xml") == 0) {
            ptree tree;

            ptree& root = tree.add("Pixel", "");
            for (std::string v : data) {
                root.add("Band", v);
            }

            std::stringstream ss;
            write_xml(ss, tree);
            return new MessageDataStream(ss.str(), "text/xml", 200);
        } else if (format.compare("application/json") == 0) {
            json11::Json res = json11::Json::object{
                {"type", "FeatureCollection"},
                {"features", json11::Json::array { 
                    json11::Json::object {
                        {"type", "Feature"},
                        {"properties", json11::Json::object {
                            {"pixel", data},
                        } },
                    }
                } },
            };
            return new MessageDataStream(res.dump(), "application/json", 200);
        } else {
            return new MessageDataStream(boost::algorithm::join(data, ","), "text/plain", 200);
        }
    }



    /**
     * \~french
     * \brief Décodage de l'URL correspondant à la requête
     * \param[in,out] src URL
     * \~english
     * \brief URL decoding
     * \param[in,out] src URL
     */
    static void url_decode(char* src) {
        unsigned char high, low;
        char* dst = src;

        while ((*src) != '\0') {
            if (*src == '+') {
                *dst = ' ';
            } else if (*src == '%') {
                *dst = '%';

                high = Utils::hexadecimal_to_int(*(src + 1));
                if (high != 0xFF) {
                    low = Utils::hexadecimal_to_int(*(src + 2));
                    if (low != 0xFF) {
                        high = (high << 4) | low;

                        /* map control-characters out */
                        if (high < 32 || high == 127) high = '_';

                        *dst = high;
                        src += 2;
                    }
                }
            } else {
                *dst = *src;
            }

            dst++;
            src++;
        }

        *dst = '\0';
    }
};


