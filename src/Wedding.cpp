#include <vcl.h>
#pragma hdrstop

#include "Wedding.h"

#pragma package(smart_init)

Wedding::Wedding(int map_id, int priest_line)
{
    this->map_id = map_id;
    this->priest_line = priest_line;
    field_0x10 = 0;
    field_0x1c = 0;
}

Wedding::~Wedding()
{
}
