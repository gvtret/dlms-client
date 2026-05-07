cmake_minimum_required(VERSION 3.16)

project(dlms_client
  VERSION 0.1.0
  LANGUAGES CXX)

add_library(dlms_client INTERFACE)

target_compile_features(dlms_client INTERFACE cxx_std_11)

target_include_directories(dlms_client
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(DLMS_CLIENT_HAS_DOCUMENTED_API ON CACHE INTERNAL
  "dlms-client public facade API is documented")
