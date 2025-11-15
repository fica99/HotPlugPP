# check-format.cmake - Cross-platform code formatting checker
# Usage: cmake -P check-format.cmake

# Find clang-format executable
include("${CMAKE_CURRENT_LIST_DIR}/find-clang-format.cmake")

message(STATUS "Checking C++ code formatting...")

# Find all C++ source files
include("${CMAKE_CURRENT_LIST_DIR}/find-source-files.cmake")

# Check formatting for each file
set(NEEDS_FORMATTING FALSE)
foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
    execute_process(
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror -style=file "${SOURCE_FILE}"
        WORKING_DIRECTORY "${SOURCE_DIR}"
        RESULT_VARIABLE FORMAT_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(NOT FORMAT_RESULT EQUAL 0)
        message(STATUS "File needs formatting: ${SOURCE_FILE}")
        set(NEEDS_FORMATTING TRUE)
    endif()
endforeach()

if(NEEDS_FORMATTING)
    message(FATAL_ERROR "Some files need formatting. Run 'cmake -P scripts/format-code.cmake' to fix.")
else()
    message(STATUS "All files are properly formatted!")
endif()
