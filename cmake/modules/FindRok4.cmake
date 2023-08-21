
# If it's found it sets ROK4_FOUND to TRUE
# and following variables are set:
#    ROK4_INCLUDE_DIR
#    ROK4_LIBRARY

FIND_PATH(ROK4_INCLUDE_DIR enums/Format.h 
    /usr/local/include/rok4
    /usr/include/rok4
    c:/msys/local/include/rok4
    )

FIND_LIBRARY(ROK4_LIBRARY NAMES librok4.so PATHS
    /usr/lib/x86_64-linux-gnu/
    /usr/local/lib 
    /usr/lib
    /usr/lib64
    c:/msys/local/lib
    )


INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Rok4" DEFAULT_MSG ROK4_INCLUDE_DIR ROK4_LIBRARY )