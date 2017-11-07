
set(version_SRCS versionMain.cpp)
if(ClangFormat_FOUND)
    add_ClangFormat_files(${version_SRCS})
endif()

if(CMAKE_CROSSCOMPILING AND NOT RTTR_NO_SUBBUILDS)
	# This relies on the explicit setting of the build type and output directory. Otherwise we would need:
	# include(${CMAKE_CURRENT_BINARY_DIR}/hostBuild/versionTargets.cmake)
	# get_target_property(RTTR_VERSION_TOOL_CMD version IMPORTED_LOCATION_RELEASE)

	include(${HOST_BUILD_DIR}/versionExeName.cmake)
	set(RTTR_VERSION_TOOL_CMD ${HOST_BUILD_DIR}/${VERSION_EXE_NAME})
	set(dependencies )
	foreach(src ${version_SRCS})
		get_filename_component(absCurFile ${src} ABSOLUTE)
		set(dependencies "${dependencies} CXX ${absCurFile}")
	endforeach()
	add_custom_command(OUTPUT ".versionBuild.txt"
						COMMAND ${CMAKE_COMMAND} --build ${HOST_BUILD_DIR} --config Release
						COMMAND ${CMAKE_COMMAND} -E touch ".versionBuild.txt"
						DEPENDS ${version_SRCS}
						IMPLICIT_DEPENDS ${dependencies}
						COMMENT "Building version tool")
	# Make this available as 'version'
	add_executable(version IMPORTED GLOBAL)
	set_target_properties(version PROPERTIES IMPORTED_LOCATION ${RTTR_VERSION_TOOL_CMD})
	add_custom_target(BuildVersion DEPENDS ".versionBuild.txt")
	add_dependencies(version BuildVersion)
else()
	find_package(Boost 1.55.0 REQUIRED COMPONENTS filesystem system)
	include_directories(SYSTEM ${Boost_INCLUDE_DIR})

	add_executable(version ${version_SRCS})
	target_link_libraries(version ${Boost_LIBRARIES} nowide-static)
	file(WRITE "${CMAKE_BINARY_DIR}/versionExeName.cmake" "set(VERSION_EXE_NAME version${CMAKE_EXECUTABLE_SUFFIX})")
	#export(TARGETS version FILE "${CMAKE_BINARY_DIR}/versionTargets.cmake")
endif()
