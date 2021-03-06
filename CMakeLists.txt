# SET(CMAKE_SYSTEM_NAME      Windows)
# SET(CMAKE_SYSTEM_PROCESSOR i686)

# SET(CMAKE_C_COMPILER       /usr/bin/i686-w64-mingw32-gcc-win32 )
# SET(CMAKE_CXX_COMPILER     /usr/bin/i686-w64-mingw32-g++-win32 )

#SET(CMAKE_C_COMPILER       gcc )
#SET(CMAKE_CXX_COMPILER     g++ )

cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++11)

if (BUILD_STATIC)
	set(BUILD_STATIC_BOOST ON)

	message( STATUS "Build static" )

	if (UNIX)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")
	endif()

	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()

if (BUILD_STATIC_BOOST)
	set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost COMPONENTS system REQUIRED)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost::system.")
	return()
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)

add_executable(starter
    proc.cpp
    main.cpp
)


if (BUILD_STATIC)
	set_target_properties(starter PROPERTIES LINK_SEARCH_START_STATIC 1)
	set_target_properties(starter PROPERTIES LINK_SEARCH_END_STATIC 1)
endif()

target_link_libraries (starter pthread boost_system boost_filesystem boost_iostreams)
# target_link_libraries (starter boost_system-mgw49-mt boost_iostreams-mgw49-mt boost_filesystem-mgw49-mt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
