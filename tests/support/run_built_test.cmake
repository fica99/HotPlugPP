if(DEFINED TEST_EXECUTABLE AND NOT DEFINED TEST_BINARY)
    set(TEST_BINARY "${TEST_EXECUTABLE}")
endif()

if(NOT DEFINED BUILD_DIR OR NOT DEFINED TEST_TARGET OR NOT DEFINED TEST_BINARY)
    message(FATAL_ERROR "BUILD_DIR, TEST_TARGET, and TEST_BINARY/TEST_EXECUTABLE are required.")
endif()

if(DEFINED TEST_CONFIG AND NOT TEST_CONFIG STREQUAL "")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config "${TEST_CONFIG}" --target
                "${TEST_TARGET}"
        RESULT_VARIABLE build_result
    )
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --target "${TEST_TARGET}"
        RESULT_VARIABLE build_result
    )
endif()

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build test target: ${TEST_TARGET}")
endif()

set(test_command "${TEST_BINARY}")

if(DEFINED TEST_WORKING_DIR AND NOT TEST_WORKING_DIR STREQUAL "")
    execute_process(
        COMMAND ${test_command}
        WORKING_DIRECTORY "${TEST_WORKING_DIR}"
        RESULT_VARIABLE test_result
    )
else()
    execute_process(
        COMMAND ${test_command}
        RESULT_VARIABLE test_result
    )
endif()

if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "Test executable failed: ${TEST_BINARY}")
endif()
