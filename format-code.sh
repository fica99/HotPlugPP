#!/bin/bash
# format-code.sh - Format all C++ source files in the project
# This is a cross-platform wrapper that calls the CMake script

set -e

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Run the CMake script
cmake -P format-code.cmake
