cmake_minimum_required(VERSION 2.8)

project(gameap-starter)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set (CMAKE_CXX_STANDARD 11)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_definitions(-std=c++11)
endif()

if (BUILD_STATIC)
	set(BUILD_STATIC_BOOST ON)

	message( STATUS "Build static" )

    set(BUILD_SHARED_LIBS OFF)

	if (UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")
        set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
    elseif(WIN32)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib;.dll")
	endif()
endif()

if (BUILD_STATIC_BOOST)
	set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost COMPONENTS system filesystem iostreams thread REQUIRED)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost::system.")
	return()
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs)

add_executable(gameap-starter
    proc.cpp
    main.cpp
)

if (BUILD_STATIC)
	set_target_properties(gameap-starter PROPERTIES LINK_SEARCH_START_STATIC 1)
	set_target_properties(gameap-starter PROPERTIES LINK_SEARCH_END_STATIC 1)
endif()

target_link_libraries (gameap-starter ${Boost_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
endif()
