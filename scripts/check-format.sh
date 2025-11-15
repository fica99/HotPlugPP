#!/bin/bash
# check-format.sh - Check if C++ code follows the style guidelines
# This is a cross-platform wrapper that calls the CMake script

set -e

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Run the CMake script
cmake -P check-format.cmake
