cmake_minimum_required(VERSION 3.14)
project(PipeYAML
    VERSION 0.1.0
    DESCRIPTION "A modern C++ YAML 1.2 parser and emitter library"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Library target
# =============================================================================
file(GLOB_RECURSE PYPEYAML_SRCS src/*.cpp)

add_library(pipeyaml ${PYPEYAML_SRCS})
target_include_directories(pipeyaml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(pipeyaml PUBLIC cxx_std_17)

# =============================================================================
# tinyTest integration (header-only, no install needed)
# =============================================================================
if(EXISTS /home/zhaoliang/tinyTest)
    add_subdirectory(/home/zhaoliang/tinyTest ${CMAKE_BINARY_DIR}/tinyTest)
endif()

# =============================================================================
# Unit tests
# =============================================================================
option(PYPEYAML_BUILD_TESTS "Build unit tests" ON)
if(PYPEYAML_BUILD_TESTS AND TARGET TinyTest::tiny_test)
    enable_testing()

    macro(add_pipeyaml_test test_name test_source)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE pipeyaml TinyTest::tiny_test)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endmacro()

    add_pipeyaml_test(test_stream tests/unit/test_stream.cpp)
    add_pipeyaml_test(test_scanner tests/unit/test_scanner.cpp)
    add_pipeyaml_test(test_node tests/unit/test_node.cpp)
    add_pipeyaml_test(test_parser tests/unit/test_parser.cpp)
    add_pipeyaml_test(test_emitter tests/unit/test_emitter.cpp)
    add_pipeyaml_test(test_roundtrip tests/integration/test_roundtrip.cpp)
endif()

# =============================================================================
# Install
# =============================================================================
include(GNUInstallDirs)
install(TARGETS pipeyaml EXPORT PipeYAMLTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT PipeYAMLTargets
    FILE PipeYAMLTargets.cmake
    NAMESPACE PipeYAML::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PipeYAML
)

# =============================================================================
# Benchmarks (Google Benchmark)
# =============================================================================
option(PYPEYAML_BUILD_BENCHMARKS "Build benchmarks" ON)
if(PYPEYAML_BUILD_BENCHMARKS)
    set(BENCHMARK_DIR /home/zhaoliang/leveldb/third_party/benchmark)
    if(EXISTS ${BENCHMARK_DIR}/build/src/libbenchmark.a)
        add_executable(bench_parse
            benchmarks/bench_parse.cpp
            benchmarks/bench_data.h
        )
        target_include_directories(bench_parse PRIVATE
            ${BENCHMARK_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks
        )
        target_link_libraries(bench_parse PRIVATE
            pipeyaml
            ${BENCHMARK_DIR}/build/src/libbenchmark.a
            ${BENCHMARK_DIR}/build/src/libbenchmark_main.a
            pthread
            rt
        )
        add_custom_target(benchmark
            COMMAND bench_parse
            DEPENDS bench_parse
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endif()
