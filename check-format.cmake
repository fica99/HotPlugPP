# check-format.cmake - Cross-platform code formatting checker
# Usage: cmake -P check-format.cmake

# Find clang-format executable
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR "clang-format not found. Please install clang-format.")
endif()

message(STATUS "Checking C++ code formatting...")

# Get the source directory (where this script is located)
get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

# Find all C++ source files
file(GLOB_RECURSE ALL_SOURCE_FILES
    "${SOURCE_DIR}/src/*.cpp"
    "${SOURCE_DIR}/src/*.hpp"
    "${SOURCE_DIR}/src/*.h"
    "${SOURCE_DIR}/include/*.cpp"
    "${SOURCE_DIR}/include/*.hpp"
    "${SOURCE_DIR}/include/*.h"
    "${SOURCE_DIR}/examples/*.cpp"
    "${SOURCE_DIR}/examples/*.hpp"
    "${SOURCE_DIR}/examples/*.h"
)

# Check formatting for each file
set(NEEDS_FORMATTING FALSE)
foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
    execute_process(
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${SOURCE_FILE}
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
    message(STATUS "")
    message(FATAL_ERROR "Some files need formatting. Run 'cmake -P format-code.cmake' to fix.")
else()
    message(STATUS "All files are properly formatted!")
endif()
