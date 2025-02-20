cmake_minimum_required(VERSION 3.2.0)
project(net_event)

if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(FATAL_ERROR "Please into another dir to build!")
endif()

#string(TOUPPER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME)
if (CMAKE_SYSTEM_NAME MATCHES "Android")
	message(STATUS "current platform: Android")
	message(STATUS "current ndk path: ${CMAKE_ANDROID_NDK}")
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
	message(STATUS "current platform: Linux")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	message(STATUS "current platform: Darwin")
	set(CMAKE_MACOSX_RPATH build)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	message(STATUS "current platform: FreeBSD")
	include_directories(
		#/usr/include/c++/4.2
		#/usr/include/c++/4.2/backward
		/usr/include
		/usr/local/include
	)

	link_directories(
		/usr/lib
		/usr/local/lib
	)

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,./:./lib:../:../lib")
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	message(STATUS "current platform: Windows")
else()
	message(FATAL_ERROR "unknown CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(CMAKE_MACOSX_RPATH build)
endif()

add_subdirectory(c)
add_subdirectory(cpp)
add_subdirectory(samples-c++/timer)
add_subdirectory(samples-c++/server)
add_subdirectory(samples-c++/client)
add_subdirectory(samples-c++/server2)
add_subdirectory(samples-c++/client2)
