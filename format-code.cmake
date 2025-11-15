# format-code.cmake - Cross-platform code formatting script
# Usage: cmake -P format-code.cmake

# Find clang-format executable
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR "clang-format not found. Please install clang-format.")
endif()

message(STATUS "Formatting C++ code with clang-format...")

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

# Format each file
foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
    execute_process(
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${SOURCE_FILE}
        RESULT_VARIABLE FORMAT_RESULT
    )
    if(NOT FORMAT_RESULT EQUAL 0)
        message(WARNING "Failed to format: ${SOURCE_FILE}")
    endif()
endforeach()

message(STATUS "Code formatting complete!")
