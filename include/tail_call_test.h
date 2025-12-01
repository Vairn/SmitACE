/**
 * @file tail_call_test.h
 * @brief Test wrapper to demonstrate and verify tail call optimization bug
 * 
 * USAGE:
 * 
 * 1. Add to your main() or gameGsCreate():
 *    #include "tail_call_test.h"
 *    tailCallTestQuick();
 * 
 * 2. Check the log output:
 *    - If tests PASS: Tail call bug is fixed (-fno-optimize-sibling-calls is working)
 *    - If tests FAIL: Tail call bug is present (need to add -fno-optimize-sibling-calls)
 * 
 * 3. Optional stress test:
 *    tailCallTestStress(10000);  // Simulates heavy music playback
 * 
 * EXPECTED RESULTS:
 * 
 * WITH -fno-optimize-sibling-calls (CORRECT):
 *   ✅ Both broken and safe patterns pass
 *   ✅ Stack remains stable
 *   ✅ No crashes
 * 
 * WITHOUT -fno-optimize-sibling-calls (BROKEN):
 *   ❌ Broken pattern fails (stack leaks)
 *   ✅ Safe pattern passes
 *   ❌ Will eventually crash
 */

#ifndef _TAIL_CALL_TEST_H_
#define _TAIL_CALL_TEST_H_

#include <ace/types.h>

/**
 * @brief Initialize the tail call test system
 * 
 * Call this before running any tests.
 */
void tailCallTestInit(void);

/**
 * @brief Test the broken wrapper pattern
 * 
 * This simulates the mt_e8() and other functions that have tail call bugs.
 * 
 * @param uwIterations Number of times to call the wrapper
 * @return 1 if test passed (stack clean), 0 if failed (stack leaked)
 */
UBYTE tailCallTestRunBroken(UWORD uwIterations);

/**
 * @brief Test the safe wrapper pattern
 * 
 * This demonstrates the workaround pattern that prevents tail call optimization.
 * 
 * @param uwIterations Number of times to call the wrapper
 * @return 1 if test passed (stack clean), 0 if failed (should never fail)
 */
UBYTE tailCallTestRunSafe(UWORD uwIterations);

/**
 * @brief Quick comprehensive test
 * 
 * Runs both broken and safe patterns with 100 iterations each.
 * Call this early in initialization to verify the fix is working.
 * 
 * RECOMMENDED: Add this to gameGsCreate() or main():
 * 
 *   tailCallTestQuick();
 * 
 * Then check the log for PASS/FAIL messages.
 * 
 * @return 1 if all tests pass, 0 if any fail
 */
UBYTE tailCallTestQuick(void);

/**
 * @brief Stress test with thousands of iterations
 * 
 * Simulates heavy music playback to verify stability.
 * Use this to ensure the fix handles production workloads.
 * 
 * USAGE:
 * 
 *   tailCallTestStress(10000);  // 10,000 calls
 * 
 * @param uwIterations Number of iterations (suggest 10000+)
 * @return 1 if test passed, 0 if failed
 */
UBYTE tailCallTestStress(UWORD uwIterations);

/**
 * @brief Broken wrapper pattern - simulates mt_e8()
 * 
 * This function will cause stack leaks if tail-call optimized.
 * DO NOT USE IN PRODUCTION - for testing only!
 * 
 * @param ubArg Test argument
 */
void tailCallTestBrokenWrapper(UBYTE ubArg);

/**
 * @brief Safe wrapper pattern - prevents tail call
 * 
 * This demonstrates how to write callback wrappers safely.
 * 
 * @param ubArg Test argument
 */
void tailCallTestSafeWrapper(UBYTE ubArg);

/**
 * @brief Safe wrapper pattern with return value
 * 
 * Another way to prevent tail call optimization.
 * 
 * @param ubArg Test argument
 * @return Always returns 1 if callback was called, 0 otherwise
 */
UBYTE tailCallTestSafeWrapper2(UBYTE ubArg);

#endif // _TAIL_CALL_TEST_H_

