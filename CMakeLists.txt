cmake_minimum_required(VERSION 3.10)
project(profanity VERSION 1.0 LANGUAGES CXX)

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please configure using a separate build directory:\n  cmake -S . -B build")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Performance and profiling options
option(ENABLE_PROFILING "Enable profiling support (-pg -fno-omit-frame-pointer)" OFF)
option(ENABLE_HIGH_PERFORMANCE "Enable maximum performance optimizations (-O3 -march=native -flto). WARNING: uses -march=native targeting the build machine CPU; resulting binaries may not be portable. For distributable builds, prefer standard Release mode without this option." OFF)
option(BUILD_TESTING "Build tests with GoogleTest" ON)
option(BUILD_BENCHMARKS "Build benchmarks with GoogleBenchmark" OFF)
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

# Compiler warnings for all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused -Wunused-parameter -Wpedantic")

# Compiler flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(ENABLE_HIGH_PERFORMANCE)
        message(STATUS "Enabling high-performance optimizations")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# Profiling support
if(ENABLE_PROFILING)
    message(STATUS "Enabling profiling support")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

# Code coverage support
if(ENABLE_COVERAGE)
    message(STATUS "Enabling code coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Platform-specific settings
if(APPLE)
    # macOS: Use framework for OpenCL
    find_library(OPENCL_LIBRARY OpenCL)
    if(NOT OPENCL_LIBRARY)
        message(FATAL_ERROR "OpenCL framework not found on macOS")
    endif()
    set(OPENCL_LIBRARIES ${OPENCL_LIBRARY})
    set(OPENCL_INCLUDE_DIRS "")
else()
    # Linux/Other: Use standard OpenCL detection, with fallback to local headers
    find_package(OpenCL)
    if(NOT OpenCL_FOUND)
        # Fallback to local OpenCL headers in third_party
        message(STATUS "System OpenCL not found, using local headers at ./third_party/OpenCL/include")
        set(OPENCL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OpenCL/include")
        # Try to find OpenCL library manually in common locations
        find_library(OPENCL_LIBRARY 
            NAMES OpenCL
            PATHS 
                /usr/lib
                /usr/local/lib
                /usr/lib/x86_64-linux-gnu
                /usr/lib64
                /opt/cuda/lib64
                /opt/AMDAPP/lib/x86_64
                /opt/intel/opencl/lib64
        )
        if(NOT OPENCL_LIBRARY)
            message(WARNING "OpenCL library not found in system. Build will fail at link time if OpenCL is not installed.\n  Please install OpenCL development files:\n    Ubuntu/Debian: sudo apt-get install ocl-icd-opencl-dev\n    NVIDIA GPU: Install CUDA toolkit\n    AMD GPU: Install ROCm or AMDGPU-Pro")
            set(OPENCL_LIBRARIES "OpenCL")
        else()
            set(OPENCL_LIBRARIES ${OPENCL_LIBRARY})
        endif()
    else()
        set(OPENCL_LIBRARIES ${OpenCL_LIBRARIES})
        set(OPENCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIRS})
    endif()
endif()

# Find cURL
find_package(CURL)
if(NOT CURL_FOUND)
    message(WARNING "cURL package not found via find_package, trying manual detection")
    # Try to find curl library manually
    find_library(CURL_LIBRARY 
        NAMES curl libcurl
        PATHS 
            /usr/lib
            /usr/local/lib
            /usr/lib/x86_64-linux-gnu
            /usr/lib64
    )
    find_path(CURL_INCLUDE_DIR
        NAMES curl/curl.h
        PATHS
            /usr/include
            /usr/local/include
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Curl/include
    )
    if(NOT CURL_LIBRARY)
        message(WARNING "cURL library not found. Build will fail at link time if cURL is not installed.\n  Please install cURL development files:\n    Ubuntu/Debian: sudo apt-get install libcurl4-openssl-dev")
        set(CURL_LIBRARIES "curl")
    else()
        set(CURL_LIBRARIES ${CURL_LIBRARY})
    endif()
    if(CURL_INCLUDE_DIR)
        set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
    else()
        # Fallback to third_party
        set(CURL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Curl/include")
    endif()
endif()

# Print detected libraries
message(STATUS "OpenCL include dirs: ${OPENCL_INCLUDE_DIRS}")
message(STATUS "OpenCL libraries: ${OPENCL_LIBRARIES}")
message(STATUS "CURL include dirs: ${CURL_INCLUDE_DIRS}")
message(STATUS "CURL libraries: ${CURL_LIBRARIES}")

# Source files for main executable (from src/)
set(PROFANITY_SOURCES
    src/Dispatcher.cpp
    src/Mode.cpp
    src/precomp.cpp
    src/profanity.cpp
)

# Main executable
add_executable(profanity.x64 ${PROFANITY_SOURCES})

# Include directories for main executable
# Standard CMake layout: include/ for public headers, src/ for private includes
target_include_directories(profanity.x64 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENCL_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# Link libraries for main executable
target_link_libraries(profanity.x64 PRIVATE
    ${OPENCL_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Strip symbols on Linux for Release builds (Makefile uses -s in LDFLAGS)
if(NOT APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(profanity.x64 PROPERTIES LINK_FLAGS_RELEASE "-s")
endif()

# Test executable (src/test/test_address_validity)
# Note: This test does NOT depend on OpenCL
set(TEST_SOURCES
    src/test/test_address_validity.cpp
)

add_executable(test_address_validity ${TEST_SOURCES})

# Test executable includes
target_include_directories(test_address_validity PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Test target for building the test executable
add_custom_target(test_build
    DEPENDS test_address_validity
    COMMENT "Building test executable"
)

# Enable CTest
enable_testing()
add_test(NAME AddressValidityTest COMMAND test_address_validity)

# GoogleTest integration for unit tests
if(BUILD_TESTING)
    message(STATUS "Configuring GoogleTest framework")
    # Note: FetchContent requires network access. For offline builds or environments
    # where external downloads are restricted, install GoogleTest system-wide and
    # it will be found via find_package instead.
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
        TIMEOUT 30
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Unit test for Mode.cpp
    add_executable(test_mode
        src/test/test_mode.cpp
        src/Mode.cpp
    )
    target_include_directories(test_mode PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENCL_INCLUDE_DIRS}
    )
    target_link_libraries(test_mode PRIVATE gtest_main)
    add_test(NAME ModeTest COMMAND test_mode)
    
    # Unit test for Dispatcher.cpp (mock version - without actual GPU)
    add_executable(test_dispatcher
        src/test/test_dispatcher.cpp
    )
    target_include_directories(test_dispatcher PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENCL_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
    )
    target_link_libraries(test_dispatcher PRIVATE gtest_main)
    add_test(NAME DispatcherTest COMMAND test_dispatcher)
    
    # Unit test for precomp.cpp
    add_executable(test_precomp
        src/test/test_precomp.cpp
        src/precomp.cpp
    )
    target_include_directories(test_precomp PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENCL_INCLUDE_DIRS}
    )
    target_link_libraries(test_precomp PRIVATE gtest_main)
    add_test(NAME PrecompTest COMMAND test_precomp)
    
    # Add custom target to run all tests
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS test_mode test_dispatcher test_precomp test_address_validity
        COMMENT "Running all unit tests"
    )
endif()

# GoogleBenchmark integration for performance benchmarks
if(BUILD_BENCHMARKS)
    message(STATUS "Configuring GoogleBenchmark framework")
    # Note: FetchContent requires network access. For offline builds, install
    # GoogleBenchmark system-wide via package manager.
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
    
    # Core algorithm benchmarks
    add_executable(bench_core
        src/benchmark/bench_core.cpp
        src/Mode.cpp
        src/precomp.cpp
    )
    target_include_directories(bench_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENCL_INCLUDE_DIRS}
    )
    target_link_libraries(bench_core PRIVATE benchmark::benchmark)
    
    # Add custom target to run benchmarks
    add_custom_target(benchmark
        COMMAND bench_core
        DEPENDS bench_core
        COMMENT "Running performance benchmarks"
    )
endif()

# Installation rules
install(TARGETS profanity.x64
    RUNTIME DESTINATION bin
)

# Install headers (optional, for library usage)
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Summary
message(STATUS "==============================================")
message(STATUS "Configuration Summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  High Performance: ${ENABLE_HIGH_PERFORMANCE}")
message(STATUS "  Profiling: ${ENABLE_PROFILING}")
message(STATUS "  Code Coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Build Testing: ${BUILD_TESTING}")
message(STATUS "  Build Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "  Project layout:")
message(STATUS "    Source files: src/")
message(STATUS "    Headers: include/")
message(STATUS "    Kernels: kernel/")
message(STATUS "    Third-party: third_party/")
message(STATUS "    Tests: src/test/")
message(STATUS "    Benchmarks: src/benchmark/")
message(STATUS "    Docs: docs/")
message(STATUS "  Main executable: profanity.x64")
message(STATUS "  Test executable: test_address_validity")
message(STATUS "  OpenCL: ${OPENCL_LIBRARIES}")
message(STATUS "  cURL: ${CURL_LIBRARIES}")
message(STATUS "==============================================")
