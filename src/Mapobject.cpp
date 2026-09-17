#include <vcl.h>
#pragma hdrstop

#include "Mapobject.h"

#pragma package(smart_init)

MapObject::MapObject(int x, int y, int value)
{
    this->x = x;
    this->y = y;
    this->value = value;
    this->ticks = 0;
}

MapObject::~MapObject()
{
}
