﻿# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

add_executable(benchStr bench_str.cpp bench.h)
target_link_libraries(benchStr simstr benchmark::benchmark benchmark::benchmark_main)

add_executable(process_result process_result.cpp)
target_link_libraries(process_result simstr)

if (EMSCRIPTEN)
    set_target_properties (benchStr PROPERTIES SUFFIX .html)
endif(EMSCRIPTEN)
