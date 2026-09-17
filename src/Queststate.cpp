#include <vcl.h>
#pragma hdrstop

#include "Queststate.h"

#pragma package(smart_init)

QuestAction::QuestAction(int action)
{
    this->action = action;
}

QuestAction::~QuestAction()
{
}

QuestRule::QuestRule(int rule)
{
    this->rule = rule;
    for (int i = 0; i < 4; i++)
        args[i] = 0;
}

QuestRule::~QuestRule()
{
}

QuestState::QuestState(int state_index, String name)
{
    this->state_index = state_index;
    this->name = name;
    fast_dispatch_rule_index = 0;
    fast_dispatch_condition_type = 0;
}

QuestState::~QuestState()
{
}
