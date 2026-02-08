# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

add_executable(benchStr bench_str.cpp bench.h)
target_link_libraries(benchStr simstr::simstr benchmark::benchmark)
target_compile_features(benchStr PUBLIC cxx_std_23)
target_compile_definitions(benchStr PRIVATE SIMSTR_VERSION="${PROJECT_VERSION}")
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        tags/12.1.0
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(benchStr fmt::fmt)

add_executable(process_result process_result.cpp)
target_link_libraries(process_result simstr_simstr)
if (EMSCRIPTEN)
    #target_link_options(benchStr PRIVATE -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=128MB -sALLOW_MEMORY_GROWTH=0)
    set_target_properties (benchStr PROPERTIES SUFFIX .html)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sASYNCIFY --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/emscripten/shell_minimal.html")
    if(SIMSTR_EMSCRIPTEN_MT)
        target_link_options(benchStr PUBLIC -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency-1 -sENVIRONMENT=web,worker)
    endif()
endif(EMSCRIPTEN)
