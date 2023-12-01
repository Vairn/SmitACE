#include "item.h"

tItem* s_vecItems;

void loadItems(const char* filename)
{
    s_vecItems = NULL;
    
}

tItem* getItem(UBYTE ubIndex)
{
    return &s_vecItems[ubIndex];
}