#include <ace/utils/bitmap.h>

#include "GameState.h"

#define VIEWPORT_UI_REGION_X 1
#define VIEWPORT_UI_REGION_Y 28
#define VIEWPORT_UI_REGION_W 248
#define VIEWPORT_UI_REGION_H 160

typedef enum {
    VIEWPORT_PICK_NONE = 0,
    VIEWPORT_PICK_DOOR_AHEAD,
    VIEWPORT_PICK_WALL_BUTTON,
    VIEWPORT_PICK_DOOR_BUTTON,
} tViewportPickKind;

typedef struct {
    UBYTE kind;
    UBYTE cellX;
    UBYTE cellY;
    UBYTE wallSide;
} tViewportPick;

UBYTE mazeWallSideFacingParty(UBYTE px, UBYTE py, UBYTE cx, UBYTE cy);
void viewportPickAtScreen(tGameState *pGameState, UWORD screenX, UWORD screenY, tViewportPick *pick);

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

void drawFullScreenMap(tGameState* pGameState, tBitMap* pCurrentBuffer);

void preFillFacing(CMazeDr* mazeDr);
void preFillPosDataNoFacing(CMazePos* mazePos);