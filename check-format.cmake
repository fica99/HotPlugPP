# check-format.cmake - Cross-platform code formatting checker
# Usage: cmake -P check-format.cmake

# Find clang-format executable
# On Windows, also check for clang-format.exe
if(WIN32)
    # Get ProgramFiles path (works for both 32-bit and 64-bit on 64-bit systems)
    set(PROGRAM_FILES_PATH "$ENV{ProgramFiles}")
    
    # First try standard find_program (checks PATH)
    find_program(CLANG_FORMAT_EXECUTABLE 
        NAMES clang-format clang-format.exe
        PATHS
            "C:/Program Files/LLVM/bin"
            "C:/Program Files (x86)/LLVM/bin"
            "${PROGRAM_FILES_PATH}/LLVM/bin"
            "$ENV{LOCALAPPDATA}/Programs/LLVM/bin"
    )
    
    # If not found, check Visual Studio installation paths
    if(NOT CLANG_FORMAT_EXECUTABLE)
        # Check common Visual Studio installation paths
        set(VS_PATHS
            "${PROGRAM_FILES_PATH}/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm"
            "${PROGRAM_FILES_PATH}/Microsoft Visual Studio/2022/Professional/VC/Tools/Llvm"
            "${PROGRAM_FILES_PATH}/Microsoft Visual Studio/2022/Enterprise/VC/Tools/Llvm"
            "${PROGRAM_FILES_PATH}/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm"
            "${PROGRAM_FILES_PATH}/Microsoft Visual Studio/2019/Professional/VC/Tools/Llvm"
            "${PROGRAM_FILES_PATH}/Microsoft Visual Studio/2019/Enterprise/VC/Tools/Llvm"
        )
        
        foreach(VS_PATH ${VS_PATHS})
            if(EXISTS "${VS_PATH}")
                # Prioritize x64 versions, then x86, then ARM64
                # Look for x64_* directories first (e.g., x64_19.1.5)
                file(GLOB LLVM_X64_DIRS "${VS_PATH}/x64_*/bin")
                foreach(LLVM_BIN_DIR ${LLVM_X64_DIRS})
                    if(EXISTS "${LLVM_BIN_DIR}/clang-format.exe")
                        set(CLANG_FORMAT_EXECUTABLE "${LLVM_BIN_DIR}/clang-format.exe")
                        break()
                    endif()
                endforeach()
                
                # Also check for plain "x64" directory
                if(NOT CLANG_FORMAT_EXECUTABLE AND EXISTS "${VS_PATH}/x64/bin")
                    if(EXISTS "${VS_PATH}/x64/bin/clang-format.exe")
                        set(CLANG_FORMAT_EXECUTABLE "${VS_PATH}/x64/bin/clang-format.exe")
                    endif()
                endif()
                
                # If x64 not found, try x86
                if(NOT CLANG_FORMAT_EXECUTABLE)
                    file(GLOB LLVM_X86_DIRS "${VS_PATH}/x86*/bin")
                    foreach(LLVM_BIN_DIR ${LLVM_X86_DIRS})
                        if(EXISTS "${LLVM_BIN_DIR}/clang-format.exe")
                            set(CLANG_FORMAT_EXECUTABLE "${LLVM_BIN_DIR}/clang-format.exe")
                            break()
                        endif()
                    endforeach()
                endif()
                
                # If still not found, try any other directories (ARM64, etc.) as fallback
                if(NOT CLANG_FORMAT_EXECUTABLE)
                    file(GLOB LLVM_BIN_DIRS "${VS_PATH}/*/bin")
                    foreach(LLVM_BIN_DIR ${LLVM_BIN_DIRS})
                        if(EXISTS "${LLVM_BIN_DIR}/clang-format.exe")
                            set(CLANG_FORMAT_EXECUTABLE "${LLVM_BIN_DIR}/clang-format.exe")
                            break()
                        endif()
                    endforeach()
                endif()
                
                if(CLANG_FORMAT_EXECUTABLE)
                    break()
                endif()
            endif()
        endforeach()
    endif()
else()
    find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
endif()

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(FATAL_ERROR 
        "clang-format not found. Please install clang-format.\n"
        "Windows: Install LLVM from https://llvm.org/builds/ or via:\n"
        "  - Chocolatey: choco install llvm\n"
        "  - Scoop: scoop install llvm\n"
        "  - Visual Studio Installer: Add 'C++ Clang Tools for Windows'\n"
        "Linux: sudo apt-get install clang-format (Ubuntu/Debian) or equivalent\n"
        "macOS: brew install clang-format"
    )
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
    # Use -style=file to use .clang-format in the source directory
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
    message(STATUS "")
    message(FATAL_ERROR "Some files need formatting. Run 'cmake -P format-code.cmake' to fix.")
else()
    message(STATUS "All files are properly formatted!")
endif()
