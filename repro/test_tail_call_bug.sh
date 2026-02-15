#!/bin/bash
# Test script for GCC m68k tail call optimization bug
# This script compiles the test cases and analyzes the assembly output

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Compiler (adjust if needed)
CC=${M68K_CC:-m68k-amigaos-gcc}

echo "========================================"
echo "GCC m68k Tail Call Bug Test Script"
echo "========================================"
echo ""

# Check if compiler exists
if ! command -v $CC &> /dev/null; then
    echo -e "${RED}ERROR: Compiler '$CC' not found${NC}"
    echo "Set M68K_CC environment variable to your m68k GCC compiler"
    exit 1
fi

echo "Using compiler: $CC"
$CC --version | head -n 1
echo ""

# Test 1: Minimal test case
echo "========================================"
echo "Test 1: Minimal Test Case"
echo "========================================"

echo "Compiling gcc_bug_minimal.c (with optimization - BROKEN)..."
$CC -O2 -S gcc_bug_minimal.c -o broken_minimal.s

echo "Compiling gcc_bug_minimal.c (with fix - WORKING)..."
$CC -O2 -fno-optimize-sibling-calls -S gcc_bug_minimal.c -o working_minimal.s

# Check for the bug pattern
echo ""
echo "Analyzing assembly output..."

# Count indirect jumps in broken version
BROKEN_JUMPS=$(grep -c "jmp\s*(a[0-9])" broken_minimal.s || true)
# Count indirect jumps in working version  
WORKING_JUMPS=$(grep -c "jmp\s*(a[0-9])" working_minimal.s || true)

echo "Broken version: $BROKEN_JUMPS indirect jmp instruction(s)"
echo "Working version: $WORKING_JUMPS indirect jmp instruction(s)"

if [ $BROKEN_JUMPS -gt 0 ]; then
    echo -e "${RED}✗ BUG CONFIRMED: Found $BROKEN_JUMPS indirect jmp(s) in broken version${NC}"
    echo ""
    echo "Buggy assembly in 'wrapper' function:"
    grep -A 10 "^wrapper:" broken_minimal.s | head -n 15
else
    echo -e "${YELLOW}⚠ BUG NOT TRIGGERED: No indirect jumps found${NC}"
    echo "This might mean:"
    echo "  1. The compiler is too smart and optimized everything away"
    echo "  2. Your GCC version doesn't have this bug (good!)"
    echo "  3. The test needs adjustment for your compiler version"
fi

if [ $WORKING_JUMPS -gt 0 ]; then
    echo -e "${YELLOW}⚠ WARNING: Working version also has indirect jumps${NC}"
else
    echo -e "${GREEN}✓ GOOD: Working version has no indirect jumps${NC}"
fi

echo ""

# Test 2: Full demonstration
echo "========================================"
echo "Test 2: Full Demonstration"
echo "========================================"

echo "Compiling gcc_m68k_tailcall_bug_demo.c (broken)..."
$CC -O2 gcc_m68k_tailcall_bug_demo.c -o demo_broken 2>&1 | grep -v "warning:" || true

echo "Compiling gcc_m68k_tailcall_bug_demo.c (working)..."
$CC -O2 -fno-optimize-sibling-calls gcc_m68k_tailcall_bug_demo.c -o demo_working 2>&1 | grep -v "warning:" || true

echo "Compiled executables:"
ls -lh demo_broken demo_working 2>/dev/null || echo "  (executables created)"

echo ""
echo "To test on Amiga/emulator, run:"
echo "  ./demo_broken    (should show stack leak)"
echo "  ./demo_working   (should show no leak)"

echo ""

# Test 3: Assembly analysis
echo "========================================"
echo "Test 3: Assembly Pattern Analysis"
echo "========================================"

echo "Searching for the specific bug pattern..."
echo ""

if grep -q "jmp\s*(a0)" broken_minimal.s; then
    echo -e "${RED}Found: jmp (a0) in broken version${NC}"
    echo "Context:"
    grep -B 3 -A 2 "jmp\s*(a0)" broken_minimal.s | head -n 10
    echo ""
fi

if grep -q "jsr\s*(a0)" working_minimal.s; then
    echo -e "${GREEN}Found: jsr (a0) in working version${NC}"
    echo "Context:"
    grep -B 3 -A 3 "jsr\s*(a0)" working_minimal.s | head -n 10
    echo ""
fi

# Look for stack cleanup in working version
if grep -q "addq.l\s*#4,sp" working_minimal.s; then
    echo -e "${GREEN}Found: Stack cleanup (addq.l #4,sp) in working version${NC}"
else
    echo -e "${YELLOW}Note: Stack cleanup might use different instruction (lea N(sp),sp)${NC}"
fi

echo ""

# Summary
echo "========================================"
echo "Summary"
echo "========================================"
echo ""

if [ $BROKEN_JUMPS -gt 0 ] && [ $WORKING_JUMPS -eq 0 ]; then
    echo -e "${RED}✗ BUG CONFIRMED${NC}"
    echo "  The broken version has tail-call-optimized indirect calls"
    echo "  The working version correctly uses jsr with stack cleanup"
    echo ""
    echo "Files to submit to GCC:"
    echo "  - gcc_bug_minimal.c"
    echo "  - broken_minimal.s"
    echo "  - working_minimal.s"
    echo "  - GCC_BUG_REPORT.md"
    echo ""
    echo "Workaround: Add to your CMakeLists.txt:"
    echo '  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-optimize-sibling-calls")'
elif [ $BROKEN_JUMPS -eq 0 ] && [ $WORKING_JUMPS -eq 0 ]; then
    echo -e "${GREEN}✓ NO BUG DETECTED${NC}"
    echo "  Either:"
    echo "  1. Your GCC version doesn't have this bug (excellent!)"
    echo "  2. The test case doesn't trigger it on your compiler"
    echo "  3. Your optimization settings are different"
    echo ""
    echo "Verify by checking your actual project assembly:"
    echo '  grep "jmp\s*(a[0-9])" build/unknown/smite.s'
else
    echo -e "${YELLOW}⚠ UNCLEAR RESULTS${NC}"
    echo "  Both versions have indirect jumps, or unexpected pattern"
    echo "  Manual inspection of assembly required"
fi

echo ""
echo "Generated files:"
echo "  broken_minimal.s    - Assembly with bug"
echo "  working_minimal.s   - Assembly with fix"
echo "  demo_broken         - Executable test (broken)"
echo "  demo_working        - Executable test (working)"
echo ""
echo "Next steps:"
echo "  1. Review the assembly files"
echo "  2. Test executables on Amiga/emulator"
echo "  3. If bug confirmed, submit to GCC with GCC_BUG_REPORT.md"
echo ""

