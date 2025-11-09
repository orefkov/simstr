# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#

add_executable(testStr test_str.cpp)
target_link_libraries(testStr simstr_simstr gtest_main)
add_test(NAME testStr COMMAND testStr)

if (EMSCRIPTEN)
    set_target_properties (testStr PROPERTIES SUFFIX .html)
endif(EMSCRIPTEN)
