cmake_minimum_required(VERSION 3.16)

project(dlms_client
  VERSION 0.1.0
  LANGUAGES CXX)

option(DLMS_BUILD_TESTS "Build GoogleTest tests" ON)
option(DLMS_USE_SYSTEM_GTEST "Use system-installed GoogleTest" OFF)

add_library(dlms_client
  src/client/client_options.cpp
  src/client/client_status.cpp)

target_compile_features(dlms_client PUBLIC cxx_std_11)

target_include_directories(dlms_client
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(DLMS_CLIENT_HAS_OPTIONS_API ON CACHE INTERNAL
  "dlms-client exposes public facade status and options")

if(DLMS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
