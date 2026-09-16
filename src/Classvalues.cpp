#include <vcl.h>
#pragma hdrstop

#include "Classvalues.h"

#pragma package(smart_init)

Classvalues::~Classvalues()
{
}

void Classvalues::LoadClasses()
{
}

Classvalues::Classvalues()
{
    field_18 = operator new(8);
    field_3c = -1;
    field_10 = 0;
    field_0 = 0;
    field_14 = new TStringList;
    LoadClasses();
}
