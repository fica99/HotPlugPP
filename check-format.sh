#!/bin/bash
# check-format.sh - Check if C++ code follows the style guidelines

set -e

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "Checking C++ code formatting..."

# Find all C++ files
files=$(find . \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
    -not -path "*/build/*" \
    -not -path "*/.git/*" \
    -not -path "*/cmake-build-*/*")

# Check if any files need formatting
needs_formatting=0
for file in $files; do
    if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
        echo "❌ File needs formatting: $file"
        needs_formatting=1
    fi
done

if [ $needs_formatting -eq 0 ]; then
    echo "✅ All files are properly formatted!"
    exit 0
else
    echo ""
    echo "❌ Some files need formatting. Run './format-code.sh' to fix."
    exit 1
fi
