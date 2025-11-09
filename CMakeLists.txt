cmake_minimum_required (VERSION 3.20)
include(cmake/prelude.cmake)
include(CMakeDependentOption)
include(FetchContent)

project (
	simstr
    VERSION 1.0.0
    DESCRIPTION "Yet another string library"
    HOMEPAGE_URL "https://snegopat.ru/simstr"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set (MSVC_COMPILER ON)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /Zc:strictStrings")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    set (CLANG_COMPILER ON)
endif ()

option(SIMSTR_BUILD_TESTS "Построить тесты" ON)
option(SIMSTR_BENCHMARKS "Построить замеры производительности" ON)
option(SIMSTR_SHARED "Функции simstr должны экспортироваться или импортироваться" OFF)

if (EMSCRIPTEN)
    option(SIMSTR_EMSCRIPTEN_MT "Использовать многопоточность в Emscripten" OFF)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option -msimd128 -msse4.2 -msse3")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option -msimd128 -msse4.2 -msse3")

    if (SIMSTR_EMSCRIPTEN_MT)
        message("Build MT emscripten")
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency-1")
    	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sENVIRONMENT=web,worker")
    endif()
endif(EMSCRIPTEN)

add_library(simstr_simstr
    src/sstring.cpp
    src/simple_unicode.cpp
)
add_library(simstr::simstr ALIAS simstr_simstr)

target_include_directories(
    simstr_simstr ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(simstr_simstr PUBLIC cxx_std_20)

set_target_properties(
    simstr_simstr PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME simstr
    OUTPUT_NAME simstr
)

function (Simdutf)
	set (SIMDUTF_TOOLS OFF)
	set (SIMDUTF_TESTS OFF)
	set (SIMDUTF_BENCHMARKS OFF)
	FetchContent_Declare(
	  simdutf
	  GIT_REPOSITORY git@github.com:simdutf/simdutf.git
	  GIT_SHALLOW TRUE
	  GIT_TAG tags/v7.5.0
	  FIND_PACKAGE_ARGS
	)
	FetchContent_MakeAvailable(simdutf)
endfunction()

Simdutf()
 
target_link_libraries(simstr_simstr PUBLIC simdutf::simdutf)

if (SIMSTR_SHARED)
    add_compile_definitions(SIMSTR_SHARED)
    target_compile_definitions(simstr_simstr PRIVATE SIMSTR_EXPORT)
endif(SIMSTR_SHARED)

if (SIMSTR_BUILD_TESTS)
    enable_testing()
    set (BUILD_TESTS ON)
    # Load and build GTest
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

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT simstr_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of simstr"
  )
endif()

include(cmake/dev-mode.cmake)
