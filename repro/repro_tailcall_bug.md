# Tail Call Optimization Bug - Minimal Reproduction

## Overview

This is a minimal reproduction case demonstrating the GCC tail call optimization bug on m68k that causes stack leaks with indirect function calls.

## Files

- `repro_tailcall_bug.c` - Minimal C code demonstrating the bug
- `repro_tailcall_bug.sh` - Build script to test both versions
- `repro_tailcall_bug.md` - This file

## The Bug

When tail call optimization is enabled, functions that call other functions through function pointers with multiple stack arguments generate incorrect code:

**With TCO (BROKEN):**
```asm
dispatcher:
    ...
    jmp (a1)              # Tail call - NO STACK CLEANUP!
                          # 12 bytes of arguments leak!
```

**Without TCO (CORRECT):**
```asm
dispatcher:
    ...
    jsr (a1)              # Normal call
    lea 12(sp),sp         # Clean up 12 bytes of arguments
    rts                   # Return
```

## How to Reproduce

### 1. Build with TCO enabled (shows the bug):
```bash
m68k-amigaos-gcc -m68020 -O2 -o repro_buggy repro_tailcall_bug.c
```

### 2. Build with TCO disabled (works correctly):
```bash
m68k-amigaos-gcc -m68020 -O2 -fno-optimize-sibling-calls -o repro_fixed repro_tailcall_bug.c
```

### 3. Generate assembly for comparison:
```bash
m68k-amigaos-gcc -m68020 -O2 -S -o repro_buggy.s repro_tailcall_bug.c
m68k-amigaos-gcc -m68020 -O2 -fno-optimize-sibling-calls -S -o repro_fixed.s repro_tailcall_bug.c
```

### 4. Check for tail calls:
```bash
grep "jmp.*(a" repro_buggy.s    # Should find jmp instructions
grep "jmp.*(a" repro_fixed.s    # Should find nothing
```

## Expected Behavior

**With TCO enabled:**
- Assembly contains `jmp (a1)` or similar tail calls
- Each call to `dispatcher()` leaks 12 bytes of stack
- After many calls, stack grows and may corrupt memory
- Program may crash or show stack corruption

**With TCO disabled:**
- Assembly contains `jsr (a1)` + `lea 12(sp),sp` + `rts`
- Stack is properly cleaned up after each call
- No stack leak, program runs correctly

## Key Code Pattern

The bug is triggered by this pattern:
```c
static void dispatcher(uint32_t index, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    func_table[index](arg1, arg2, arg3);  // Tail call through function pointer
}
```

**Requirements for the bug:**
1. Function calls through function pointer (indirect call)
2. Multiple arguments passed on stack (3+ arguments)
3. Tail call optimization enabled
4. Compiler cannot statically determine callee

## Sharing

This minimal reproduction can be shared to:
- Report the bug to GCC developers
- Test if newer GCC versions have the bug
- Demonstrate the issue to others
- Verify fixes

## Integration with ACE

If you want to integrate this into the ACE test suite, you could:
1. Add it to a test directory
2. Create a CMake target for it
3. Run it as part of CI/CD to detect the bug

