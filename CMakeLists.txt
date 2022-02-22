################### INITIALISATION

# check the cmake version
cmake_minimum_required(VERSION 3.10)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)

# Définit le nom du projet 
project(ROK4SERVER)

################### PARAMÈTRES

option(BUILD_VERSION "Build version" 0.0.0)
option(OBJECT_ENABLED "Build with object storage" 0)

################### VERSION
set(VERSION ${BUILD_VERSION})

message("Version ${VERSION}")

################### PACKAGE CONFIGURATION

set(CPACK_PACKAGE_NAME "ROK4SERVER")
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_DESCRIPTION "WMS/WMTS/TMS server")
set(CPACK_PACKAGE_VENDOR "IGN")
set(CPACK_GENERATOR "TGZ")

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

################### COMPILATION

# Configuration du fichier (injection de la version et de la compilation objet)
configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)

message(STATUS "Building rok4core")
set(FILEIMAGE_ENABLED 0)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/core)

message(STATUS "Building ROK4 Server")
file(GLOB ROK4SERVER_SRCS "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_executable(rok4 ${ROK4SERVER_SRCS})

include_directories(lib/core/src)

target_include_directories(rok4 PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(rok4 PUBLIC rok4core fcgi tinyxml boostlogsetup boostthread boostfilesystem boostsystem)

################### TESTS

################### DOCUMENTATION

################### INSTALLATION

INSTALL(TARGETS rok4 rok4core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

if(DEB_PACKAGE)

    # Installe les fichiers de configurations
    INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/" DESTINATION /etc/rok4/config)

    # Installe les fichiers de service
    INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4" 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION /etc/init.d
    )

    INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4.service" 
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        DESTINATION /usr/lib/systemd/system
    )

else(DEB_PACKAGE)

    # Installe les fichiers de configurations
    INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/config/" DESTINATION etc/rok4/config)
  
    # Installe les fichiers de service
    INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION etc/init.d
    )

    INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/service/rok4.service" 
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        DESTINATION usr/lib/systemd/system
    )

endif(DEB_PACKAGE)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BUILD_ARCHITECTURE "64bit")
else()
    set(BUILD_ARCHITECTURE "32bit")
endif()
set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${BUILD_ARCHITECTURE}")
include(CPack)