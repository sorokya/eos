#include <vcl.h>
#pragma hdrstop

#include "Quest.h"

#pragma package(smart_init)

Quest::Quest(int id)
{
    this->quest_id = id;
    field_0xc = 0;
    loaded = 0;
}

Quest::~Quest()
{
}
