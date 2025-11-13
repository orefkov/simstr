# CMakeList.txt : CMake project for core_as, include source and define
# project specific logic here.
#

add_executable(testStr test_str.cpp)

if (NOT SIMSTR_IN_SHARED)
    target_link_libraries(testStr simstr::simstr GTest::gtest_main)
else()
    # Пример создания экзешника, если simstr экспортируются из dll.
    # Так как мы подключаем только simstr_dll, и входящие в неё либы не видны
    # нужно вручную задать каталоги include и стандарт C++20
    target_link_libraries(testStr simstr_dll GTest::gtest_main)
    get_target_property(SIMSTR_INCLUDES simstr::simstr INCLUDE_DIRECTORIES)
    target_include_directories(testStr PRIVATE ${SIMSTR_INCLUDES})
    target_compile_features(testStr PUBLIC cxx_std_20)
endif()

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
