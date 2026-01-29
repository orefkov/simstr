# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#

add_executable(test_str test_str.cpp)
add_executable(test_expr_only test_expr_only.cpp test_tostrexpr.cpp)

target_link_libraries(test_str simstr::simstr GTest::gtest_main)
target_link_libraries(test_expr_only GTest::gtest_main)
target_compile_features(test_str PUBLIC cxx_std_23)
target_compile_features(test_expr_only PUBLIC cxx_std_23)

add_test(NAME test_str COMMAND test_str)
add_test(NAME test_expr_only COMMAND test_expr_only)

if (EMSCRIPTEN)
    set_target_properties (test_str PROPERTIES SUFFIX .html)
    target_compile_options(test_str PUBLIC -Wno-warn-absolute-paths -Wno-unknown-warning-option -msimd128 -msse4.2 -msse3)
    set_target_properties (test_expr_only PROPERTIES SUFFIX .html)
    target_compile_options(test_expr_only PUBLIC -Wno-warn-absolute-paths -Wno-unknown-warning-option -msimd128 -msse4.2 -msse3)
    if(SIMSTR_EMSCRIPTEN_MT)
        target_compile_options(test_str PUBLIC -pthread)
        target_compile_options(test_expr_only PUBLIC -pthread)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency-1")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sENVIRONMENT=web,worker")
    endif()
endif(EMSCRIPTEN)
