#include <vcl.h>
#pragma hdrstop

#include "Playercommand.h"

#pragma package(smart_init)

PlayerCommand::PlayerCommand(int action, int arg, String text)
{
    this->action = action;
    this->arg = arg;
    this->text = text;
}

PlayerCommand::~PlayerCommand()
{
}
