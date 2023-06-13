#include "maze.h"

#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/utils/file.h>

unsigned char demomap[] = {
  
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,0,0,3,0,3,0,0,1,0,0,0,0,1,0,1,0,0,0,1,0,6,1,0,0,1,0,2,0,3,0,0,
  1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,1,1,0,0,1,0,1,1,0,0,1,1,1,2,1,1,1,
  1,1,1,1,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,5,0,0,0,0,1,
  1,0,0,2,0,2,0,0,1,1,1,1,0,1,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,2,1,1,
  1,0,0,1,0,1,0,0,1,0,0,0,0,1,0,0,0,2,0,0,0,1,0,0,0,0,1,0,0,0,0,1,
  1,1,1,1,0,1,1,1,1,5,1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,2,0,1,1,0,1,
  1,0,0,2,0,2,0,0,1,1,1,4,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,
  1,0,0,1,0,1,0,0,1,0,0,0,0,5,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,1,
  1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,0,0,1,0,1,1,0,1,
  1,0,0,4,0,2,0,0,1,0,0,1,0,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,
  1,0,0,1,0,1,0,5,1,0,0,0,0,0,1,0,0,1,0,1,0,1,0,1,0,0,1,0,0,0,0,1,
  1,1,1,1,0,1,1,1,1,1,1,2,1,1,1,0,0,1,1,1,0,1,1,1,0,0,1,0,0,1,0,1,
  1,0,0,2,0,2,0,0,1,0,4,0,0,0,2,0,0,2,0,0,0,0,0,2,0,0,2,0,1,1,0,1,
  1,0,0,1,0,1,0,0,1,1,1,0,1,1,1,0,0,1,1,1,0,1,1,1,0,0,1,0,0,1,0,1,
  1,1,1,1,0,1,1,1,1,0,0,5,0,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,1,
  1,0,0,2,0,2,0,0,1,0,1,1,1,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1,
  1,0,0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,1,1,1,5,1,0,0,1,0,1,1,0,1,
  1,1,1,1,0,1,1,1,1,0,0,1,0,0,1,5,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,1,
  1,0,0,2,0,2,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,
  1,5,0,1,0,1,0,0,1,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1,1,
  1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,1,5,1,3,1,1,1,1,0,1,0,1,
  1,0,0,2,0,2,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,
  1,0,0,1,0,1,0,0,1,0,0,0,2,0,0,0,2,0,0,0,0,1,1,1,1,0,1,1,1,1,4,1,
  1,1,1,1,0,1,1,1,1,1,1,1,1,1,2,1,1,1,2,1,1,1,0,0,0,0,0,0,0,1,0,1,
  1,0,0,2,0,2,0,0,1,5,1,1,0,1,0,1,0,0,0,0,1,1,0,1,1,1,1,1,0,1,0,1,
  1,5,0,1,0,1,0,0,1,0,5,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,0,1,0,1,
  1,1,1,1,0,1,1,0,1,0,1,1,0,0,0,1,0,1,0,0,1,1,0,1,1,1,1,1,0,1,0,1,
  1,0,0,2,0,1,1,1,1,0,1,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1,0,0,0,1,0,1,
  1,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,1,0,1,1,1,1,1,0,1,0,1,
  1,1,1,1,0,2,0,1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,0,0,5,1,5,0,0,1,5,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

tMaze* mazeCreateDemoData(void)
{
    tMaze* pNewMaze = mazeCreate(32, 32);
    memcpy(pNewMaze->_mazeData, demomap, 32 * 32);
    return pNewMaze;
}

tMaze* mazeCreate(UBYTE width, UBYTE height)
{
    tMaze* pMaze = (tMaze*)memAllocFastClear(sizeof(tMaze));
    pMaze->_width = width;
    pMaze->_height = height;
    pMaze->_mazeData = (UBYTE*)memAllocFastClear(sizeof(UBYTE) * width * height);
    pMaze->_mazeCol = (UBYTE*)memAllocFastClear(sizeof(UBYTE) * width * height);
    pMaze->_mazeFloor = (UBYTE*)memAllocFastClear(sizeof(UBYTE) * width * height);
    pMaze->_eventCount =0;
    pMaze->_events = 0;
   
    return pMaze;
}

tMaze* mazeLoad(const char* filename)
{
    
    tFile* pFile = fileOpen(filename, "rb");
    if (pFile)
    {
        UBYTE width = 0;
        UBYTE height = 0;
        UWORD eventCount = 0;
        UWORD stringCount = 0;
        fileRead(pFile, &width, 1);
        fileRead(pFile, &height, 1);
        tMaze* pMaze = mazeCreate(width, height);
        fileRead(pFile, pMaze->_mazeData, width * height);
        fileRead(pFile, pMaze->_mazeCol, width * height);
        fileRead(pFile, pMaze->_mazeFloor, width * height);
        fileRead(pFile, &eventCount, 2);
        
        for (int i = 0; i < eventCount; i++) {
            UBYTE x = 0;
            UBYTE y = 0;
            UBYTE eventType = 0;
            UBYTE eventDataSize = 0;
            UBYTE* eventData = 0;
            fileRead(pFile, &x, 1);
            fileRead(pFile, &y, 1);
            fileRead(pFile, &eventType, 1);
            fileRead(pFile, &eventDataSize, 1);
            eventData = (UBYTE*)memAllocFastClear(eventDataSize);
            fileRead(pFile, eventData, eventDataSize);
            tMazeEvent* event = mazeEventCreate(x, y, eventType, eventDataSize, eventData);
            mazeAppendEvent(pMaze, event);
        }
        fileRead(pFile, &stringCount, 2);
        for (int i = 0; i < stringCount; i++) {
            UWORD length = 0;
            UBYTE* string = 0;
            fileRead(pFile, &length, 2);
            string = (UBYTE*)memAllocFastClear(length);
            fileRead(pFile, string, length);
            //tMazeString* mazeString = mazeStringCreate(length, string);
            mazeAddString(pMaze, string, length);
            memFree(string, length);
        }
        fileClose(pFile);
        return pMaze;
    }
    return 0;
}

void mazeSave(tMaze* pMaze, const char* sFilename)
{
    tFile* pFile = fileOpen(sFilename, "wb");
    if (pFile)
    {
        fileWrite(pFile, &pMaze->_width, 1);
        fileWrite(pFile, &pMaze->_height, 1);
        fileWrite(pFile, pMaze->_mazeData, pMaze->_width * pMaze->_height);
        fileWrite(pFile, pMaze->_mazeCol, pMaze->_width * pMaze->_height);
        fileWrite(pFile, pMaze->_mazeFloor, pMaze->_width * pMaze->_height);
        fileWrite(pFile, &pMaze->_eventCount, 2);
           tMazeEvent* event = pMaze->_events;
     
        while (event != NULL) {
            fileWrite(pFile, &event->_x, 1);
            fileWrite(pFile, &event->_y, 1);
            fileWrite(pFile, &event->_eventType, 1);
            fileWrite(pFile, &event->_eventDataSize, 1);
            fileWrite(pFile, event->_eventData, event->_eventDataSize);
            event = event->_next;   
        }
        fileWrite(pFile, &pMaze->_stringCount, 2);
        tMazeString* mazeString = pMaze->_strings;
        while (mazeString != NULL) {
            fileWrite(pFile, &mazeString->_length, 2);
            fileWrite(pFile, mazeString->_string, mazeString->_length);
            mazeString = mazeString->_next;
        }
        fileClose(pFile);
    }
}

UBYTE mazeGetCell(tMaze* pMaze, UBYTE x, UBYTE y)
{
    return pMaze->_mazeData[x + y * pMaze->_width];
}

void mazeSetCell(tMaze* pMaze, UBYTE x, UBYTE y, UBYTE value)
{
    pMaze->_mazeData[x + y * pMaze->_width] = value;
}

void mazeAppendEvent(tMaze* pMaze, tMazeEvent* newEvent) {
    if (pMaze->_events == NULL) {
        pMaze->_events = newEvent;
    } else {
        tMazeEvent* lastEvent = pMaze->_events;
        while (lastEvent->_next != NULL) {
            lastEvent = lastEvent->_next;
        }
        lastEvent->_next = newEvent;
        newEvent->_prev = lastEvent;
    }
    pMaze->_eventCount++;
}

tMazeEvent* mazeEventCreate(UBYTE x, UBYTE y, UBYTE eventType, UBYTE eventDataSize, UBYTE* eventData) {
    tMazeEvent* event = (tMazeEvent*) memAllocFastClear(sizeof(tMazeEvent));
    event->_x = x;
    event->_y = y;
    event->_eventType = eventType;
    event->_eventDataSize = eventDataSize;
    event->_eventData = (UBYTE*) memAllocFastClear(eventDataSize * sizeof(UBYTE));
    for (int i = 0; i < eventDataSize; i++) {
        event->_eventData[i] = eventData[i];
    }
    event->_next = NULL;
    event->_prev = NULL;
    return event;
}

void mazeRemoveEvent(tMaze* pMaze, tMazeEvent* event) {
    if (pMaze->_events == event) {
        pMaze->_events = event->_next;
    }
    if (event->_prev != NULL) {
        event->_prev->_next = event->_next;
    }
    if (event->_next != NULL) {
        event->_next->_prev = event->_prev;
    }
    memFree(event->_eventData, event->_eventDataSize);
    memFree(event, sizeof(tMazeEvent));
    pMaze->_eventCount--;
}

void mazeIterateEvents(tMaze* pMaze, void (*callback)(tMazeEvent*)) {
    tMazeEvent* event = pMaze->_events;
    while (event != NULL) {
        (*callback)(event);
        event = event->_next;
    }
}

void mazeRemoveAllEvents(tMaze* pMaze) {
    // Remove all events
    tMazeEvent* currentEvent = pMaze->_events;
    while (currentEvent != NULL) {
        tMazeEvent* nextEvent = currentEvent->_next;
        memFree(currentEvent->_eventData, currentEvent->_eventDataSize);
        memFree(currentEvent, sizeof(tMazeEvent));
        currentEvent = nextEvent;
    }
    pMaze->_events = NULL;
    pMaze->_eventCount = 0;
}

void mazeDelete(tMaze* pMaze) {
    // Remove all events
   mazeRemoveAllEvents(pMaze);
    
    // Free pMaze data
    memFree(pMaze->_mazeData, sizeof(UBYTE) * pMaze->_width * pMaze->_height);
    memFree(pMaze->_mazeCol,sizeof(UBYTE) * pMaze->_width * pMaze->_height);
    memFree(pMaze->_mazeFloor,sizeof(UBYTE) * pMaze->_width * pMaze->_height);
    
    // Free pMaze struct
    memFree(pMaze, sizeof(tMaze));
}

void mazeAddString(tMaze* pMaze, char* string, UWORD length)
{
    if (pMaze==NULL) return;

    if (pMaze->_strings==NULL)
    {
        pMaze->_strings=(tMazeString*)memAllocFastClear(sizeof(tMazeString));
        pMaze->_strings->_string=(UBYTE*)memAllocFastClear(length);
        pMaze->_strings->_length=length;
        for (int i=0;i<length;i++)
        {
            pMaze->_strings->_string[i]=string[i];
        }
        pMaze->_strings->_next=NULL;
        
    }
    else
    {
        tMazeString* lastString=pMaze->_strings;
        while (lastString->_next!=NULL)
        {
            lastString=lastString->_next;
        }
        lastString->_next=(tMazeString*)memAllocFastClear(sizeof(tMazeString));
        lastString->_next->_string=(UBYTE*)memAllocFastClear(length);
        lastString->_next->_length=length;
        for (int i=0;i<length;i++)
        {
            lastString->_next->_string[i]=string[i];
        }
        lastString->_next->_next=NULL;
        
    }

    pMaze->_stringCount++;


}
void mazeRemoveStrings(tMaze* pMaze)
{
    if (pMaze==NULL) return;
    while (pMaze->_strings == NULL)
    {
        tMazeString* nextString=pMaze->_strings->_next;
        memFree(pMaze->_strings->_string,pMaze->_strings->_length);
        memFree(pMaze->_strings,sizeof(tMazeString));
        pMaze->_strings=nextString;
    }
    {
        /* code */
    }
    

}