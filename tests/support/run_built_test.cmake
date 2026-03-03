# run_built_test.cmake — manual test runner utility
#
# This script is NOT used by the default CTest configuration (add_test calls the
# test executable directly). It is kept as a convenience for manual invocations
# where you want to run a specific test binary without going through ctest:
#
#   cmake -DTEST_BINARY=<path/to/hotplugpp_tests>  \
#         [-DTEST_WORKING_DIR=<dir>]               \
#         -P tests/support/run_built_test.cmake
#
# Build the test targets before invoking ctest or this script:
#   cmake --build <build> --target build_hotplugpp_tests
#   ctest --test-dir <build> -C Release --output-on-failure

if(DEFINED TEST_EXECUTABLE AND NOT DEFINED TEST_BINARY)
    set(TEST_BINARY "${TEST_EXECUTABLE}")
endif()

if(NOT DEFINED TEST_BINARY)
    message(FATAL_ERROR "TEST_BINARY (or TEST_EXECUTABLE) is required.")
endif()

if(DEFINED TEST_WORKING_DIR AND NOT TEST_WORKING_DIR STREQUAL "")
    execute_process(
        COMMAND "${TEST_BINARY}"
        WORKING_DIRECTORY "${TEST_WORKING_DIR}"
        RESULT_VARIABLE test_result
    )
else()
    execute_process(
        COMMAND "${TEST_BINARY}"
        RESULT_VARIABLE test_result
    )
endif()

if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "Test executable failed: ${TEST_BINARY}")
endif()
