#include "layer.h"

void gameUIInit(cbRegion cbOnHovered, cbRegion cbOnUnHovered, cbRegionClick cbOnPressed, cbRegionClick cbOnReleased);
void gameUIUpdate(void);
void gameUIDestroy(void);

Layer* gameUIGetLayer(void);

void gameUpdateBattery(UBYTE ubBattery);

