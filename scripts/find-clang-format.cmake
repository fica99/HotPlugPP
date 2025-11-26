# find-clang-format.cmake - Find clang-format executable
# This file is included by format-code.cmake and check-format.cmake
# Sets CLANG_FORMAT_EXECUTABLE variable

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

message(STATUS "Found clang-format: ${CLANG_FORMAT_EXECUTABLE}")
