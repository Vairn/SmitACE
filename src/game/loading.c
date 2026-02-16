#include "GameState.h"
#include "smite.h"
#include <ace/managers/timer.h>

#define LOADING_STUB_DELAY 100

static void loadingGsCreate(void) {
	timerCreate();
}

static void loadingGsLoop(void) {
	if (timerGet() > LOADING_STUB_DELAY) {
		stateChange(g_pStateMachineGame, &g_sStateGame);
	}
}

static void loadingGsDestroy(void) {
}

tState g_sStateLoading = {
	.cbCreate = loadingGsCreate, .cbLoop = loadingGsLoop, .cbDestroy = loadingGsDestroy
};
