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

class Attribute;

#ifndef UTILSXML
#define UTILSXML

#include <vector>
#include <string>
#include <tinyxml.h>

#include <rok4/utils/Keyword.h>
#include <rok4/utils/TileMatrixLimits.h>

class UtilsXML
{

    public:

        /**
         * \~french Construit un noeud xml simple (de type text)
         * \~english Create a simple XML text node
         */
        static TiXmlElement* buildTextNode ( std::string elementName, std::string value ) {
            TiXmlElement * elem = new TiXmlElement ( elementName );
            TiXmlText * text = new TiXmlText ( value );
            elem->LinkEndChild ( text );
            return elem;
        }

        /**
         * \~french Récupère le texte d'un noeud simple
         * \~english Get text form simple XML node
         */
        static const std::string getTextStrFromElem(TiXmlElement* pElem) {
            const TiXmlNode* child = pElem->FirstChild();
            if ( child ) {
                const TiXmlText* childText = child->ToText();
                if ( childText ) {
                    return childText->ValueStr();
                }
            }
            return "";
        }


        /**
         * \~french \brief Export XML du mot clé pour le GetCapabilities
         * \param[in] elName Nom de l'élément XML
         * \~english \brief Keyword XML export for GetCapabilities
         * \param[in] elName XML element name
         */
        static TiXmlElement* getXml(std::string elName, Keyword k) {
            TiXmlElement* el = new TiXmlElement ( elName );
            el->LinkEndChild ( new TiXmlText ( k.getContent() ) );
            
            for ( std::map<std::string,std::string>::const_iterator it = k.getAttributes()->begin(); it != k.getAttributes()->end(); it++ ) {
                el->SetAttribute ( ( *it ).first, ( *it ).second );
            }
            
            return el;
        }


        /**
         * \~french \brief Export XML des tuiles limites pour un niveau
         * \param[in] tml Tuiles limites du niveau
         * \~english \brief XML export for tiles' limits for a level
         * \param[in] tml Level's tiles limits
         */
        static TiXmlElement* getXml(TileMatrixLimits tml) {
            TiXmlElement* tmLimitsEl = new TiXmlElement ( "TileMatrixLimits" );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "TileMatrix", tml.tileMatrixId ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MinTileRow", std::to_string(tml.minTileRow) ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MaxTileRow", std::to_string(tml.maxTileRow) ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MinTileCol", std::to_string(tml.minTileCol) ) );
            tmLimitsEl->LinkEndChild ( buildTextNode ( "MaxTileCol", std::to_string(tml.maxTileCol) ) );
            
            return tmLimitsEl;
        }

};

#endif // ATTRIBUTE_H

