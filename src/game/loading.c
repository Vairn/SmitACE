#include "GameState.h"
#include "smite.h"
#include "screen.h"
#include <ace/managers/log.h>
#include <ace/managers/timer.h>

#define LOADING_STUB_DELAY 100

static void loadingGsCreate(void) {
	logBlockBegin("loadingGsCreate()");
	timerCreate();
	logBlockEnd("loadingGsCreate()");
}

static void loadingGsLoop(void) {
	if (timerGet() > LOADING_STUB_DELAY) {
		stateChange(g_pStateMachineGame, &g_sStateGame);
		return;
	}
	ScreenUpdate();
}

static void loadingGsDestroy(void) {
}

tState g_sStateLoading = {
	.cbCreate = loadingGsCreate, .cbLoop = loadingGsLoop, .cbDestroy = loadingGsDestroy
};
