# format-code.cmake - Cross-platform code formatting script
# Usage: cmake -P format-code.cmake

# Find clang-format executable
include("${CMAKE_CURRENT_LIST_DIR}/find-clang-format.cmake")

message(STATUS "Formatting C++ code...")

# Find all C++ source files
include("${CMAKE_CURRENT_LIST_DIR}/find-source-files.cmake")

# Format each file
foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
    execute_process(
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i -style=file "${SOURCE_FILE}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE FORMAT_RESULT
        ERROR_QUIET
        OUTPUT_QUIET
    )
    if(FORMAT_RESULT EQUAL 0)
        message(STATUS "Formatted: ${SOURCE_FILE}")
    else()
        message(WARNING "Failed to format: ${SOURCE_FILE}")
    endif()
endforeach()

message(STATUS "Code formatting complete!")
