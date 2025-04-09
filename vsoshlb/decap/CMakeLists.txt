cmake_minimum_required (VERSION 3.9)
set(VSOSHLB_INCLUDE_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../>")

find_package(Glog REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(Threads)

add_library(decap STATIC
    XdpDecap.cpp
    XdpDecap.h
    XdpDecapStructs.h
)

target_link_libraries(decap
    bpfadapter
    "Folly::folly"
    glog::glog
)

target_include_directories(
  decap PUBLIC
  ${GFLAGS_INCLUDE_DIR}
  ${VSOSHLB_INCLUDE_DIR}
)

install(TARGETS decap
  EXPORT vsoshlb-exports
  DESTINATION decap
)

if (BUILD_TESTS)
  add_subdirectory(testing)
endif()
