cmake_minimum_required(VERSION 3.17)
set (CMAKE_CXX_STANDARD 11)

set(LIBNAME "libh9")
set(TESTNAME "unittests")

set(LIB_MAJOR_VERS "0")
set(LIB_MINOR_VERS "1")
set(LIB_PATCH_VERS "0")

project(${LIBNAME} VERSION ${LIB_MAJOR_VERS}.${LIB_MINOR_VERS}.${LIB_PATCH_VERS})

include_directories(${PROJECT_SOURCE_DIR}/lib)
add_library(libh9 
    lib/h9_sysex.c
    lib/utils.c
    lib/libh9.c)
set_property(TARGET ${LIBNAME} PROPERTY C_STANDARD 11)
set_target_properties(${LIBNAME} PROPERTIES PREFIX "")


project(${TESTNAME})
add_library(${LIBNAME}_coverage
    lib/h9_sysex.c
    lib/utils.c
    lib/libh9.c)
set_property(TARGET ${LIBNAME}_coverage PROPERTY C_STANDARD 11)
set_target_properties(${LIBNAME}_coverage PROPERTIES PREFIX "")

add_executable(${TESTNAME} 
    ${PROJECT_SOURCE_DIR}/test/test_helpers.cpp
    ${PROJECT_SOURCE_DIR}/test/h9_midi_test.cpp
    ${PROJECT_SOURCE_DIR}/test/h9_controls_test.cpp
    ${PROJECT_SOURCE_DIR}/test/h9_preset_test.cpp
    ${PROJECT_SOURCE_DIR}/test/h9_sysex_test.cpp
    ${PROJECT_SOURCE_DIR}/test/utils_test.cpp
    ${PROJECT_SOURCE_DIR}/third_party/googletest/googletest/src/gtest_main.cc)
target_include_directories(${TESTNAME} PRIVATE ${PROJECT_SOURCE_DIR}/test)
target_link_libraries(${TESTNAME} gtest gtest_main ${LIBNAME}_coverage)

set_property(TARGET ${TESTNAME} PROPERTY C_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/third_party/cmake/")

include(CodeCoverage)
append_coverage_compiler_flags(TARGET "${LIBNAME}_coverage")
append_coverage_compiler_flags(TARGET "${TESTNAME}")
setup_target_for_coverage_gcovr_html(
    NAME coverage
    EXECUTABLE ctest -j ${PROCESSOR_COUNT}
    DEPENDENCIES ${TESTNAME}
    BASE_DIRECTORY "./"
    EXCLUDE "third_party/googletest" "test"
)

include(GoogleTest)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/googletest)
gtest_discover_tests(${TESTNAME} WORKING_DIRECTORY ${PROJECT_DIR})

enable_testing()
