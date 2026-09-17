#include <vcl.h>
#pragma hdrstop

#include "Questcounterlist.h"

#pragma package(smart_init)

QuestCounterList::QuestCounterList(String name)
{
    this->name = name;
    count = 1;
}

QuestCounterList::QuestCounterList(String name, int count)
{
    this->name = name;
    this->count = count;
}

QuestCounterList::~QuestCounterList()
{
}
