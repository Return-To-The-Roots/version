
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
ENDIF (CMAKE_CROSSCOMPILING)

################################################################################

SET(version_SRCS versionMain.cpp)

ADD_EXECUTABLE(version ${version_SRCS})

################################################################################
