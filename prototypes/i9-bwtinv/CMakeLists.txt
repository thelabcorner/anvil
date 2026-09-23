cmake_minimum_required(VERSION 3.20)
project(i9_bwtinv LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Upstream is NOT modified: both targets compile the same repo file, only the
# OpenMP prototype target adds -DLIBSAIS_OPENMP (a documented libsais build flag).
set(SAIS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/libsais/src/libsais.c")
set(SAIS_INC "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/libsais/include")

# ---- reference: identical to the shipped ANVIL build (no OpenMP) ------------
add_library(sais_ref STATIC "${SAIS_SRC}")
target_include_directories(sais_ref PUBLIC "${SAIS_INC}")
target_compile_options(sais_ref PRIVATE /O2 /DNDEBUG)

add_executable(unbwt_bench_ref unbwt_bench.cpp)
target_compile_options(unbwt_bench_ref PRIVATE /O2 /DNDEBUG)
target_link_libraries(unbwt_bench_ref PRIVATE sais_ref)
target_link_options(unbwt_bench_ref PRIVATE /STACK:16777216)

# ---- prototype: libsais with the documented OpenMP support flag -------------
add_library(sais_omp STATIC "${SAIS_SRC}")
target_include_directories(sais_omp PUBLIC "${SAIS_INC}")
target_compile_options(sais_omp PRIVATE /O2 /DNDEBUG /DLIBSAIS_OPENMP /openmp)

add_executable(unbwt_bench_omp unbwt_bench.cpp)
target_compile_options(unbwt_bench_omp PRIVATE /O2 /DNDEBUG /openmp)
target_compile_definitions(unbwt_bench_omp PRIVATE USE_OMP=1 LIBSAIS_OPENMP=1)
target_link_libraries(unbwt_bench_omp PRIVATE sais_omp "C:/Program Files/LLVM/lib/libomp.lib")
target_link_options(unbwt_bench_omp PRIVATE /STACK:16777216)

add_executable(probe_aux probe_aux.cpp)
target_compile_options(probe_aux PRIVATE /O2 /DNDEBUG)
target_link_libraries(probe_aux PRIVATE sais_ref)

add_executable(probe_file probe_file.cpp)
target_compile_options(probe_file PRIVATE /O2 /DNDEBUG)
target_link_libraries(probe_file PRIVATE sais_ref)
target_link_options(probe_file PRIVATE /STACK:16777216)
