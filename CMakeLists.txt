cmake_minimum_required(VERSION 2.8)

project(webserver)

set(SOURCE_EXE main.cpp)

set(SOURCE_LIB WebServer.cpp)

add_library(webserver STATIC ${SOURCE_LIB})

add_executable(final ${SOURCE_EXE})

find_package(Threads REQUIRED)
if(NOT Threads_FOUND)
	message(SEND_ERROR "Failed to find pThread")
	return()
endif()

target_link_libraries(final webserver ${CMAKE_THREAD_LIBS_INIT})
