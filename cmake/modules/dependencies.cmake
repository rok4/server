if(ROK4SERVERDEPENDENCIES_FOUND)
    return()
endif(ROK4SERVERDEPENDENCIES_FOUND)

message("Search dependencies for ROK4 SERVER")

# Extern libraries, shared

if(NOT TARGET tinyxml)
    find_package(TinyXML)
    if(TINYXML_FOUND)
        add_library(tinyxml SHARED IMPORTED)
        set_property(TARGET tinyxml PROPERTY IMPORTED_LOCATION ${TINYXML_LIBRARY})
    else(TINYXML_FOUND)
        message(FATAL_ERROR   "Cannot find extern library tinyxml")
    endif(TINYXML_FOUND)
endif(NOT TARGET tinyxml)

if(NOT TARGET fcgi)
    find_package(Fcgi)
    if(FCGI_FOUND)
        add_library(fcgi SHARED IMPORTED)
        set_property(TARGET fcgi PROPERTY IMPORTED_LOCATION ${FCGI_LIBRARY})
    else(FCGI_FOUND)
        message(FATAL_ERROR "Cannot find extern library fcgi")
    endif(FCGI_FOUND)
endif(NOT TARGET fcgi)

if(NOT TARGET boostlog)
    find_package(BoostLog)
    if(BOOSTLOG_FOUND)
        add_library(boostlog SHARED IMPORTED)
        set_property(TARGET boostlog PROPERTY IMPORTED_LOCATION ${BOOSTLOG_LIBRARY})
        add_library(boostlogsetup SHARED IMPORTED)
        set_property(TARGET boostlogsetup PROPERTY IMPORTED_LOCATION ${BOOSTLOGSETUP_LIBRARY})
        add_library(boostthread SHARED IMPORTED)
        set_property(TARGET boostthread PROPERTY IMPORTED_LOCATION ${BOOSTTHREAD_LIBRARY})
        add_library(boostsystem SHARED IMPORTED)
        set_property(TARGET boostsystem PROPERTY IMPORTED_LOCATION ${BOOSTSYSTEM_LIBRARY})
        add_library(boostfilesystem SHARED IMPORTED)
        set_property(TARGET boostfilesystem PROPERTY IMPORTED_LOCATION ${BOOSTFILESYSTEM_LIBRARY})
        add_definitions(-DBOOST_LOG_DYN_LINK -DBOOST_SYSTEM_USE_UTF8)
    else(BOOSTLOG_FOUND)
        message(FATAL_ERROR "Cannot find extern library boostlog")
    endif(BOOSTLOG_FOUND)
endif(NOT TARGET boostlog)

if(NOT TARGET curl)
    find_package(Curl)
    if(CURL_FOUND)
        add_library(curl SHARED IMPORTED)
        set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CURL_LIBRARY})
    else(CURL_FOUND)
        message(FATAL_ERROR "Cannot find extern library libcurl")
    endif(CURL_FOUND)
endif(NOT TARGET curl)

if(NOT TARGET openssl)
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        add_library(openssl SHARED IMPORTED)
        set_property(TARGET openssl PROPERTY IMPORTED_LOCATION ${OPENSSL_LIBRARY})
        add_library(crypto SHARED IMPORTED)
        set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${CRYPTO_LIBRARY})
    else(OPENSSL_FOUND)
        message(FATAL_ERROR "Cannot find extern library openssl and crypto")
    endif(OPENSSL_FOUND)
endif(NOT TARGET openssl)

if(NOT TARGET proj)
    find_package(Proj)
    if(PROJ_FOUND)
        add_library(proj SHARED IMPORTED)
        set_property(TARGET proj PROPERTY IMPORTED_LOCATION ${PROJ_LIBRARY})
    else(PROJ_FOUND)
        message(FATAL_ERROR "Cannot find extern library proj")
    endif(PROJ_FOUND)
endif(NOT TARGET proj)

# Extern libraries, static

if(NOT TARGET thread)
    find_package(Threads REQUIRED)
    if(NOT CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR "Need the PThread library")
    endif(NOT CMAKE_USE_PTHREADS_INIT)
    add_library(thread STATIC IMPORTED)
    set_property(TARGET thread PROPERTY IMPORTED_LOCATION ${CMAKE_THREAD_LIBS_INIT})
endif(NOT TARGET thread)

set(ROK4SERVERDEPENDENCIES_FOUND TRUE BOOL)