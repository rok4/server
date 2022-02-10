
# CMake module to search for Curl library
#
# If it's found it sets RADOS_FOUND to TRUE
# and following variables are set:
#    PROJ_INCLUDE_DIR
#    PROJ_LIBRARY

FIND_PATH(PROJ_INCLUDE_DIR proj.h 
    /usr/local/include 
    /usr/include
    c:/msys/local/include
    C:/dev/cpp/libproj/src
    )

FIND_LIBRARY(PROJ_LIBRARY NAMES libproj.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64
    c:/msys/local/lib
    C:/dev/cpp/libproj/src
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Proj" DEFAULT_MSG PROJ_INCLUDE_DIR PROJ_LIBRARY )