# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#

add_executable(testStr test_str.cpp)

target_link_libraries(testStr simstr::simstr GTest::gtest_main)

add_test(NAME testStr COMMAND testStr)

if (EMSCRIPTEN)
    set_target_properties (testStr PROPERTIES SUFFIX .html)
    target_compile_options(testStr PUBLIC -Wno-warn-absolute-paths -Wno-unknown-warning-option -msimd128 -msse4.2 -msse3)
    if(SIMSTR_EMSCRIPTEN_MT)
        target_compile_options(testStr PUBLIC -pthread)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency-1")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sENVIRONMENT=web,worker")
    endif()
endif(EMSCRIPTEN)
