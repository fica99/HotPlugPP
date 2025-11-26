# find-source-files.cmake - Find all C++ source files in the project
# This file is included by format-code.cmake and check-format.cmake
# Sets SOURCE_DIR and ALL_SOURCE_FILES variables

# Get source directory (parent of scripts directory) and find all C++ files
get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB_RECURSE ALL_SOURCE_FILES
    "${SOURCE_DIR}/src/*.cpp" "${SOURCE_DIR}/src/*.hpp" "${SOURCE_DIR}/src/*.h"
    "${SOURCE_DIR}/include/*.cpp" "${SOURCE_DIR}/include/*.hpp" "${SOURCE_DIR}/include/*.h"
    "${SOURCE_DIR}/examples/*.cpp" "${SOURCE_DIR}/examples/*.hpp" "${SOURCE_DIR}/examples/*.h"
)
