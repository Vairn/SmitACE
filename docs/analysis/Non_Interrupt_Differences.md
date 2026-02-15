# Non-Interrupt Code Differences

## Stack Cleanup Patterns

**smite.s (BROKEN):**
- 7 `lea` instructions for stack cleanup
- Uses `addq.l #N,sp` for small cleanup (8 bytes or less)
- Uses `lea N(sp),sp` for larger cleanup

**smite_works.s (WORKING):**
- 29 `lea` instructions for stack cleanup (4x more!)
- More explicit stack cleanup
- Uses `lea` for all stack cleanup, even small amounts

## Code Size

- **smite.s**: 23,575 lines
- **smite_works.s**: 28,257 lines

**The working version is LARGER by ~4,700 lines!**

This suggests:
- More explicit code (less optimization?)
- More stack cleanup code
- Possibly different compilation flags

## Function Call Patterns

Need to check:
- Number of indirect function calls (`jsr (a0)`, `jsr (a1)`)
- Number of direct function calls (`jsr <label>`)
- Return instruction patterns (`rts`)

## Stack Frame Management

**smite.s:**
- Fewer explicit stack cleanup operations
- Relies more on compiler-generated cleanup

**smite_works.s:**
- More explicit stack cleanup
- Uses `lea` for all stack pointer adjustments
- More conservative stack management

## Hypothesis

The working version might:
1. Have more explicit stack cleanup (prevents leaks)
2. Use different calling conventions
3. Have been compiled with different optimization flags
4. Have more defensive stack management

