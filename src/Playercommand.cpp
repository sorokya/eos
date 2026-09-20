#include <vcl.h>
#pragma hdrstop

#include "Playercommand.h"

#pragma package(smart_init)

PlayerCommand::PlayerCommand(int family, int action, String text)
{
    this->family = family;
    this->action = action;
    this->text = text;
}

PlayerCommand::~PlayerCommand()
{
}
