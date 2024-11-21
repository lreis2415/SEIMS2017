##############
# Unit Tests
# See the official site of GoogleTest supported by CMake for detailed information:
#    https://cmake.org/cmake/help/latest/module/GoogleTest.html
##############
CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)
include(GoogleTest)

file(GLOB TEST_SRC_FILES *.cpp utils/*.cpp raster/*.cpp)
IF (MONGOC_FOUND)
   file(GLOB TEST_MONGODB_SRC_FILES db/*.cpp)
   list(APPEND TEST_SRC_FILES ${TEST_MONGODB_SRC_FILES})
ENDIF ()
set(APPNAME unittest)
add_executable(${APPNAME} ${TEST_SRC_FILES})
SET_TARGET_PROPERTIES(${APPNAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
# SET_TARGET_PROPERTIES(${APPNAME} PROPERTIES OUTPUT_NAME ${APPNAME}${CMAKE_BUILD_POSTFIX})

IF (GTEST_FOUND)
    target_link_libraries(${APPNAME} ${TARGET_VISIBILITY}
                          GTest::gtest GTest::gtest_main ${CCGLNAME})
ELSE ()
    target_link_libraries(${APPNAME} ${TARGET_VISIBILITY} gtest gtest_main ${CCGLNAME})
ENDIF ()

IF(HOST)
    set(MONGOHOST ${HOST})
ELSE()
    set(MONGOHOST "127.0.0.1")
ENDIF()
IF(PORT)
    set(MONGOPORT ${PORT})
ELSE()
    set(MONGOPORT "27017")
ENDIF()

IF (XCODE OR MSVC)
    # Xcode failed with gtest_discover_tests()
    # Stay tuned https://github.com/google/googletest/issues/3475#issuecomment-873690957
    # MSVC 2019 on Github Actions also failed.
    # gtest_add_tests(TARGET      ${APPNAME}
    #                 TEST_SUFFIX .noArgs
    #                 TEST_LIST   noArgsTests
    #                )
    set_tests_properties(${ArgsTests} PROPERTIES TIMEOUT 600)
    gtest_add_tests(TARGET      ${APPNAME}
                    EXTRA_ARGS  -host ${MONGOHOST}  -port ${MONGOPORT}
                    TEST_SUFFIX .withArgs
                    TEST_LIST   withArgsTests
                   )
    set_tests_properties(${withArgsTests} PROPERTIES TIMEOUT 600)
ELSE ()
    gtest_discover_tests(${APPNAME}
                         EXTRA_ARGS  -host ${MONGOHOST}  -port ${MONGOPORT}
                         DISCOVERY_TIMEOUT 600
                        )
ENDIF ()

install(TARGETS ${APPNAME}
        DESTINATION ${INSTALL_DIR}/bin)

if ((CV_GCC OR CV_CLANG) AND CODE_COVERAGE)
    # As an executable target, adds the 'ccov-${APPNAME}' target and instrumentation for generating coverage reports.
    # Note that, the code coverage should excluding the test sources themself and gtest source code.
    target_code_coverage(${APPNAME} EXCLUDE ${TEST_DIR}/* ${googletest_SOURCE_DIR}/* ${googletest_BINARY_DIR}/*)
    # As a library target, adds coverage instrumentation but no targets.
    target_code_coverage(${CCGLNAME})
endif ()

# Previously implementation based on CodeCoverage.cmake by Lars Bilke
# But failed with Clang and llvm-cov. So I switched to code-coverage.cmake
#   written by George Cave.  2021-12-23 LJ.
# if (CODE_COVERAGE)
#     # https://github.com/bilke/cmake-modules/issues/40
#     include(CodeCoverage)
#     append_coverage_compiler_flags()
#     setup_target_for_coverage_lcov(
#         NAME coverage
#         EXECUTABLE ctest -C ${CMAKE_BUILD_TYPE}
#         )
# endif ()
# SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)

if (USE_SAN)
  add_sanitizers(${APPNAME})
endif ()
