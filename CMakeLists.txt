cmake_minimum_required(VERSION 3.16)

project(dlms_client
  VERSION 0.1.0
  LANGUAGES CXX)

option(DLMS_BUILD_TESTS "Build GoogleTest tests" ON)
option(DLMS_USE_SYSTEM_GTEST "Use system-installed GoogleTest" OFF)

add_library(dlms_client
  src/client/client.cpp
  src/client/client_options.cpp
  src/client/client_status.cpp)

if(NOT TARGET dlms_xdlms AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../dlms-xdlms/CMakeLists.txt")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../dlms-xdlms" "${CMAKE_CURRENT_BINARY_DIR}/_deps/dlms-xdlms")
endif()

if(NOT TARGET dlms_association AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../dlms-association/CMakeLists.txt")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../dlms-association" "${CMAKE_CURRENT_BINARY_DIR}/_deps/dlms-association")
endif()

if(NOT TARGET dlms_profile AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../dlms-profile/CMakeLists.txt")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../dlms-profile" "${CMAKE_CURRENT_BINARY_DIR}/_deps/dlms-profile")
endif()

target_compile_features(dlms_client PUBLIC cxx_std_11)

target_include_directories(dlms_client
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(dlms_client
  PUBLIC
    dlms_association
    dlms_profile
    dlms_xdlms)

set(DLMS_CLIENT_HAS_OPTIONS_API ON CACHE INTERNAL
  "dlms-client exposes public facade status and options")

if(DLMS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
