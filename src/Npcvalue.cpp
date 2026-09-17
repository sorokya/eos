#include <vcl.h>
#pragma hdrstop

#include "Npcvalue.h"

#pragma package(smart_init)

NpcValue::NpcValue()
{
    talk_rate = 0;
    has_talk = 0;
    drops.clear();
    talk_lines.clear();
}

NpcValue::NpcValue(int id)
{
    this->id = id;
    talk_rate = 0;
    has_talk = 0;
    drops.clear();
    talk_lines.clear();
}

NpcValue::~NpcValue()
{
}

void NpcValue::AddDrop(int item_id, int min_amount, int max_amount, int rate)
{
    NpcDropItem val(item_id);
    val.min_amount = min_amount;
    val.max_amount = max_amount;
    val.rate = rate;
    drops.insert(drops.end(), val);
}

void NpcValue::AddTalkMessage(int rate, String message)
{
    has_talk = 1;
    talk_rate = rate;
    talk_lines.insert(talk_lines.end(), message);
}
