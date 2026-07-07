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
set(TINYTEST_DIR "" CACHE PATH "Path to tinyTest (e.g. /path/to/tinyTest)")
if(EXISTS "${TINYTEST_DIR}")
    add_subdirectory("${TINYTEST_DIR}" "${CMAKE_BINARY_DIR}/tinyTest")
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
set(YAML_CPP_DIR "" CACHE PATH "Path to yaml-cpp root (set to enable yaml-cpp comparison)")
set(BENCHMARK_DIR "" CACHE PATH "Path to Google Benchmark (e.g. /path/to/benchmark)")
option(PYPEYAML_BUILD_BENCHMARKS "Build benchmarks" ON)
if(PYPEYAML_BUILD_BENCHMARKS)
    if(EXISTS "${BENCHMARK_DIR}/build/src/libbenchmark.a")
        # PipeYAML benchmark
        add_executable(bench_parse
            benchmarks/bench_parse.cpp
            benchmarks/bench_data.h
        )
        target_include_directories(bench_parse PRIVATE
            "${BENCHMARK_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
        )
        target_link_libraries(bench_parse PRIVATE
            pipeyaml
            "${BENCHMARK_DIR}/build/src/libbenchmark.a"
            "${BENCHMARK_DIR}/build/src/libbenchmark_main.a"
            pthread
            rt
        )

        # yaml-cpp comparison benchmark
        if(NOT "${YAML_CPP_DIR}" STREQUAL "")
            set(YAML_CPP_INCLUDE_DIR "${YAML_CPP_DIR}/include")
            # Try shared library first, then static
            if(EXISTS "${YAML_CPP_DIR}/build/libyaml-cpp.a")
                set(YAML_CPP_LIB "${YAML_CPP_DIR}/build/libyaml-cpp.a")
            elseif(EXISTS "${YAML_CPP_DIR}/lib/libyaml-cpp.so")
                set(YAML_CPP_LIB "${YAML_CPP_DIR}/lib/libyaml-cpp.so")
            endif()
        endif()
        if(DEFINED YAML_CPP_LIB AND EXISTS "${YAML_CPP_INCLUDE_DIR}/yaml-cpp/yaml.h" AND EXISTS "${YAML_CPP_LIB}")
            add_executable(bench_yamlcpp
                benchmarks/bench_yamlcpp.cpp
                benchmarks/bench_data.h
            )
            target_include_directories(bench_yamlcpp PRIVATE
                "${BENCHMARK_DIR}/include"
                "${YAML_CPP_INCLUDE_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks"
            )
            target_link_libraries(bench_yamlcpp PRIVATE
                "${YAML_CPP_LIB}"
                "${BENCHMARK_DIR}/build/src/libbenchmark.a"
                "${BENCHMARK_DIR}/build/src/libbenchmark_main.a"
                pthread
                rt
            )

            add_custom_target(bench_compare
                COMMAND echo "=== PipeYAML ===" && ./bench_parse && echo "=== yaml-cpp ===" && ./bench_yamlcpp
                DEPENDS bench_parse bench_yamlcpp
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            )
        endif()

        add_custom_target(benchmark
            COMMAND bench_parse
            DEPENDS bench_parse
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
    endif()
endif()
