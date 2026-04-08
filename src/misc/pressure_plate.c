#include "pressure_plate.h"
#include "maze.h"
#include "script.h"
#include <string.h>

void pressurePlateListClear(tPressurePlateList *list)
{
    if (!list)
        return;
    list->count = 0;
    memset(list->plates, 0, sizeof(list->plates));
}

UBYTE pressurePlateAdd(tPressurePlateList *list, UBYTE x, UBYTE y, UBYTE eventType,
    UBYTE dataSize, const UBYTE *pData)
{
    if (!list || dataSize > PRESSURE_PLATE_DATA_MAX || list->count >= PRESSURE_PLATES_MAX)
        return 0;
    tPressurePlate *p = &list->plates[list->count];
    p->x = x;
    p->y = y;
    p->eventType = eventType;
    p->dataSize = dataSize;
    if (dataSize > 0 && pData)
        memcpy(p->data, pData, dataSize);
    else
        memset(p->data, 0, sizeof(p->data));
    list->count++;
    return 1;
}

void pressurePlatesTryFireAt(tPressurePlateList *list, tMaze *pMaze, UBYTE px, UBYTE py)
{
    if (!list || !pMaze)
        return;
    for (UBYTE i = 0; i < list->count; i++)
    {
        tPressurePlate *pl = &list->plates[i];
        if (pl->x != px || pl->y != py)
            continue;
        tMazeEvent *ev = mazeEventCreate(px, py, pl->eventType, pl->dataSize, pl->data);
        if (ev)
        {
            handleEvent(pMaze, ev);
            mazeRemoveEvent(pMaze, ev);
        }
    }
}
