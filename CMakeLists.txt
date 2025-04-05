cmake_minimum_required (VERSION 3.20)
include(CMakeDependentOption)

project ("simstr")
set (CMAKE_CXX_STANDARD 20)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set (MSVC_COMPILER ON)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /Zc:strictStrings")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set (CLANG_COMPILER ON)
endif ()

option(SIMSTR_BUILD_TESTS "Построить тесты" ON)
option(SIMSTR_BENCHMARKS "Построить замеры производительности" Off)
option(SIMSTR_AS_SHARED "Функции simstr должны экспортироваться или импортироваться" OFF)
cmake_dependent_option(SIMSTR_EXPORT "Функции simstr должны экспортироваться" ON "SIMSTR_AS_SHARED" OFF)

set (SIMDUTF_TOOLS OFF)
set (SIMDUTF_TESTS OFF)
add_subdirectory(ThirdParty/simdutf)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(simstr
    src/sstring.cpp
    src/simple_unicode.cpp
)

target_link_libraries(simstr PUBLIC simdutf)

if (SIMSTR_AS_SHARED)
    add_compile_definitions(SIMSTR_AS_SHARED)
    if (SIMSTR_EXPORT)
        add_compile_definitions(SIMSTR_EXPORT)
    endif(SIMSTR_EXPORT)
endif(SIMSTR_AS_SHARED)

if (SIMSTR_BUILD_TESTS)
    enable_testing()
    set (BUILD_TESTS ON)
    # Load and build GTest
    include(FetchContent)
    FetchContent_Declare(
      googletest
      # Specify the commit you depend on and update it regularly.
      URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt FALSE CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

function (GBencmark)
    # Load and build Google benchmarks
    include(FetchContent)
    FetchContent_Declare(
      googlebench
      # Specify the commit you depend on and update it regularly.
      URL https://github.com/google/benchmark/archive/refs/tags/v1.7.0.zip
    )
    set (CMAKE_CXX_STANDARD 20)
    set (BENCHMARK_ENABLE_TESTING OFF)
    set (BENCHMARK_ENABLE_LTO OFF)
    set (BENCHMARK_ENABLE_INSTALL OFF)
    set (BENCHMARK_INSTALL_DOCS OFF)
    set (BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
    set (BENCHMARK_ENABLE_GTEST_TESTS OFF)
    add_compile_definitions(BENCHMARK_STATIC_DEFINE)
    FetchContent_MakeAvailable(googlebench)
endfunction()

if (SIMSTR_BENCHMARKS)# AND CMAKE_BUILD_TYPE STREQUAL Release)
    set (BUILD_BENCHMARKS ON)
    GBencmark()
    add_subdirectory(bench)
endif ()
