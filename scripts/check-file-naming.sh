#!/bin/bash
#
# Script to validate file naming conventions
# All .cpp and .hpp files must follow snake_case naming convention
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

# Pattern for valid snake_case file names (lowercase letters, numbers, underscores)
SNAKE_CASE_PATTERN='^[a-z][a-z0-9_]*\.(cpp|hpp)$'

error_count=0

echo "Checking file naming conventions..."
echo "Looking in: $ROOT_DIR"
echo ""

# Find all .cpp and .hpp files, excluding build directories
while IFS= read -r -d '' file; do
    # Get just the filename without path
    filename=$(basename "$file")
    
    # Check if filename matches snake_case pattern
    if ! [[ "$filename" =~ $SNAKE_CASE_PATTERN ]]; then
        echo -e "${RED}ERROR${NC}: '$file' does not follow snake_case naming convention"
        echo "       Expected format: lowercase_with_underscores.cpp or .hpp"
        error_count=$((error_count + 1))
    fi
done < <(find "$ROOT_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) \
    -not -path "*/build/*" \
    -not -path "*/.git/*" \
    -not -path "*/_codeql*" \
    -print0)

echo ""
if [ $error_count -eq 0 ]; then
    echo -e "${GREEN}SUCCESS${NC}: All files follow snake_case naming convention"
    exit 0
else
    echo -e "${RED}FAILED${NC}: $error_count file(s) have incorrect naming"
    exit 1
fi
