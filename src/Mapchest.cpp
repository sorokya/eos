#include <vcl.h>
#pragma hdrstop

#include "Mapchest.h"

#pragma package(smart_init)

MapChest::MapChest(short x, short y, short key_id)
{
    this->x = x;
    this->y = y;
    this->key_id = key_id;
    this->updated = 0;
}

MapChest::~MapChest()
{
}
