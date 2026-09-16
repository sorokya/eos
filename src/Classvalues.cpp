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

ClassValue Classvalues::GetByIndex(Classvalues *self, int index)
{
    if (index < 0 || (unsigned)index > self->values.size() - 1)
        index = 0;
    return self->values[index];
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
