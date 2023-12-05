#include "smite.h"
#include <ace/managers/state.h>

extern tStateManager *g_pStateMachineGame;


static void MoveForwards();
static void MoveBackwards();
static void MoveLeft();
static void MoveRight();
static void TurnLeft();
static void TurnRight();
