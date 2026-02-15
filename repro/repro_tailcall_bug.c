/*
 * Minimal Reproduction Case: GCC Tail Call Optimization Bug on m68k
 * 
 * This demonstrates the stack leak bug when tail call optimization
 * is enabled for indirect function calls with stack arguments.
 * 
 * Compile with:
 *   - Tail call optimization ENABLED (default): Shows the bug
 *   - Tail call optimization DISABLED (-fno-optimize-sibling-calls): Works correctly
 * 
 * The bug: Functions called through function pointers with multiple
 * stack arguments leak stack space when tail-call optimized.
 */

#include <stdio.h>
#include <stdint.h>

/* Function pointer type */
typedef void (*callback_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* Target functions that will be called through function pointer */
static void target_func1(uint32_t a, uint32_t b, uint32_t c) {
	/* This function will be tail-called */
	/* If TCO is enabled, stack arguments leak here */
	(void)a; (void)b; (void)c;
}

static void target_func2(uint32_t a, uint32_t b, uint32_t c) {
	(void)a; (void)b; (void)c;
}

/* Function pointer table */
static callback_t func_table[] = {
	target_func1,
	target_func2,
};

/* 
 * This function does a tail call through function pointer.
 * With TCO enabled, this generates: jmp (aX) - NO STACK CLEANUP!
 * With TCO disabled, this generates: jsr (aX) + lea 12(sp),sp + rts
 */
static void dispatcher(uint32_t index, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
	/* Tail call through function pointer - THIS IS THE BUG */
	func_table[index](arg1, arg2, arg3);
	/* 
	 * With TCO: jmp (a1) - arguments never cleaned up!
	 * Without TCO: jsr (a1) + lea 12(sp),sp - proper cleanup
	 */
}

/* Test function that calls the dispatcher many times */
static void test_stack_leak(void) {
	uint32_t i;
	volatile uint32_t stack_marker = 0xDEADBEEF;
	
	printf("Testing stack leak...\n");
	printf("Stack marker at: %p\n", (void*)&stack_marker);
	
	/* Call dispatcher many times - each call leaks 12 bytes with TCO */
	for (i = 0; i < 1000; i++) {
		dispatcher(i % 2, 1, 2, 3);
		
		/* Check if stack marker got corrupted (stack grew into it) */
		if (stack_marker != 0xDEADBEEF) {
			printf("ERROR: Stack corruption detected at iteration %lu!\n", (unsigned long)i);
			printf("Stack marker value: 0x%08lX (expected 0xDEADBEEF)\n", 
			       (unsigned long)stack_marker);
			return;
		}
	}
	
	printf("Test completed - no corruption detected (TCO may be disabled)\n");
}

int main(void) {
	printf("GCC Tail Call Optimization Bug Reproduction\n");
	printf("===========================================\n\n");
	
	printf("This test calls a function through a function pointer\n");
	printf("with 3 stack arguments (12 bytes) 1000 times.\n\n");
	
	printf("With TCO ENABLED:\n");
	printf("  - Each call leaks 12 bytes of stack\n");
	printf("  - After ~1000 calls, stack grows ~12KB\n");
	printf("  - Stack corruption should occur\n\n");
	
	printf("With TCO DISABLED (-fno-optimize-sibling-calls):\n");
	printf("  - Stack is properly cleaned up after each call\n");
	printf("  - No stack leak, test should pass\n\n");
	
	test_stack_leak();
	
	return 0;
}

