# CMake module to search for CppUnit headers
#
# If it's found it sets FCGI_FOUND to TRUE
# and following variables are set:
#    CPPUNIT_INCLUDE_DIR
#    CPPUNIT_LIBRARY

FIND_PATH(CPPUNIT_INCLUDE_DIR cppunit/Test.h
  /usr/local/include 
  /usr/include 
  c:/msys/local/include
  )

FIND_LIBRARY(CPPUNIT_LIBRARY NAMES libcppunit.so PATHS 
  /usr/local/lib 
  /usr/lib 
  /usr/lib64
  c:/msys/local/lib
  /usr/lib/x86_64-linux-gnu/
  )

INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "CppUnit" DEFAULT_MSG CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY )