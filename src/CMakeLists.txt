
include_directories(
  ${PROJECT_SOURCE_DIR}/jaulib/include
  ${PROJECT_SOURCE_DIR}/IXWebSocket
  ${SYS_INCLUDE_DIRS}
)

set (${PROJECT_NAME}_SRCS
  ${PROJECT_SOURCE_DIR}/jaulib/src/debug.cpp
  ${PROJECT_SOURCE_DIR}/jaulib/src/basic_types.cpp
  ${PROJECT_SOURCE_DIR}/jaulib/src/base_codec.cpp
  ${PROJECT_SOURCE_DIR}/jaulib/src/byte_stream.cpp
#  ${PROJECT_SOURCE_DIR}/jaulib/src/cpuid.cpp
  ${PROJECT_SOURCE_DIR}/jaulib/src/environment.cpp
  ${PROJECT_SOURCE_DIR}/jaulib/src/file_util.cpp
  ${PROJECT_SOURCE_DIR}/jaulib/src/io_util.cpp
#  ${PROJECT_SOURCE_DIR}/jaulib/src/os_support.cpp
#  ${PROJECT_SOURCE_DIR}/jaulib/src/unix/user_info.cpp
  ${PROJECT_SOURCE_DIR}/src/HorizonFetch.cpp
)

set(target ${PROJECT_NAME})
add_executable(${target} ${${PROJECT_NAME}_SRCS})
target_compile_options(${target} PUBLIC ${${PROJECT_NAME}_CXX_FLAGS})
target_link_options(${target} PUBLIC ${${PROJECT_NAME}_EXE_LINKER_FLAGS})
target_link_libraries (
  ${target}
  ${LIBCURL_LIBNAME}
  ${LIBUNWIND_LIBNAME}
  ${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
