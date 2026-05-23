if(DLMS_USE_SYSTEM_GTEST)
  find_package(GTest REQUIRED)
elseif(NOT TARGET GTest::gtest_main)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(dlms_client_tests
  client/test_client.cpp
  client/test_client_options.cpp
  client/test_client_status.cpp)

target_link_libraries(dlms_client_tests
  PRIVATE
    dlms_client
    GTest::gtest_main)

include(GoogleTest)
if(WIN32)
  get_filename_component(DLMS_MINGW_BIN_DIR
    "${CMAKE_CXX_COMPILER}" DIRECTORY)
  add_test(NAME dlms_client_tests
    COMMAND ${CMAKE_COMMAND} -E env
      "PATH=${DLMS_MINGW_BIN_DIR}"
      $<TARGET_FILE:dlms_client_tests>)
else()
  gtest_discover_tests(dlms_client_tests)
endif()
