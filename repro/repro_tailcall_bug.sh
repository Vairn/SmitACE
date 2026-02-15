#!/bin/bash
# Build script to test tail call optimization bug

echo "Building with tail call optimization ENABLED (buggy)..."
m68k-amigaos-gcc -m68020 -O2 -o repro_buggy repro_tailcall_bug.c

echo ""
echo "Building with tail call optimization DISABLED (fixed)..."
m68k-amigaos-gcc -m68020 -O2 -fno-optimize-sibling-calls -o repro_fixed repro_tailcall_bug.c

echo ""
echo "Generating assembly for comparison..."
m68k-amigaos-gcc -m68020 -O2 -S -o repro_buggy.s repro_tailcall_bug.c
m68k-amigaos-gcc -m68020 -O2 -fno-optimize-sibling-calls -S -o repro_fixed.s repro_tailcall_bug.c

echo ""
echo "Checking for tail calls (jmp indirect)..."
echo "Buggy version (should have jmp):"
grep -n "jmp.*(a" repro_buggy.s || echo "  No jmp found"

echo ""
echo "Fixed version (should NOT have jmp):"
grep -n "jmp.*(a" repro_fixed.s || echo "  No jmp found (correct)"

echo ""
echo "Done! Compare repro_buggy.s and repro_fixed.s to see the difference."

