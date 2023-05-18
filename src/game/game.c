#include "game.h"

#include <ace/managers/key.h>
#include "gameState.h"
#include "screen.h"


static void gameGsCreate(void) {
    	systemUse();
	
	tScreen* pScreen = ScreenGetActive();
 
    g_pCurrentMaze = mazeCreate(32,32);
    for(int i = 0; i < 32; i++) {
        for(int j = 0; j < 32; j++) {
            if (i==0 || j==0 || i==31 || j==31)
                mazeSetCell(g_pCurrentMaze, i, j, 1);
                
        }
    }
    
    //mazeEventCreate(2,2,EVENT_SETWALLCOL,1,(UBYTE[]){1});
    mazeAppendEvent(g_pCurrentMaze, mazeEventCreate(2,2,EVENT_SETWALLCOL,1,(UBYTE[]){1}));
    mazeAddString(g_pCurrentMaze, "Welcome to My Life Sucks!", 25);
    mazeAddString(g_pCurrentMaze, "Press any key to continue...", 28);
    mazeAddString(g_pCurrentMaze, "Pretty Dusty Here...", 20);
    systemUse();
     mazeSave(g_pCurrentMaze, "data/MAZE.DAT");
     mazeDelete(g_pCurrentMaze);
     g_pCurrentMaze = mazeLoad("data/MAZE.DAT");

    tWallset* pWallset = wallsetLoad("data/factory.wll");
    systemUnuse();
}

static void gameGsLoop(void) {
    
  ScreenUpdate();
    gameExit();
}

static void gameGsDestroy(void) {
    
}
tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy
};
