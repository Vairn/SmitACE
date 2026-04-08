#pragma once

#include <ace/types.h>

struct _maze;
typedef struct _maze tMaze;

#define PRESSURE_PLATE_DATA_MAX 8
#define PRESSURE_PLATES_MAX 16

typedef struct {
    UBYTE x;
    UBYTE y;
    UBYTE eventType;
    UBYTE dataSize;
    UBYTE data[PRESSURE_PLATE_DATA_MAX];
} tPressurePlate;

typedef struct {
    UBYTE count;
    tPressurePlate plates[PRESSURE_PLATES_MAX];
} tPressurePlateList;

void pressurePlateListClear(tPressurePlateList *list);

/** Returns 0 if list full or invalid. */
UBYTE pressurePlateAdd(tPressurePlateList *list, UBYTE x, UBYTE y, UBYTE eventType,
    UBYTE dataSize, const UBYTE *pData);

/** After a successful move, run any plate at (px, py). */
void pressurePlatesTryFireAt(tPressurePlateList *list, tMaze *pMaze, UBYTE px, UBYTE py);
