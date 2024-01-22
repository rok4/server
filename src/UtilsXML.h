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
#include <iomanip>

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
        static int GetDecimalPlaces ( double dbVal ) {
            dbVal = fmod(dbVal, 1);
            static const int MAX_DP = 10;
            double THRES = pow ( 0.1, MAX_DP );
            if ( dbVal == 0.0 )
                return 0;
            int nDecimal = 0;
            while ( dbVal - floor ( dbVal ) > THRES && nDecimal < MAX_DP && ceil(dbVal)-dbVal > THRES) {
                dbVal *= 10.0;
                THRES *= 10.0;
                nDecimal++;
            }
            return nDecimal;
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


        /**
         * \~french \brief Export XML d'une bbox, en géographique ou dans le CRS fourni
         * \details La bbox fournie doit être en coordonnées géographique ou dans le CRS fourni
         * \param[in] bbox Bounding box
         * \param[in] crs CRS spécifique
         * \~english \brief XML export for bounding box, geographical or with provided CRS
         * \details Provided bbox have to be geographical or with the provided CRS
         * \param[in] bbox Bounding box
         * \param[in] crs Specific CRS
         */
        static TiXmlElement* getXml(BoundingBox<double> bbox, std::string crs = "") {

            std::ostringstream os;

            if (crs == "") {
                TiXmlElement * el = new TiXmlElement ( "EX_GeographicBoundingBox" );

                os.str ( "" );
                os<<bbox.xmin;
                el->LinkEndChild ( UtilsXML::buildTextNode ( "westBoundLongitude", os.str() ) );
                os.str ( "" );
                os<<bbox.xmax;
                el->LinkEndChild ( UtilsXML::buildTextNode ( "eastBoundLongitude", os.str() ) );
                os.str ( "" );
                os<<bbox.ymin;
                el->LinkEndChild ( UtilsXML::buildTextNode ( "southBoundLatitude", os.str() ) );
                os.str ( "" );
                os<<bbox.ymax;
                el->LinkEndChild ( UtilsXML::buildTextNode ( "northBoundLatitude", os.str() ) );
                os.str ( "" );

                return el;
            } else {

                TiXmlElement * el = new TiXmlElement ( "BoundingBox" );

                el->SetAttribute ( "CRS", crs );
                int floatprecision = GetDecimalPlaces ( bbox.xmin );
                floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.xmax ) );
                floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymin ) );
                floatprecision = std::max ( floatprecision,GetDecimalPlaces ( bbox.ymax ) );
                floatprecision = std::min ( floatprecision,9 ); //FIXME gestion du nombre maximal de décimal.

                os.str ( "" );
                os<< std::fixed << std::setprecision ( floatprecision );
                os<<bbox.xmin;
                el->SetAttribute ( "minx",os.str() );
                os.str ( "" );
                os<<bbox.ymin;
                el->SetAttribute ( "miny",os.str() );
                os.str ( "" );
                os<<bbox.xmax;
                el->SetAttribute ( "maxx",os.str() );
                os.str ( "" );
                os<<bbox.ymax;
                el->SetAttribute ( "maxy",os.str() );
                os.str ( "" );

                return el;
            }

        }

};

#endif // ATTRIBUTE_H

