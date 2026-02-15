# Tail Call Fix Verification
## Checking if Attributes Are Applied Correctly

**Issue:** Even after adding `__attribute__((optimize("no-optimize-sibling-calls")))` to interrupt handlers and ProTracker functions, the code still crashes.

**Possible Causes:**
1. Attribute syntax might be incorrect
2. Attributes might not be applied (need rebuild)
3. Other functions in the call chain still have tail calls
4. The attribute might not work with function pointers

---

## Functions Updated

### Interrupt Handlers (system.c)
- ✅ `int1Handler` through `int7Handler`
- ✅ `osIntServer`
- ✅ `osIntVector`
- ✅ `ciaIcrHandler`
- ✅ `aceInputHandler`

### ProTracker Functions (ptplayer.c)
- ✅ `mt_e_cmds`
- ✅ `blocked_e_cmds`
- ✅ `checkmorefx`
- ✅ `mt_e8`
- ✅ `moreBlockedFx`

---

## Verification Steps

### 1. Rebuild and Check Assembly

After rebuilding, check the generated assembly:

```bash
# Rebuild the project
cmake --build build/unknown

# Check if tail calls are still present
grep "jmp (a" build/unknown/smite.s | grep -E "(mt_e_cmds|blocked_e_cmds|checkmorefx|mt_e8|moreBlockedFx)"
```

**Expected:** Should show `jsr` instead of `jmp` for these functions.

### 2. Verify Attribute Syntax

The attribute syntax might need to be different. Try:

**Option A (Current):**
```c
__attribute__((optimize("no-optimize-sibling-calls")))
```

**Option B (Alternative):**
```c
__attribute__((optimize("-fno-optimize-sibling-calls")))
```

**Option C (Pragma):**
```c
#pragma GCC optimize("no-optimize-sibling-calls")
```

### 3. Check Function Pointer Calls

The issue might be that function pointers bypass the attribute. The functions themselves might not have tail calls, but the **functions they call through pointers** might.

**Example:**
```c
// mt_e_cmds doesn't have tail call (good)
// But ecmd_tab[ubCmdE]() might have tail calls (bad)
ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
```

**Solution:** We might need to disable tail calls globally, or add attributes to ALL functions in the call chain.

### 4. Check Other Call Sites

Functions might be called from multiple places:
- Direct calls (should respect attributes)
- Function pointer calls (might bypass attributes)
- Inline expansion (might bypass attributes)

---

## Next Steps

1. **Rebuild and verify** - Check if new assembly has `jsr` instead of `jmp`
2. **Check attribute syntax** - Verify GCC supports the format used
3. **Global disable** - Consider `-fno-optimize-sibling-calls` in CMakeLists.txt
4. **Check call chain** - Verify all functions in interrupt call chain are protected

---

## Alternative: Global Disable

If per-function attributes don't work, disable globally:

```cmake
# In CMakeLists.txt
target_compile_options(${SMITE_EXECUTABLE} PRIVATE -fno-optimize-sibling-calls)
target_compile_options(ace PRIVATE -fno-optimize-sibling-calls)
```

This is the most reliable solution, though it affects the entire codebase.

---

**Status:** Waiting for rebuild verification to confirm if attributes are working.

