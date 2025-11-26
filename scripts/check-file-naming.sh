#!/bin/bash
#
# Script to validate file naming conventions
# All .cpp, .hpp, and .h files must follow either PascalCase or snake_case naming convention
#

set -e
set -u
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

error_count=0

echo "Checking file naming conventions..."
echo "Looking in: $ROOT_DIR"
echo ""

# Find all .cpp and .hpp files, excluding build directories
while IFS= read -r -d '' file; do
    # Get just the filename without path
    filename=$(basename "$file")
    
    # Extract name without extension
    name="${filename%.*}"
    
    # Check if filename follows valid naming conventions:
    # 1. PascalCase: starts with uppercase, alphanumeric only (e.g., PluginLoader, IPlugin)
    # 2. snake_case: lowercase with underscores (e.g., host_app, main_entry)
    
    is_pascal_case=false
    is_snake_case=false
    
    if [[ "$name" =~ ^[A-Z][a-zA-Z0-9]*$ ]]; then
        is_pascal_case=true
    fi
    
    if [[ "$name" =~ ^[a-z]([a-z0-9_]*[a-z0-9])?$ ]]; then
        is_snake_case=true
    fi
    
    if [[ "$is_pascal_case" == false && "$is_snake_case" == false ]]; then
        echo -e "${RED}ERROR${NC}: '$file' does not follow naming conventions"
        echo "       Expected: PascalCase (e.g., PluginLoader.cpp) or snake_case (e.g., host_app.cpp)"
        error_count=$((error_count + 1))
    fi
done < <(find "$ROOT_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
    -not -path "*/build/*" \
    -not -path "*/.git/*" \
    -not -path "*/_codeql*" \
    -print0)

echo ""
if [ $error_count -eq 0 ]; then
    echo -e "${GREEN}SUCCESS${NC}: All files follow the naming conventions (PascalCase or snake_case)"
    exit 0
else
    echo -e "${RED}FAILED${NC}: $error_count file(s) have incorrect naming"
    echo "C++ files should use either:"
    echo "  - PascalCase for class files (e.g., PluginLoader.cpp, IPlugin.hpp)"
    echo "  - snake_case for main/utility files (e.g., host_app.cpp)"
    exit 1
fi
