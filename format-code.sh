#!/bin/bash
# format-code.sh - Format all C++ source files in the project

set -e

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "Formatting C++ code with clang-format..."

# Find and format all C++ files
find . \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
    -not -path "*/build/*" \
    -not -path "*/.git/*" \
    -not -path "*/cmake-build-*/*" \
    -not -path "*/_codeql_build_dir/*" \
    -exec clang-format -i {} +

echo "Code formatting complete!"
