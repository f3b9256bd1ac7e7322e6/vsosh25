cmake_minimum_required (VERSION 3.9)
set(VSOSHLB_TPR_INCLUDE_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../>")

find_package(Glog REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(Threads)

add_subdirectory(bpf_util)

set(LIBBPF_INCLUDE_DIRS "${LIB_BPF_PREFIX}/include")
set(LIBBPF_LIBRARIES "${LIB_BPF_PREFIX}/lib")
set(BPFOBJECT_VMLINUX_H "bpf/vmlinux/vmlinux.h")
set(BPFOBJECT_BPFTOOL_EXE "${CMAKE_INSTALL_PREFIX}/bin/bpftool")
set(BPFOBJECT_EXTRA_DEFINES "-DVSOSHLB_CMAKE_BUILD")
find_package(BpfObject REQUIRED)

bpf_object(tpr_bpf
    bpf/tcp_pkt_router.bpf.c

)

add_library(vsoshlb_tpr STATIC
    TPRStatsPoller.cpp
    TcpPktRouter.cpp
)

target_compile_definitions(vsoshlb_tpr PUBLIC VSOSHLB_CMAKE_BUILD=1)

target_link_libraries(vsoshlb_tpr
    "Folly::folly"
    glog::glog
    ${BPF_LINK_LIBRARIES}
    tpr_bpf_skel
)

target_include_directories(
  vsoshlb_tpr PUBLIC
  ${GFLAGS_INCLUDE_DIR}
  ${VSOSHLB_TPR_INCLUDE_DIR}
)
