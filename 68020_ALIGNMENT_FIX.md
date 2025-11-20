# 68020+ Alignment Issue Fix

## Problem Summary

**Symptoms:**
- Crash during `blitManagerCreate` with guru meditation `8000 000B` (Address Error)
- B-Trap FFFF at address 00050100
- Only occurs on 68020+ CPUs
- Only occurs in Release builds (not Debug builds)
- Stack size: 4096 bytes

**Root Cause:**
On 68020+ CPUs, accessing WORD/UWORD (16-bit) or LONG/ULONG/pointers (32-bit) at odd memory addresses causes a hardware Address Error exception. Several ACE structures had misaligned fields due to odd numbers of UBYTE fields before larger types.

In Debug builds, the compiler adds padding to prevent this. In Release builds with optimizations, the padding is removed, causing the crash.

## Structures Fixed

### 1. `tCopBlock` (deps/ACE/include/ace/managers/copper.h)
**Issue:** After 3 consecutive UBYTE fields, the `pCmds` pointer was at an odd address.

**Fix:** Added `UBYTE _pad` padding byte before `pCmds`.

**Memory layout:**
```
Before fix:
- offset 12-14: ubDisabled, ubUpdated, ubResized (3 bytes)
- offset 15: pCmds pointer (ODD ADDRESS - CRASH!)

After fix:
- offset 12-15: ubDisabled, ubUpdated, ubResized, _pad (4 bytes)
- offset 16: pCmds pointer (EVEN ADDRESS - OK)
```

### 2. `tBob` (deps/ACE/include/ace/managers/bob.h)
**Issue:** After `UBYTE isUndrawRequired`, the `_uwBlitSize` field was misaligned.

**Fix:** Added `UBYTE _pad` padding byte after `isUndrawRequired`.

### 3. `tAdvancedSprite` (deps/ACE/include/ace/managers/advancedsprite.h)
**Issue:** After `UBYTE ubSpriteCount`, all subsequent fields (WORD, pointers, etc.) were misaligned.

**Fix:** Added `UBYTE _pad1` padding byte after `ubSpriteCount`.

### 4. `tSimpleBufferManager` (deps/ACE/include/ace/managers/viewport/simplebuffer.h)
**Issue:** After `UBYTE ubFlags`, the `UWORD uwCopperOffset` field was misaligned.

**Fix:** Added `UBYTE _pad` padding byte after `ubFlags`.

### 5. `tVpManager` (deps/ACE/include/ace/utils/extview.h)
**Issue:** Ending with single `UBYTE ubId` caused misalignment in arrays.

**Fix:** Added `UBYTE _pad[3]` to ensure 4-byte alignment.

### 6. `tRedrawState` (deps/ACE/include/ace/managers/viewport/tilebuffer.h)
**Issue:** Ending with single `UBYTE ubPendingCount` caused misalignment.

**Fix:** Added `UBYTE _pad[3]` after `ubPendingCount`.

### 7. `tTileBufferManager` (deps/ACE/include/ace/managers/viewport/tilebuffer.h)
**Issue:** Two UBYTE fields before UWORD field caused misalignment.

**Fix:** Added `UBYTE _pad1[2]` after `ubTileShift`.

### 8. `tView` (deps/ACE/include/ace/utils/extview.h)
**Issue:** After `UBYTE ubVpCount`, the `UWORD uwFlags` was at odd address.

**Fix:** Added `UBYTE _pad1` after `ubVpCount`.

### 9. `tVPort` (deps/ACE/include/ace/utils/extview.h)
**Issue:** Conditional compilation could leave `UWORD* pPalette` misaligned.

**Fix:** Added conditional `UBYTE _pad2` when `ACE_USE_AGA_FEATURES` not defined.

## Testing

To verify the fix:

1. **Clean rebuild:**
   ```
   cmake --build build --clean-first --config Release
   ```

2. **Test on 68020+ system:**
   - The program should no longer crash during initialization
   - The "Block begin: blitManagerCreate" message should complete successfully
   - No guru meditation should appear

3. **Verify structure sizes (optional):**
   Add debug prints to check structure sizes match expectations with proper alignment.

## Prevention

To prevent similar issues in the future:

1. **Structure design guidelines:**
   - Avoid odd numbers of UBYTE fields before WORD/ULONG/pointer fields
   - Always add explicit padding bytes when needed
   - Use `static_assert` to verify structure sizes if available

2. **Compiler flags:**
   - Consider using `-Wpadded` warning flag to detect padding issues during development
   - Test both Debug and Release builds on 68020+ hardware/emulation

3. **Code review checklist:**
   - Check any structure with UBYTE fields followed by larger types
   - Verify pointer fields are at even offsets
   - Test on 68020+ target before release

## Related Information

- Guru meditation `8000 000B` = Address Error (hardware exception)
- 68000 CPU: More lenient with unaligned access (slower but doesn't crash)
- 68020+ CPU: Strict alignment required (crashes on unaligned access)
- Debug builds: Compiler adds padding automatically
- Release builds: Compiler may optimize away padding for smaller structure sizes

## Files Modified

1. `deps/ACE/include/ace/managers/copper.h` - tCopBlock structure
2. `deps/ACE/include/ace/managers/bob.h` - tBob structure
3. `deps/ACE/include/ace/managers/advancedsprite.h` - tAdvancedSprite structure
4. `deps/ACE/include/ace/managers/viewport/simplebuffer.h` - tSimpleBufferManager structure
5. `deps/ACE/include/ace/utils/extview.h` - tVpManager, tView, tVPort structures
6. `deps/ACE/include/ace/managers/viewport/tilebuffer.h` - tRedrawState, tTileBufferManager structures

## Next Steps

1. Rebuild the project with the fixes
2. Test on your 68020+ hardware/emulator
3. If the issue persists, check for other structures with similar patterns
4. Consider submitting these fixes upstream to the ACE repository

