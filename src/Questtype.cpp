#include <vcl.h>
#pragma hdrstop

#include "Questtype.h"

#pragma package(smart_init)

Questtype::Questtype(int value, String name)
{
    this->value = value;
    this->name = name;
}

Questtype::~Questtype()
{
}
