#include <vcl.h>
#pragma hdrstop

#include "Wedding.h"

#pragma package(smart_init)

Wedding::Wedding(int map_id, int priest_line)
{
    this->map_id = map_id;
    this->priest_line = priest_line;
    player1_confirmed = 0;
    player2_confirmed = 0;
}

Wedding::~Wedding()
{
}
