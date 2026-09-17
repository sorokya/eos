#include <vcl.h>
#pragma hdrstop

#include "Mapobject.h"

#pragma package(smart_init)

Mapobject::Mapobject(short x, short y, short value)
{
    this->x = x;
    this->y = y;
    this->value = value;
    this->ticks = 0;
}

Mapobject::~Mapobject()
{
}
