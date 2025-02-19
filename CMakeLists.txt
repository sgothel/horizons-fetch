cmake_minimum_required (VERSION 3.13.0 FATAL_ERROR)
set(USE_LIBCURL ON CACHE BOOL "" FORCE)
set(USE_LIBUNWIND ON CACHE BOOL "" FORCE)
set(BUILDJAVA OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE STRING "" FORCE)

# Enforce debug mode and disabled testing (low power machines, ..)
#
# set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
# set(CMAKE_C_COMPILER "gcc" CACHE STRING "" FORCE)
# set(CMAKE_CXX_COMPILER "g++" CACHE STRING "" FORCE)

include(jaulib/JaulibSetup.cmake)
JaulibPreset()

project (horizonfetch LANGUAGES C CXX)

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

set(DONT_USE_RTTI OFF)

JaulibSetup()

# set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)
# set(DEBUG ON)

# Fixup clangd w/ absolute path
set (app_clangd_source_args "-isystem, ${CMAKE_CURRENT_SOURCE_DIR}/jaulib/include, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/jaulib/src")
set (app_clangd_source_args "${app_clangd_source_args}, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/jaulib/include/catch2_jau, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/jaulib/test")
set (app_clangd_source_args "${app_clangd_source_args}, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/src")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/clangd.in
                ${CMAKE_CURRENT_BINARY_DIR}/.clangd)
file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/.clangd ${CMAKE_CURRENT_SOURCE_DIR}/.clangd)

add_subdirectory (src)
