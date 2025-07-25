cmake_minimum_required(VERSION 3.2.0)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release")

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

add_definitions(
	"-W"
	"-O3"
	"-fPIC"
	"-Wall"
	"-Werror"
	"-Wshadow"
	"-Wformat"
	"-Wpointer-arith"
	"-D_REENTRANT"
	"-D_USE_FAST_MACRO"
	"-Wno-long-long"
	"-D_POSIX_PTHREAD_SEMANTICS"
	"-DHAS_MYSQL_DLL"
	"-DHAS_SQLITE_DLL"
	"-DHAS_ZLIB_DLL"
	"-Wno-unused-parameter"
	"-std=c++11"
)

if (CMAKE_BUILD_TYPE STREQUAL "")
	set(CMAKE_BUILD_TYPE "DEBUG")
else()
	string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
	add_definitions("-O3")
else()
	add_definitions("-g")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if(CMAKE_SYSTEM_NAME MATCHES "Android")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CMAKE_SHARED_LINKER_FLAGS "/DYNAMICBASE ws2_32.lib IPHlpApi.lib")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(src ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mqtt
   )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/../c/src
)

foreach(iter ${src})
	aux_source_directory(${iter} lib_src)
endforeach()

if(NOT CMAKE_SYSTEM_NAME MATCHES "Android")
	set(lib_output_path ${CMAKE_CURRENT_SOURCE_DIR}/lib)

	set(LIBRARY_OUTPUT_PATH ${lib_output_path})
	add_library(nio_cpp_static STATIC ${lib_src})
	SET_TARGET_PROPERTIES(nio_cpp_static PROPERTIES OUTPUT_NAME "nio_cpp")

	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${lib_output_path})
	link_directories(${lib_output_path})

	install(TARGETS nio_cpp_static
		ARCHIVE DESTINATION /usr/local/lib)
	install(DIRECTORY include/nio
		DESTINATION /usr/local/include
		FILES_MATCHING PATTERN "*.hpp")
endif()

##############################################################################
