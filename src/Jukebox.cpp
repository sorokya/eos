#include <vcl.h>
#pragma hdrstop

#include "Jukebox.h"

#pragma package(smart_init)

JukeBox::JukeBox(int map_id)
{
    this->map_id = map_id;
    active = 0;
}

JukeBox::~JukeBox()
{
}
