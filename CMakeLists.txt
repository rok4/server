################### INITIALISATION

# check the cmake version
cmake_minimum_required(VERSION 3.10)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)

################### PARAMÈTRES

set(BUILD_VERSION "0.0.0" CACHE STRING "Build version")
set(CPACK_SYSTEM_NAME "ubuntu-20.04" CACHE STRING "Building OS, to deduce package format to generate")
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation location")
set(DEBUG_BUILD 0 CACHE BOOL "Debug mode compilation")
set(UNITTEST_ENABLED 1 CACHE BOOL "Unit tests compilation")
set(DOC_ENABLED 1 CACHE BOOL "Documentation compilation")

set(VERSION ${BUILD_VERSION})

message("Version ${VERSION}")

# Définit le projet 
project(rok4-server
        VERSION ${VERSION}
        DESCRIPTION "WMS/WMTS/TMS server"
        LANGUAGES CXX)

configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

################### MODE DEBUG

if(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE debugbuild)
  set(CMAKE_CXX_FLAGS_DEBUGBUILD "-g -O0")
  set(CMAKE_C_FLAGS_DEBUGBUILD "-g")
else(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE specificbuild)
  set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-O3")
endif(DEBUG_BUILD)

################### RECHERCHE DES DÉPENDANCES

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

include(cmake/modules/dependencies.cmake)

################### DÉFINITION DES FICHIERS SOURCES ET DES HEADERS

# Dossiers contenant les headers nécessaires
include_directories(${PROJECT_BINARY_DIR} ${ROK4_INCLUDE_DIR} ${BOOSTLOG_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${FCGI_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR})

message(STATUS "Building ROK4 Server")

file(GLOB ROK4SERVER_SRCS 
    "${PROJECT_SOURCE_DIR}/src/*.cpp" 
    "${PROJECT_SOURCE_DIR}/src/healthcheck/*.cpp"
)

add_executable(${PROJECT_NAME} ${ROK4SERVER_SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "rok4")

# Lien des librairies dépendantes
target_link_libraries(${PROJECT_NAME} PUBLIC rok4 fcgi tinyxml boostlog boostlogsetup boostthread boostfilesystem boostsystem curl openssl crypto proj)

################### TESTS UNITAIRES

if(UNITTEST_ENABLED)
    message("${CPPUNIT_INCLUDE_DIR}")
    include_directories(${CPPUNIT_INCLUDE_DIR})
    enable_testing()
    add_definitions(-DUNITTEST)
    file(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/CppUnit*.cpp" )
    add_executable(UnitTester-${PROJECT_NAME} tests/main.cpp ${UnitTests_SRCS} tests/TimedTestListener.cpp tests/XmlTimedTestOutputterHook.cpp )
    target_link_libraries(UnitTester-${PROJECT_NAME} cppunit)
    foreach(test ${UnitTests_SRCS})
        message("  - adding test ${test}")
        get_filename_component(TestName ${test} NAME_WE)
        add_test(${TestName} UnitTester-${PROJECT_NAME} ${TestName})
    endforeach(test)
endif(UNITTEST_ENABLED)

################### DOCUMENTATION

if(DOC_ENABLED)

    set(DOXYGEN_DOXYFILE_ENCODING UTF-8)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist)
    set(DOXYGEN_OUTPUT_LANGUAGE french)
    set(DOXYGEN_INPUT_ENCODING UTF-8)
    set(DOXYGEN_IMAGE_PATH docs/images)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE docs/main.doxygen.md)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_HAVE_DOT NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_HEADER docs/doxygen/header.html)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/doxygen-awesome.css)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
        
    doxygen_add_docs(doc ${CMAKE_CURRENT_SOURCE_DIR}/src/ ${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/)

endif(DOC_ENABLED)

################### INSTALLATION

# For access to standard installation directory variables (CMAKE_INSTALL_xDIR).
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Installe les fichiers de configurations
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/" DESTINATION etc/rok4)

# Installe le fichier de service
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4.service" 
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION lib/systemd/system
)

################### PACKAGE CONFIGURATION
message(STATUS "Configuring packages")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BUILD_ARCHITECTURE "amd64")
else()
    set(BUILD_ARCHITECTURE "i386")
endif()

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION}-${CPACK_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_VENDOR "IGN")
set(CPACK_PACKAGE_CONTACT "IGN - Géoportail (<tout_rdev@ign.fr>)")

if(CPACK_SYSTEM_NAME AND CPACK_SYSTEM_NAME STREQUAL "ubuntu-20.04" OR CPACK_SYSTEM_NAME STREQUAL "ubuntu-22.04")
    set(CPACK_GENERATOR "DEB;TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "librok4-dev, libfcgi-dev ,libtinyxml-dev ,libcurl4-openssl-dev ,libproj-dev ,libssl-dev ,libboost-log-dev ,libboost-filesystem-dev ,libboost-system-dev")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/cmake/debian/postinst;" )
    set(CPACK_BINARY_DEB "ON")
endif()

if(CPACK_SYSTEM_NAME AND CPACK_SYSTEM_NAME STREQUAL "alpine3")
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

