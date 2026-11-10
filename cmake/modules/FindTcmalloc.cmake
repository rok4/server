
# CMake module to search for Tcmalloc library
#
# If it's found it sets TCMALLOC_FOUND to TRUE
# and following variables are set:
#    TCMALLOC_LIBRARY

FIND_LIBRARY(TCMALLOC_LIBRARY NAMES libtcmalloc.so PATHS 
    /usr/local/lib 
    /usr/lib
    /lib
    /usr/lib64
    /usr/lib/x86_64-linux-gnu
    c:/msys/local/lib
    )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Tcmalloc" DEFAULT_MSG TCMALLOC_LIBRARY )
