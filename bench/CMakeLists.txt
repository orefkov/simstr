# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

add_executable(benchStr bench_str.cpp bench.h)
target_link_libraries(benchStr simstr::simstr benchmark::benchmark benchmark::benchmark_main)

add_executable(process_result process_result.cpp)
target_link_libraries(process_result simstr_simstr)

if (EMSCRIPTEN)
    #target_link_options(benchStr PRIVATE -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=128MB -sALLOW_MEMORY_GROWTH=0)
    set_target_properties (benchStr PROPERTIES SUFFIX .html)
    target_link_options(benchStr PUBLIC --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/bench.js)
endif(EMSCRIPTEN)
