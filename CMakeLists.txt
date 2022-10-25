################### INITIALISATION

# check the cmake version
cmake_minimum_required(VERSION 3.10)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)

################### PARAMÈTRES

option(BUILD_VERSION "Build version" 0.0.0)
option(CPACK_SYSTEM_NAME "System name for packaging" "linux")
option(CMAKE_INSTALL_PREFIX "Installation location" "/usr/local")
option(OBJECT_ENABLED "Build with object storage" 0)
option(DEBUG_BUILD "Build with debug mode" 0)
option(UNITTEST_ENABLED "Unit tests compilation" 0)

################### VERSION
set(VERSION ${BUILD_VERSION})

message("Version ${VERSION}")

# Définit le projet 
project(rok4-server
        VERSION ${VERSION}
        DESCRIPTION "WMS/WMTS/TMS server"
        LANGUAGES CXX)

################### MODE DEBUG

if(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE debugbuild)
  set(CMAKE_CXX_FLAGS_DEBUGBUILD "-g -O0")
  set(CMAKE_C_FLAGS_DEBUGBUILD "-g")
else(DEBUG_BUILD)
  set(CMAKE_BUILD_TYPE specificbuild)
  set(CMAKE_CXX_FLAGS_SPECIFICBUILD "-O3")
endif(DEBUG_BUILD)

################### DÉPENDANCES

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
message(STATUS "Modules CMake : ${CMAKE_MODULE_PATH}")

include(cmake/modules/dependencies.cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

################### COMPILATION

# Configuration du fichier (injection de la version et de la compilation objet)
configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)

message(STATUS "Building ROK4 Core")
set(FILEIMAGE_ENABLED 0)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/core)

message(STATUS "Building ROK4 Server")

# Dossiers contenant les headers nécessaires
include_directories(lib/core/src)
include_directories(${PROJECT_BINARY_DIR} ${BOOSTLOG_INCLUDE_DIR} ${CURL_INCLUDE_DIR} ${FCGI_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${TINYXML_INCLUDE_DIR})

file(GLOB ROK4SERVER_SRCS "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/healthcheck/*.cpp")
add_executable(rok4 ${ROK4SERVER_SRCS})

# Lien des librairies dépendantes
target_link_libraries(rok4 PUBLIC rok4core fcgi tinyxml boostlogsetup boostthread boostfilesystem boostsystem curl openssl crypto proj)
target_link_libraries(rok4 PRIVATE Threads::Threads)

################### TESTS

if(UNITTEST_ENABLED)
    include_directories(${CPPUNIT_INCLUDE_DIR})
    enable_testing()
    file(GLOB UnitTests_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/CppUnit*.cpp" )
    add_executable(UnitTester-rok4server tests/main.cpp ${UnitTests_SRCS} tests/TimedTestListener.cpp tests/XmlTimedTestOutputterHook.cpp )
    target_link_libraries(UnitTester-rok4server cppunit)
    foreach(test ${UnitTests_SRCS})
            message("  - adding test ${test}")
            get_filename_component(TestName ${test} NAME_WE)
            add_test(${TestName} UnitTester-rok4server ${TestName})
    endforeach(test)
endif(UNITTEST_ENABLED)

################### INSTALLATION

install(TARGETS rok4
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
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

if(CPACK_SYSTEM_NAME AND CPACK_SYSTEM_NAME STREQUAL "debian11" OR CPACK_SYSTEM_NAME STREQUAL "debian12")
    set(CPACK_GENERATOR "DEB;TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "rok4-styles, rok4-tilematrixsets, libfcgi-dev ,libtinyxml-dev ,zlib1g-dev ,libcurl4-openssl-dev ,libproj-dev ,libssl-dev ,libturbojpeg0-dev ,libjpeg-dev ,libc6-dev ,librados-dev ,libboost-log-dev ,libboost-filesystem-dev ,libboost-system-dev")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/cmake/debian/postinst;" )
    set(CPACK_BINARY_DEB "ON")
endif()

if(CPACK_SYSTEM_NAME AND CPACK_SYSTEM_NAME STREQUAL "alpine3")
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

