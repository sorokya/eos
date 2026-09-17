#include <vcl.h>
#pragma hdrstop

#include "Jukebox.h"

#pragma package(smart_init)

JukeBox::JukeBox(short id)
{
    this->id = id;
    playing = 0;
}

JukeBox::~JukeBox()
{
}
