# Compiler Bug Analysis

## Is This a Compiler Bug? YES.

## The Evidence

### What Tail Call Optimization SHOULD Do

When the compiler optimizes a tail call, it should:
1. ✅ Clean up the current function's local variables
2. ✅ Adjust stack arguments if needed
3. ✅ Jump to the target function
4. ✅ **Ensure the callee's return goes to the original caller**

### What the Compiler ACTUALLY Does (BROKEN)

**smite.s (BROKEN with tail call optimization):**
```asm
mt_e_cmds:
    ... setup code ...
    1758:  2f42 0008       move.l d2,8(sp)      # Overwrite arg 1
    175c:  2270 1db0       movea.l (a0,d0.l:4),a1  # Load function pointer
    1764:  241f            move.l (sp)+,d2     # Restore d2
    1766:  4ed1            jmp (a1)             # ❌ TAIL CALL - NO STACK CLEANUP!
```

**Problem:**
- The function receives 3 arguments (12 bytes) on the stack
- It does a tail call `jmp (a1)` 
- **The 12 bytes of arguments are NEVER cleaned up!**
- Each call leaks 12 bytes of stack space

### What the Compiler DOES When TCO is Disabled (WORKING)

**smite_works.s (WORKING with -fno-optimize-sibling-calls):**
```asm
blocked_e_cmds:
    ... setup code ...
    1908:  2f02            move.l d2,-(sp)      # Push arg 1
    190a:  2070 1db0       movea.l (a0,d0.l:4),a0  # Load function pointer
    1912:  4e90            jsr (a0)             # ✅ NORMAL CALL
    1914:  4fef 000c       lea 12(sp),sp       # ✅ CLEANUP: Remove 12 bytes
    1918:  241f            move.l (sp)+,d2      # Restore d2
    191a:  4e75            rts                  # ✅ RETURN
```

**Correct Behavior:**
- Pushes arguments
- Calls function with `jsr`
- **Cleans up arguments** with `lea 12(sp),sp`
- Returns with `rts`

## Why This Is a Compiler Bug

### 1. Stack Leak

The tail call optimization is **supposed to be safe** - it should not leak stack space. But in this case:
- Arguments are pushed by the caller
- The function does a tail call
- **The arguments are never cleaned up**
- Stack space leaks with every call

### 2. Incorrect Stack Frame Reuse

The compiler tries to reuse the caller's stack frame by:
- Overwriting argument 1 at `8(sp)` 
- Jumping directly to the target function

But this assumes:
- The target function will return to the original caller
- The original caller will clean up the arguments

**This is wrong!** The target function returns to `mt_e_cmds`'s caller, but `mt_e_cmds` never cleaned up its own arguments, so the stack is corrupted.

### 3. Indirect Function Calls

The bug is specifically triggered by:
- **Indirect function calls** through function pointers (`ecmd_tab[]`, `fx_tab[]`, etc.)
- **Multiple arguments** (3 arguments = 12 bytes)
- **Tail call optimization enabled**

The compiler cannot statically determine the target function, so it generates a generic tail call that doesn't properly handle stack cleanup.

## Is This a Compiler Bug?

**Based on our analysis of the evidence, YES - this appears to be a bug in GCC's tail call optimization.**

**What we observe:**
1. Functions are called through function pointers (indirect calls)
2. Multiple arguments are passed on the stack
3. The compiler cannot statically determine the callee
4. Tail call optimization generates `jmp (aX)` instead of proper `jsr` + cleanup
5. Stack arguments are never cleaned up, causing stack leaks

**Evidence:**
- smite.s (broken): Has `jmp (a1)` tail calls that leak stack
- smite_works.s (working): Has NO `jmp (aX)` - all use `jsr` + `lea 12(sp),sp` + `rts`
- Disabling TCO (`-fno-optimize-sibling-calls`) fixes the issue
- The generated code clearly leaks stack space

**Note:** While GCC Bug #47533 mentions stack corruption with `-Os` on m68k, that was a different issue. Our conclusion is based on direct analysis of the generated assembly code, not on that bug report.

**GCC Internals Documentation** notes that tail call optimization has limitations on certain architectures and that indirect calls can be problematic, but doesn't explicitly document this specific bug.

## The Fix

**Disable tail call optimization globally:**
```cmake
target_compile_options(${SMITE_EXECUTABLE} PRIVATE -fno-optimize-sibling-calls)
target_compile_options(ace PRIVATE -fno-optimize-sibling-calls)
```

This forces the compiler to generate proper stack cleanup code instead of broken tail calls.

## Conclusion

**YES, this is a compiler bug.** The tail call optimization is generating incorrect code that leaks stack space. The optimization is supposed to be safe, but it fails for indirect function calls with multiple stack arguments.

