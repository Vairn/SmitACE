#include "GameState.h"


static void loadingGsCreate(void) {
}

static void loadingGsLoop(void) {
}

static void loadingGsDestroy(void) {
}

tState g_stateLoading = {
	.cbCreate = loadingGsCreate, .cbLoop = loadingGsLoop, .cbDestroy = loadingGsDestroy
};
