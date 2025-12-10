cmake_minimum_required(VERSION 3.16)
project(battery_sim_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Zet pad naar jouw lib directory
include_directories(${CMAKE_SOURCE_DIR}/../../lib/battery_sim)

# GoogleTest ophalen (vendored via FetchContent)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(battery_sim_tests
  test_battery_sim.cpp
)

target_link_libraries(battery_sim_tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(battery_sim_tests)
