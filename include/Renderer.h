#include <ace/utils/bitmap.h>

#include "GameState.h"

typedef struct {
    int xs;
    int ys;
} CMazeDr;

typedef struct {
    int xDelta;
    int yDelta;
} CMazePos;

extern CMazePos g_mazePos[18];
extern CMazeDr g_mazeDr[4];

// Get the facing of the monster relative to the facing of the player.
int drel(int d1, int d2);
void drawView(tGameState* pGameState, tBitMap* pCurrentBuffer);

void preFillFacing(CMazeDr* mazeDr);
void preFillPosDataNoFacing(CMazePos* mazePos);