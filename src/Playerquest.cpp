#include <vcl.h>
#pragma hdrstop

#include "Playerquest.h"

#pragma package(smart_init)

PlayerQuest::PlayerQuest(int quest_id, int state_index, int version)
{
    this->quest_id = quest_id;
    this->state_index = state_index;
    this->version = version;
    done = 0;
    for (int i = 0; i < 5; i++)
        counters[i] = 0;
}

PlayerQuest::~PlayerQuest()
{
}
