#include <vcl.h>
#pragma hdrstop

#include "Questtype.h"

#pragma package(smart_init)

QuestType::QuestType(int value, String name)
{
    this->value = value;
    this->name = name;
}

QuestType::~QuestType()
{
}
