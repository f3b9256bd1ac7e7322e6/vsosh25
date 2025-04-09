cmake_minimum_required (VERSION 3.9 FATAL_ERROR)

project(
  vsoshlb
)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(VSOSHLB_INCLUDE_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "setting C++ standard to C++${CMAKE_CXX_STANDARD}")

# for clang's compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(fmt CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(Glog REQUIRED)
find_package(Gflags REQUIRED)
find_package(Threads)

include_directories(".")
add_subdirectory(vsoshlb)

if(BUILD_TESTS)
  enable_testing()
  include(VsoshlbTest)
endif()

if(DEFINED ENV{CMAKE_BUILD_EXAMPLE_THRIFT})
    message("Building example of thrift and grpc based vsoshlb server")
    add_subdirectory(example)
endif()

if(DEFINED ENV{CMAKE_BUILD_EXAMPLE_GRPC})
    add_subdirectory(example_grpc)
endif()

if(DEFINED ENV{CMAKE_BUILD_TOOLS})
    add_subdirectory(tools)
endif()

install(
  EXPORT vsoshlb-exports
  FILE vsoshlb-targets.cmake
  NAMESPACE vsoshlb::
  DESTINATION lib/cmake/vsoshlb/
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/vsoshlb-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vsoshlb-config.cmake
  INSTALL_DESTINATION lib/cmake/vsoshlb/
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/vsoshlb-config.cmake
  DESTINATION lib/cmake/vsoshlb/
)
