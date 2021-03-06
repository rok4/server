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
 * \file Message.cpp
 * \~french
 * \brief Implémentation des classes gérant les messages utilisateurs
 * \~english
 * \brief Implement classes handling user messages
 */

#include "Message.h"
#include <boost/log/trivial.hpp>
#include <iostream>

inline std::string tagName ( std::string t ) {
    return ( t=="wms"?"ServiceExceptionReport":"ExceptionReport" ) ;
}

inline std::string xmlnsUri ( std::string t ) {
    return ( t=="wms"?"http://www.opengis.net/ogc":"http://www.opengis.net/ows/1.1" ) ;
}

/**
 * Methode commune pour generer un Rapport d'exception.
 * @param sex une exception de service
 */
std::string genSER ( ServiceException *sex ) {

    BOOST_LOG_TRIVIAL(debug) <<  "service=["<<sex->getService() <<"]"  ;
    std::string msg = "";

    if (sex->getFormat() == "application/json") {
        msg = sex->toString() ;
    } else {
        msg+= "<"+tagName ( sex->getService() ) +" xmlns=\""+xmlnsUri ( sex->getService() ) +"\">\n" ;
        msg+= sex->toString() ;
        msg+= "</"+tagName ( sex->getService() ) +">" ;
    }

    BOOST_LOG_TRIVIAL(debug) <<  "SERVICE EXCEPTION : " << msg ;

    return msg ;
}

SERDataSource::SERDataSource ( ServiceException *sex ) : MessageDataSource ( "",sex->getFormat() ) {
    this->message= genSER ( sex ) ;
    this->httpStatus= ServiceException::getCodeAsStatusCode ( sex->getCode() ) ;
    delete sex;
}

SERDataStream::SERDataStream ( ServiceException *sex ) : MessageDataStream ( "", sex->getFormat() ) {
    this->message= genSER ( sex ) ;
    this->httpStatus= ServiceException::getCodeAsStatusCode ( sex->getCode() ) ;
    delete sex;
}
