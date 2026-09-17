#include <vcl.h>
#pragma hdrstop

#include "Playerquest.h"

#pragma package(smart_init)

PlayerQuest::PlayerQuest(int quest_id, short a, short b)
{
    this->quest_id = quest_id;
    this->field_4 = a;
    this->field_6 = b;
    field_12 = 0;
    for (int i = 0; i < 5; i++)
        field_8[i] = 0;
}

PlayerQuest::~PlayerQuest()
{
}
