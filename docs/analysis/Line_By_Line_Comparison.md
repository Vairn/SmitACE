# Line-by-Line Comparison: smite.s vs smite_works.s

## int3Handler - Register Saving

### smite.s (BROKEN):
```asm
int3Handler:
    b42:	f227 e003      	fmovemx fp0-fp1,-(sp)     # Save FP: 8 bytes
    b46:	48e7 f0c0      	movem.l d0-d3/a0-a1,-(sp) # Save: d0-d3, a0-a1 = 6 regs = 24 bytes
    ...
    c1c:	4cdf 030f      	movem.l (sp)+,d0-d3/a0-a1 # Restore: d0-d3, a0-a1 = 6 regs
    c20:	f21f d0c0      	fmovemx (sp)+,fp0-fp1     # Restore FP
    c24:	4e73           	rte
```

**Total saved:** d0-d3, a0-a1 = 6 registers (24 bytes)

### smite_works.s (WORKING):
```asm
int3Handler:
    fa2:	f227 e003      	fmovemx fp0-fp1,-(sp)     # Save FP: 8 bytes
    fa6:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp) # Save: d0-d2, a0-a1 = 5 regs = 20 bytes
    ...
    fd2:	4cdf 0307      	movem.l (sp)+,d0-d2/a0-a1 # Restore: d0-d2, a0-a1 = 5 regs
    fd6:	f21f d0c0      	fmovemx (sp)+,fp0-fp1     # Restore FP
    fda:	4e73           	rte
```

**Total saved:** d0-d2, a0-a1 = 5 registers (20 bytes)

## 🔴 CRITICAL DIFFERENCE #1: Register Count Mismatch!

**smite.s saves d0-d3 (4 data registers)**
**smite_works.s saves d0-d2 (3 data registers)**

**Problem:** If smite.s saves d3 but the working version doesn't, and d3 is used by the interrupted code, saving/restoring d3 could be wrong!

## int4Handler - Register Saving

### smite.s (BROKEN):
```asm
int4Handler:
    c26:	f227 e003      	fmovemx fp0-fp1,-(sp)
    c2a:	48e7 f0c0      	movem.l d0-d3/a0-a1,-(sp) # Saves d0-d3, a0-a1 = 6 regs
    ...
    d2e:	4cdf 030f      	movem.l (sp)+,d0-d3/a0-a1 # Restores d0-d3, a0-a1
    d32:	f21f d0c0      	fmovemx (sp)+,fp0-fp1
    d36:	4e73           	rte
```

### smite_works.s (WORKING):
```asm
int4Handler:
    eae:	f227 e003      	fmovemx fp0-fp1,-(sp)
    eb2:	48e7 c0c0      	movem.l d0-d1/a0-a1,-(sp) # Saves d0-d1, a0-a1 = 4 regs
    ...
    ed0:	4cdf 0303      	movem.l (sp)+,d0-d1/a0-a1 # Restores d0-d1, a0-a1
    ed4:	f21f d0c0      	fmovemx (sp)+,fp0-fp1
    ed8:	4e73           	rte
```

## 🔴 CRITICAL DIFFERENCE #2: int4Handler Register Count!

**smite.s saves d0-d3 (4 data registers)**
**smite_works.s saves d0-d1 (2 data registers)**

**This is a HUGE difference!** The broken version saves 2 extra registers that the working version doesn't!

## Code Flow Differences

### smite.s int3Handler:
- NO logPushInt/logPopInt calls
- Calls timerOnInterrupt() explicitly
- Complex bit manipulation for checks
- Uses d3 for temporary values

### smite_works.s int3Handler:
- HAS logPushInt() call (line 1771-1772)
- timerOnInterrupt() is INLINED (line 1797-1799)
- Simpler bit manipulation
- Uses d2 for uwReqClr (already saved)

## 🔴 CRITICAL DIFFERENCE #3: d3 Usage

**smite.s:**
- Saves d3 in movem.l
- Uses d3 for temporary values in bit checks (line 909, 933, etc.)
- Restores d3

**smite_works.s:**
- Does NOT save d3
- Does NOT use d3
- Uses d2 for uwReqClr (which is saved)

**Problem:** If smite.s saves d3 but the interrupted code was using d3, saving it is correct. BUT if the working version doesn't save d3, that means the working version's interrupted code doesn't use d3, OR the working version's code doesn't use d3 either.

**The real question:** Why does smite.s need to save d3 when smite_works.s doesn't?

## Hypothesis

The compiler in smite.s is generating code that uses d3 for temporary values, requiring it to be saved. The working version doesn't use d3, so it doesn't need to save it.

**But wait:** If smite.s saves d3 and the interrupted code was using d3, that's CORRECT behavior. The problem might be that smite.s is using d3 when it shouldn't, or the register allocation is different.

## Stack Frame Size

**smite.s:**
- FP regs: 8 bytes
- Data regs: 24 bytes (d0-d3, a0-a1)
- **Total: 32 bytes**

**smite_works.s:**
- FP regs: 8 bytes  
- Data regs: 20 bytes (d0-d2, a0-a1)
- **Total: 28 bytes**

**Difference: 4 bytes!**

This 4-byte difference in stack frame size could cause stack misalignment or exception frame corruption!

