/**
 * GCC m68k Tail Call Optimization Bug Demonstration
 * 
 * This minimal test case demonstrates a bug in GCC's m68k backend where
 * tail call optimization of indirect function calls with stack-passed
 * arguments causes stack corruption.
 * 
 * To compile and test:
 *   m68k-amigaos-gcc -O2 -S gcc_m68k_tailcall_bug_demo.c -o broken.s
 *   m68k-amigaos-gcc -O2 -fno-optimize-sibling-calls -S gcc_m68k_tailcall_bug_demo.c -o working.s
 * 
 * Expected result:
 *   - broken.s will show: jmp (a0) without stack cleanup → stack leak
 *   - working.s will show: jsr (a0) followed by stack cleanup → correct
 * 
 * Observed on: GCC 6.5.0, GCC 10+ for m68k-amigaos
 */

#include <stdio.h>

/* Volatile to prevent constant folding */
static volatile unsigned int g_stack_measurements[100];
static volatile unsigned int g_measurement_count = 0;

/* Function pointer type - callback with stack-passed argument */
typedef void (*callback_fn)(unsigned int arg);

/* Global callback pointer - prevents inlining */
static callback_fn g_callback = 0;

/**
 * Simple callback implementation
 * This needs to be non-trivial to prevent inlining
 */
__attribute__((noinline))
static void callback_impl(unsigned int arg) {
    /* Do something non-trivial to prevent optimization */
    volatile unsigned int temp = arg * 2;
    g_stack_measurements[g_measurement_count % 100] = temp;
}

/**
 * BUGGY PATTERN: Indirect call in tail position with stack argument
 * 
 * With -O2 (tail call optimization enabled):
 *   - GCC will convert the jsr/rts to jmp
 *   - Stack argument will not be cleaned up
 *   - Each call leaks 4 bytes
 * 
 * With -fno-optimize-sibling-calls:
 *   - Uses normal jsr (a0)
 *   - Stack is cleaned up properly with addq.l #4,sp
 *   - No leak
 */
__attribute__((noinline))
void indirect_call_wrapper(unsigned int arg) {
    if (g_callback) {
        /* This is in tail position - will be tail-call optimized */
        g_callback(arg);
    }
}

/**
 * Get current stack pointer
 */
__attribute__((always_inline))
static inline unsigned long get_stack_pointer(void) {
    unsigned long sp;
    __asm__ volatile("move.l %%sp,%0" : "=r"(sp) : : "memory");
    return sp;
}

/**
 * Test harness - calls the wrapper many times and measures stack leak
 */
void test_stack_leak(unsigned int iterations) {
    unsigned long stack_before, stack_after;
    unsigned int i;
    
    /* Initialize callback */
    g_callback = callback_impl;
    g_measurement_count = 0;
    
    /* Measure stack before */
    stack_before = get_stack_pointer();
    
    /* Call wrapper many times - each call should clean up its stack */
    for (i = 0; i < iterations; i++) {
        indirect_call_wrapper(i);
    }
    
    /* Measure stack after */
    stack_after = get_stack_pointer();
    
    /* Print results */
    printf("Stack before: 0x%08lx\n", stack_before);
    printf("Stack after:  0x%08lx\n", stack_after);
    printf("Stack diff:   %ld bytes\n", (long)(stack_after - stack_before));
    printf("Iterations:   %u\n", iterations);
    
    if (stack_after != stack_before) {
        printf("\n*** BUG DETECTED: Stack leaked %ld bytes! ***\n", 
               (long)(stack_before - stack_after));
        printf("Expected: 0 bytes leaked (stack should be same before/after)\n");
    } else {
        printf("\nOK: Stack is clean (no leak detected)\n");
    }
}

/**
 * Additional test case: Multiple arguments on stack
 * This demonstrates the bug with more stack arguments
 */
typedef void (*callback_3args_fn)(unsigned int a, unsigned int b, unsigned int c);
static callback_3args_fn g_callback_3args = 0;

__attribute__((noinline))
static void callback_3args_impl(unsigned int a, unsigned int b, unsigned int c) {
    volatile unsigned int temp = a + b + c;
    g_stack_measurements[g_measurement_count % 100] = temp;
}

/**
 * BUGGY PATTERN: 3 stack arguments in tail-call position
 * This should leak 12 bytes per call with tail-call optimization
 */
__attribute__((noinline))
void indirect_call_wrapper_3args(unsigned int a, unsigned int b, unsigned int c) {
    if (g_callback_3args) {
        g_callback_3args(a, b, c);
    }
}

void test_stack_leak_3args(unsigned int iterations) {
    unsigned long stack_before, stack_after;
    unsigned int i;
    
    g_callback_3args = callback_3args_impl;
    g_measurement_count = 0;
    
    stack_before = get_stack_pointer();
    
    for (i = 0; i < iterations; i++) {
        indirect_call_wrapper_3args(i, i+1, i+2);
    }
    
    stack_after = get_stack_pointer();
    
    printf("\n=== Test with 3 stack arguments ===\n");
    printf("Stack before: 0x%08lx\n", stack_before);
    printf("Stack after:  0x%08lx\n", stack_after);
    printf("Stack diff:   %ld bytes\n", (long)(stack_after - stack_before));
    printf("Iterations:   %u\n", iterations);
    
    if (stack_after != stack_before) {
        printf("\n*** BUG DETECTED: Stack leaked %ld bytes! ***\n", 
               (long)(stack_before - stack_after));
        printf("Expected leak per call: 12 bytes (3 args × 4 bytes)\n");
        printf("Actual leak per call: %ld bytes\n", 
               (long)(stack_before - stack_after) / iterations);
    } else {
        printf("\nOK: Stack is clean (no leak detected)\n");
    }
}

int main(void) {
    printf("GCC m68k Tail Call Optimization Bug Test\n");
    printf("==========================================\n\n");
    
    printf("Testing indirect call with 1 argument (4 bytes on stack)...\n");
    test_stack_leak(100);
    
    test_stack_leak_3args(100);
    
    printf("\n=== How to verify ===\n");
    printf("1. Compile with: m68k-amigaos-gcc -O2 -S file.c -o broken.s\n");
    printf("2. Look for 'indirect_call_wrapper' in broken.s\n");
    printf("3. You should see 'jmp (a0)' or 'jmp (a1)' at the end\n");
    printf("4. This is WRONG - should be 'jsr (a0); addq.l #4,sp; rts'\n");
    printf("\n");
    printf("5. Compile with: m68k-amigaos-gcc -O2 -fno-optimize-sibling-calls -S file.c -o working.s\n");
    printf("6. Look for 'indirect_call_wrapper' in working.s\n");
    printf("7. You should see 'jsr (a0); addq.l #4,sp; rts'\n");
    printf("8. This is CORRECT - stack is cleaned up properly\n");
    
    return 0;
}

