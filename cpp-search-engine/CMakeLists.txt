cmake_minimum_required(VERSION 3.20)
project(IndexForge VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and warning flags
if (MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# Directories
include_directories(include)

# IndexForge core library (static and shared)
set(CORE_SOURCES
    src/tokenizer.cpp
    src/stemmer.cpp
    src/compression.cpp
    src/inverted_index.cpp
    src/ranking.cpp
    src/query_parser.cpp
    src/retrieval_engine.cpp
    src/snippet_generator.cpp
    src/persistence.cpp
    src/linear_scanner.cpp
    src/engine_api.cpp
)

# Shared library for Python ctypes / external bindings
add_library(indexforge SHARED ${CORE_SOURCES})
set_target_properties(indexforge PROPERTIES 
    OUTPUT_NAME "indexforge"
    PREFIX ""
)
if (NOT MSVC)
    target_link_options(indexforge PRIVATE -static)
endif()

# Static library for tests and benchmarks
add_library(indexforge_static STATIC ${CORE_SOURCES})
target_compile_definitions(indexforge_static PUBLIC INDEXFORGE_STATIC)

# Command-Line Interface executable
add_executable(indexforge_cli src/main_cli.cpp)
target_link_libraries(indexforge_cli PRIVATE indexforge_static)

# Unit & Integration Tests executable
add_executable(indexforge_tests
    tests/run_tests.cpp
    tests/test_tokenizer.cpp
    tests/test_stemmer.cpp
    tests/test_inverted_index.cpp
    tests/test_compression.cpp
    tests/test_ranking.cpp
    tests/test_query_parser.cpp
    tests/test_persistence.cpp
    tests/test_concurrency.cpp
)
target_link_libraries(indexforge_tests PRIVATE indexforge_static)

# Benchmarks executable
add_executable(indexforge_benchmark benchmarks/benchmark_core.cpp)
target_link_libraries(indexforge_benchmark PRIVATE indexforge_static)
