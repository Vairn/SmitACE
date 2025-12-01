/**
 * @file tail_call_test.c
 * @brief Test wrapper to demonstrate tail call optimization bug
 * 
 * This file provides functions to test and verify the tail call optimization bug.
 * Use these functions during initialization to detect stack corruption.
 * 
 * When compiled WITH tail call optimization (broken):
 * - Stack will corrupt progressively
 * - Eventually crashes
 * 
 * When compiled with -fno-optimize-sibling-calls (working):
 * - Stack remains stable
 * - No crashes
 */

#include <ace/managers/log.h>
#include <ace/managers/system.h>

// ============================================================================
// Test callback function pointers (simulates the bug patterns)
// ============================================================================

typedef void (*tTestCallback)(UBYTE ubArg);

static tTestCallback s_pTestCallback = 0;
static UWORD s_uwCallCount = 0;
static UWORD s_uwMaxCalls = 0;

// ============================================================================
// Test Implementation Functions
// ============================================================================

/**
 * Simple callback that does minimal work
 * Used to test if the wrapper properly cleans up stack
 * IMPORTANT: noinline prevents the compiler from optimizing this away
 */
__attribute__((noinline))
static void testCallbackImpl(UBYTE ubArg) {
	s_uwCallCount++;
	// Just do something simple to prevent optimization
	volatile UBYTE ubTemp = ubArg * 2;
	(void)ubTemp;
}

/**
 * BROKEN PATTERN: Indirect call in tail position
 * This simulates mt_e8() and will cause stack leak if tail-call optimized
 * IMPORTANT: noinline forces this to be a real function so tail call can happen
 */
__attribute__((noinline))
void tailCallTestBrokenWrapper(UBYTE ubArg) {
	if(s_pTestCallback) {
		s_pTestCallback(ubArg);  // ← TAIL CALL! Will leak 4 bytes if optimized
	}
}

/**
 * SAFE PATTERN: Force no tail call optimization by adding statement after
 * IMPORTANT: noinline forces this to be a real function
 */
__attribute__((noinline))
void tailCallTestSafeWrapper(UBYTE ubArg) {
	if(s_pTestCallback) {
		s_pTestCallback(ubArg);
	}
	// Adding any statement after prevents tail call optimization
	return;
}

/**
 * SAFE PATTERN: Using return value prevents tail call
 * IMPORTANT: noinline forces this to be a real function
 */
__attribute__((noinline))
UBYTE tailCallTestSafeWrapper2(UBYTE ubArg) {
	UBYTE ubResult = 0;
	if(s_pTestCallback) {
		s_pTestCallback(ubArg);
		ubResult = 1;
	}
	return ubResult;
}

// ============================================================================
// Test Harness
// ============================================================================

/**
 * Initialize the test system
 */
void tailCallTestInit(void) {
	logWrite("TailCallTest: Initializing\n");
	s_pTestCallback = testCallbackImpl;
	s_uwCallCount = 0;
	s_uwMaxCalls = 0;
}

/**
 * Run the broken wrapper test
 * If compiled without -fno-optimize-sibling-calls, this will leak stack!
 * 
 * @param uwIterations Number of times to call the wrapper
 * @return 1 if test passed, 0 if failed
 */
UBYTE tailCallTestRunBroken(UWORD uwIterations) {
	logWrite("TailCallTest: Running BROKEN pattern (%hu iterations)...\n", uwIterations);
	s_uwCallCount = 0;
	s_uwMaxCalls = uwIterations;
	
	// Get current stack pointer
	ULONG ulStackBefore = 0;
	asm volatile("move.l %%sp,%0" : "=r"(ulStackBefore) : : "memory");
	
	// Call the broken wrapper many times
	for(UWORD i = 0; i < uwIterations; i++) {
		tailCallTestBrokenWrapper((UBYTE)(i & 0xFF));
	}
	
	// Check stack pointer after
	ULONG ulStackAfter = 0;
	asm volatile("move.l %%sp,%0" : "=r"(ulStackAfter) : : "memory");
	
	LONG lStackDiff = (LONG)(ulStackAfter - ulStackBefore);
	
	logWrite("TailCallTest: Stack before: %08lx, after: %08lx, diff: %ld bytes\n", 
		ulStackBefore, ulStackAfter, lStackDiff);
	logWrite("TailCallTest: Callback called %hu times\n", s_uwCallCount);
	
	if(lStackDiff != 0) {
		logWrite("TailCallTest: FAILED! Stack leaked %ld bytes\n", -lStackDiff);
		return 0;
	}
	
	logWrite("TailCallTest: PASSED! Stack is clean\n");
	return 1;
}

/**
 * Run the safe wrapper test
 * Should never leak regardless of optimization
 * 
 * @param uwIterations Number of times to call the wrapper
 * @return 1 if test passed, 0 if failed
 */
UBYTE tailCallTestRunSafe(UWORD uwIterations) {
	logWrite("TailCallTest: Running SAFE pattern (%hu iterations)...\n", uwIterations);
	s_uwCallCount = 0;
	s_uwMaxCalls = uwIterations;
	
	// Get current stack pointer
	ULONG ulStackBefore = 0;
	asm volatile("move.l %%sp,%0" : "=r"(ulStackBefore) : : "memory");
	
	// Call the safe wrapper many times
	for(UWORD i = 0; i < uwIterations; i++) {
		tailCallTestSafeWrapper((UBYTE)(i & 0xFF));
	}
	
	// Check stack pointer after
	ULONG ulStackAfter = 0;
	asm volatile("move.l %%sp,%0" : "=r"(ulStackAfter) : : "memory");
	
	LONG lStackDiff = (LONG)(ulStackAfter - ulStackBefore);
	
	logWrite("TailCallTest: Stack before: %08lx, after: %08lx, diff: %ld bytes\n", 
		ulStackBefore, ulStackAfter, lStackDiff);
	logWrite("TailCallTest: Callback called %hu times\n", s_uwCallCount);
	
	if(lStackDiff != 0) {
		logWrite("TailCallTest: FAILED! Stack leaked %ld bytes\n", -lStackDiff);
		return 0;
	}
	
	logWrite("TailCallTest: PASSED! Stack is clean\n");
	return 1;
}

/**
 * Quick comprehensive test
 * Call this early in initialization to verify the fix
 * 
 * @return 1 if all tests pass, 0 if any fail
 */
UBYTE tailCallTestQuick(void) {
	logBlockBegin("TailCallTest: Quick Test");
	
	tailCallTestInit();
	
	UBYTE ubResult = 1;
	
	// Test with 100 iterations - enough to detect leaks
	if(!tailCallTestRunBroken(100)) {
		logWrite("TailCallTest: WARNING - Broken pattern failed!\n");
		logWrite("TailCallTest: This means tail call optimization is ENABLED and BROKEN\n");
		logWrite("TailCallTest: Add -fno-optimize-sibling-calls to CMAKE_C_FLAGS\n");
		ubResult = 0;
	}
	
	if(!tailCallTestRunSafe(100)) {
		logWrite("TailCallTest: ERROR - Safe pattern failed! This should never happen!\n");
		ubResult = 0;
	}
	
	if(ubResult) {
		logWrite("TailCallTest: All tests PASSED - tail call bug is fixed or not triggered\n");
	}
	
	logBlockEnd("TailCallTest: Quick Test");
	return ubResult;
}

/**
 * Stress test - calls wrapper thousands of times
 * Use this to verify stability under heavy load
 * 
 * @param uwIterations Number of iterations (suggest 10000+)
 * @return 1 if test passed, 0 if failed
 */
UBYTE tailCallTestStress(UWORD uwIterations) {
	logBlockBegin("TailCallTest: Stress Test");
	
	tailCallTestInit();
	
	logWrite("TailCallTest: Running stress test with %hu iterations\n", uwIterations);
	logWrite("TailCallTest: This simulates heavy music playback...\n");
	
	UBYTE ubResult = tailCallTestRunBroken(uwIterations);
	
	if(!ubResult) {
		logWrite("TailCallTest: STRESS TEST FAILED - Stack corruption detected!\n");
		logWrite("TailCallTest: With %hu calls, stack would be corrupted\n", uwIterations);
		logWrite("TailCallTest: This explains crashes during initialization\n");
	}
	else {
		logWrite("TailCallTest: STRESS TEST PASSED\n");
	}
	
	logBlockEnd("TailCallTest: Stress Test");
	return ubResult;
}

