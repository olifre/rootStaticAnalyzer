# Check if cmake has the required version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0 FATAL_ERROR)

# Set project name
PROJECT(rootStaticAnalyzer)

# Extra warnings. 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wshadow -Wextra")

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

include(CheckCXXCompilerFlag)

# We require C++11, even in case ROOT was not built with C++11 support. 
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Use root-config to auto-include ROOT (root-config must be in PATH)
EXEC_PROGRAM( root-config ARGS "--incdir" OUTPUT_VARIABLE ROOT_INCLUDE_DIR RETURN_VALUE RETVAR1)
MESSAGE(STATUS "Found ROOT include path at ${ROOT_INCLUDE_DIR}")

EXEC_PROGRAM( root-config ARGS "--libs" OUTPUT_VARIABLE ROOT_LIBS RETURN_VALUE RETVAR2)

EXEC_PROGRAM( root-config ARGS "--cflags" OUTPUT_VARIABLE ROOT_CFLAGS RETURN_VALUE RETVAR3)
MESSAGE(STATUS "Using cflags from ROOT: ${ROOT_CFLAGS}")

IF(${RETVAR1}${RETVAR2}${RETVAR3} MATCHES "000")
	include_directories(${ROOT_INCLUDE_DIR})

	# Prefer our flags, but don't deny the ROOT ones: 
	SET(CMAKE_CXX_FLAGS "${ROOT_CFLAGS} ${CMAKE_CXX_FLAGS}")
ELSE()
	MESSAGE(FATAL_ERROR "ERROR: root-config did not provide LIBRARY and INCLUDE path, is ROOT installed and root-config in PATH?")
ENDIF()

find_package(OptionParser REQUIRED COMPONENT MAYBEBUILTIN)
include_directories(${OptionParser_INCLUDE_DIRS})
SET(CMAKE_CXX_FLAGS "${OptionParser_CFLAGS} ${CMAKE_CXX_FLAGS}")

ADD_SUBDIRECTORY(src)

#SET(MANDIR "share/man/man1" CACHE FILEPATH "mandir")
#INSTALL(FILES rootStaticAnalyzer.1 DESTINATION ${MANDIR})
