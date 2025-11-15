# check-format.cmake - Cross-platform code formatting checker
# Usage: cmake -P check-format.cmake

# Find clang-format executable
if(WIN32)
    # Try standard locations first
    find_program(CLANG_FORMAT_EXECUTABLE 
        NAMES clang-format clang-format.exe
        PATHS
            "C:/Program Files/LLVM/bin"
            "C:/Program Files (x86)/LLVM/bin"
            "$ENV{ProgramFiles}/LLVM/bin"
            "$ENV{LOCALAPPDATA}/Programs/LLVM/bin"
    )
    
    # If not found, search in Visual Studio installations (prioritize x64)
    if(NOT CLANG_FORMAT_EXECUTABLE)
        set(VS_BASE "$ENV{ProgramFiles}/Microsoft Visual Studio")
        # Search for x64 versions first
        file(GLOB VS_X64_PATHS "${VS_BASE}/*/*/VC/Tools/Llvm/x64*/bin/clang-format.exe"
                               "${VS_BASE}/*/*/VC/Tools/Llvm/x64/bin/clang-format.exe")
        if(VS_X64_PATHS)
            list(GET VS_X64_PATHS 0 CLANG_FORMAT_EXECUTABLE)
        endif()
    endif()
else()
    find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
endif()

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR 
        "clang-format not found. Please install clang-format.\n"
        "Windows: Install LLVM or add 'C++ Clang Tools for Windows' in Visual Studio Installer\n"
        "Linux: sudo apt-get install clang-format\n"
        "macOS: brew install clang-format"
    )
endif()

message(STATUS "Checking C++ code formatting...")

# Get source directory (parent of scripts directory) and find all C++ files
get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB_RECURSE ALL_SOURCE_FILES
    "${SOURCE_DIR}/src/*.cpp" "${SOURCE_DIR}/src/*.hpp" "${SOURCE_DIR}/src/*.h"
    "${SOURCE_DIR}/include/*.cpp" "${SOURCE_DIR}/include/*.hpp" "${SOURCE_DIR}/include/*.h"
    "${SOURCE_DIR}/examples/*.cpp" "${SOURCE_DIR}/examples/*.hpp" "${SOURCE_DIR}/examples/*.h"
)

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
    message(FATAL_ERROR "Some files need formatting. Run 'cmake -P format-code.cmake' to fix.")
else()
    message(STATUS "All files are properly formatted!")
endif()
