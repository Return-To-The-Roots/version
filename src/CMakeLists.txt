
IF (CMAKE_CROSSCOMPILING)
	MESSAGE(STATUS "version: Reverting to host compiler to avoid \"cannot execute binary file\"")

	SET(CMAKE_CXX_COMPILER "g++")
	SET(CMAKE_CXX_FLAGS "")
	SET(CMAKE_CXX_LINK_FLAGS "")
	SET(CMAKE_EXE_LINKER_FLAGS "-lz")

	SET(CMAKE_CXX_STANDARD_LIBRARIES "")
	
	SET(CMAKE_EXECUTABLE_SUFFIX "")
	SET(CMAKE_GNULD_IMAGE_VERSION "")

	SET(CMAKE_CXX_COMPILE_OBJECT  "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} -o <OBJECT> -c <SOURCE>")
	SET(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_LINK_FLAGS} <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
ENDIF ()

################################################################################

SET(version_SRCS versionMain.cpp)
if(ClangFormat_FOUND)
    add_ClangFormat_files(${version_SRCS})
endif()

FIND_PACKAGE(Boost 1.55.0 COMPONENTS filesystem system REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIR})

ADD_EXECUTABLE(version ${version_SRCS})
TARGET_LINK_LIBRARIES(version ${Boost_LIBRARIES} nowide-static)

################################################################################
