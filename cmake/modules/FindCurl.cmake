
# CMake module to search for Curl library
#
# If it's found it sets CURL_FOUND to TRUE
# and following variables are set:
#    CURL_INCLUDE_DIR
#    CURL_LIBRARY

FIND_PATH(CURL_INCLUDE_DIR curl.h 
    /usr/local/include 
    /usr/include/x86_64-linux-gnu/curl
    /usr/include 
    /usr/local/include/curl
    /usr/include/curl
    c:/msys/local/include
    C:/dev/cpp/libcurl/src
    )

FIND_LIBRARY(CURL_LIBRARY NAMES libcurl.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64 
    /usr/local/lib/curl 
    /usr/lib/curl 
    c:/msys/local/lib
    C:/dev/cpp/libcurl/src
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Curl" DEFAULT_MSG CURL_INCLUDE_DIR CURL_LIBRARY )
