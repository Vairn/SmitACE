# Register Saving Analysis

## Comparison: smite.s vs smite_works.s

### int3Handler Register Saving

**smite.s (current):**
```asm
int3Handler:
    b42:	598f           	subq.l #4,sp          # Allocate stack space
    b44:	f227 e003      	fmovemx fp0-fp1,-(sp)  # Save FP registers ✅
    b48:	48e7 c0c0      	movem.l d0-d1/a0-a1,-(sp)  # Save d0-d1, a0-a1 ✅
    ...
    c18:	4cdf 0303      	movem.l (sp)+,d0-d1/a0-a1  # Restore
    c1c:	f21f d0c0      	fmovemx (sp)+,fp0-fp1      # Restore FP
    c20:	588f           	addq.l #4,sp          # Deallocate stack space
    c22:	4e73           	rte
```

**smite_works.s (working):**
```asm
int3Handler:
    fa2:	f227 e003      	fmovemx fp0-fp1,-(sp)  # Save FP registers ✅
    fa6:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)  # Save d0-d2, a0-a1 ✅
    ...
    fd2:	4cdf 0307      	movem.l (sp)+,d0-d2/a0-a1  # Restore
    fd6:	f21f d0c0      	fmovemx (sp)+,fp0-fp1      # Restore FP
    fda:	4e73           	rte
```

## Key Differences

1. **smite.s saves d0-d1, smite_works.s saves d0-d2**
   - Current version: `movem.l d0-d1/a0-a1`
   - Working version: `movem.l d0-d2/a0-a1`

2. **smite.s allocates stack space first (`subq.l #4,sp`)**
   - This is for storing `uwReqClr` on the stack
   - Working version uses d2 for `uwReqClr` instead

3. **smite.s has `logPushInt()`/`logPopInt()` calls missing or optimized away**
   - Working version has these calls which might use d2

## Analysis

The current version stores `uwReqClr` on the stack instead of in a register. This is actually **safer** from a register corruption perspective, BUT:

**Potential Issue:** If the stack gets corrupted by tail calls (which we're trying to fix), the stack location where `uwReqClr` is stored could be overwritten!

The working version uses d2 for `uwReqClr`, which means:
- d2 must be saved (which it is)
- d2 won't be corrupted by stack leaks
- But d2 could be corrupted if the interrupted code was using it

## Conclusion

The register saving itself looks correct - both versions save the registers they use. The difference is:
- **Current version:** Uses stack for `uwReqClr` (vulnerable to stack corruption)
- **Working version:** Uses d2 for `uwReqClr` (vulnerable to register corruption if interrupted code was using d2)

**However**, the real issue is likely still **tail calls** causing stack corruption, not register saving. The register saving looks fine - the problem is that tail calls leak stack space, which corrupts the stack location where `uwReqClr` is stored in the current version.

