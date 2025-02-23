cmake_minimum_required(VERSION 3.2.0)
project(client)

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    message(STATUS "build clientf release version")
elseif (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
	message(STATUS "build client for debug version")
else()
    message(STATUS "build client for default version")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions("-Wno-invalid-source-encoding")
    include_directories("/usr/local/include")
    SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

##############################################################################

set(home_path ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(cpp_inc ${home_path}/cpp/include)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${cpp_inc}
)

set(base_path ${CMAKE_CURRENT_SOURCE_DIR})
set(src_paths ${base_path})

foreach(iter ${src_paths})
    aux_source_directory(${iter} src_files)
endforeach()

##############################################################################

#SET(CMAKE_VERBOSE_MAKEFILE on)

add_definitions(
    "-g"
    "-W"
    "-Wall"
    "-Werror"
    "-Wshadow"
    "-Wformat"
    "-Wpointer-arith"
    "-D_REENTRANT"
    "-Wno-long-long"
    "-Wuninitialized"
    "-D_POSIX_PTHREAD_SEMANTICS"
    "-fexceptions"
    "-Wno-unused-parameter"
    "-Wno-error=deprecated-declarations"
    "-Wno-deprecated-declarations"
    "-fPIC"
    "-O3"
    "-Wno-error=unused-result"
    "-std=c++11"
)

set(libnio ${home_path}/c/lib/libnio.a)
set(libnio_cpp ${home_path}/cpp/lib/libnio_cpp.a)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(lib_all  ${libnio_cpp} ${libnio} -lpthread -ldl)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(lib_all  ${libnio_cpp} ${libnio} -lpthread -ldl)
endif()

set(output_path ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${output_path})
link_directories(${output_path})

add_executable(client ${src_files})
target_link_libraries(client ${lib_all})

###############################################################################
