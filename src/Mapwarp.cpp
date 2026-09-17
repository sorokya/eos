#include <vcl.h>
#pragma hdrstop

#include "Mapwarp.h"

#pragma package(smart_init)

MapWarp::MapWarp(
    short from_x, short from_y, int dest_map, int level, short to_x, short to_y)
{
    this->from_x = from_x;
    this->from_y = from_y;
    this->dest_map = dest_map;
    this->level = level;
    this->to_x = to_x;
    this->to_y = to_y;
}

MapWarp::~MapWarp()
{
}
