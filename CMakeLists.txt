# If we are cross-compiling invoke a new CMake build tree
if(CMAKE_CROSSCOMPILING AND NOT RTTR_NO_SUBBUILDS)
	set(HOST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/hostBuild)
	file(MAKE_DIRECTORY ${HOST_BUILD_DIR})

	set(extraArgs )
	if(RTTR_HOST_CMAKE_GENERATOR)
		list(APPEND extraArgs "-G ${RTTR_HOST_CMAKE_GENERATOR}")
	endif()
	if(RTTR_HOST_CMAKE_TOOLSET)
		list(APPEND extraArgs "-T ${RTTR_HOST_CMAKE_TOOLSET}")
	endif()
	if(RTTR_HOST_CMAKE_PLATFORM)
		list(APPEND extraArgs "-A ${RTTR_HOST_CMAKE_PLATFORM}")
	endif()

	MESSAGE(STATUS "${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${HOST_BUILD_DIR} -DRTTR_NO_SUBBUILDS=ON ${extraArgs}")

	if(DEFINED ENV{CC})
		set(_oldCC $ENV{CC})
		unset(ENV{CC})
	else()
		undef(_oldCC)
	endif()	
	if(DEFINED ENV{CXX})
		set(_oldCXX $ENV{CXX})
		unset(ENV{CXX})
	else()
		undef(_oldCXX)
	endif()

	execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}
					-DCMAKE_BUILD_TYPE=Release
					-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${HOST_BUILD_DIR}
					-DRTTR_NO_SUBBUILDS=ON
					${extraArgs}
					WORKING_DIRECTORY ${HOST_BUILD_DIR}
					RESULT_VARIABLE VERSION_CFG_RESULT
					ERROR_VARIABLE VERSION_CFG_ERROR
					OUTPUT_STRIP_TRAILING_WHITESPACE
					ERROR_STRIP_TRAILING_WHITESPACE)
	
	if(DEFINED _oldCC)
		set(ENV{CC] ${_oldCC})
	endif()
	if(DEFINED _oldCXX)
		set(ENV{CXX] ${_oldCXX})
	endif()
	if(NOT VERSION_CFG_RESULT STREQUAL "0")
		message(FATAL_ERROR "Configuring version tool failed with: ${VERSION_CFG_RESULT}\nError: ${VERSION_CFG_ERROR}")
	endif()

else()
	cmake_minimum_required(VERSION 2.8.12)
	if(NOT CMAKE_VERSION VERSION_LESS 3.1)
		# Simplified variable expansion. No change for usual variable references so use faster one
		cmake_policy(SET CMP0053 NEW)
	endif()

	project(version)

	if(MSVC AND NOT CMAKE_LIBRARY_ARCHITECTURE)
		if(CMAKE_SIZEOF_VOID_P EQUAL 4)
			set(CMAKE_LIBRARY_ARCHITECTURE "x86" CACHE INTERNAL "")
		else()
			set(CMAKE_LIBRARY_ARCHITECTURE "x64" CACHE INTERNAL "")
		endif()
	endif()
	if(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	endif()

	if(NOT TARGET nowide-static)
		set(RTTR_LIBUTIL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../libutil" CACHE FILEPATH "Path to libutil")
		list(APPEND CMAKE_MODULE_PATH "${RTTR_LIBUTIL_PATH}/cmake/Modules")
		include(RttrBoostCfg)

		find_package(Boost 1.55.0)
		# Set hint for multi arhcs
		if(Boost_FOUND)
			list(APPEND CMAKE_PREFIX_PATH ${Boost_INCLUDE_DIR}/stage ${Boost_INCLUDE_DIR})
		endif()

		set(RTTR_NO_LIBUTIL ON)
		add_subdirectory(${RTTR_LIBUTIL_PATH} libutil)
	endif()
endif()

add_subdirectory(src)
