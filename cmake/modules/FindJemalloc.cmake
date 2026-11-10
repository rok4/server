
# CMake module to search for Jemalloc library
#
# If it's found it sets JEMALLOC_FOUND to TRUE
# and following variables are set:
#    JEMALLOC_INCLUDE_DIR
#    JEMALLOC_LIBRARY

FIND_PATH(JEMALLOC_INCLUDE_DIR jemalloc.h
    /usr/local/include 
    /usr/include 
    /usr/include/jemalloc
    c:/msys/local/include
    )

FIND_LIBRARY(JEMALLOC_LIBRARY NAMES libjemalloc.so PATHS 
    /usr/local/lib 
    /usr/lib
    /lib
    /usr/lib64
    /usr/lib/x86_64-linux-gnu
    c:/msys/local/lib
    )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Jemalloc" DEFAULT_MSG JEMALLOC_INCLUDE_DIR JEMALLOC_LIBRARY )
