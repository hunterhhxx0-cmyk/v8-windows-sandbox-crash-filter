cmake_minimum_required(VERSION 3.20)
project(v8_windows_sandbox_crash_filter LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "The prototype currently supports Windows only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "The initial prototype targets 64-bit Windows (x64).")
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(crash_filter_tests
  test/access_violation_parser_unittest.cc
  test/memory_region_query_unittest.cc
)
target_compile_features(crash_filter_tests PRIVATE cxx_std_17)
target_include_directories(crash_filter_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(crash_filter_tests PRIVATE GTest::gtest_main)
if(MSVC)
  target_compile_options(crash_filter_tests PRIVATE /W4 /WX)
endif()

include(GoogleTest)
gtest_discover_tests(crash_filter_tests)
