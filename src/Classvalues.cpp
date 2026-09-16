#include <vcl.h>
#pragma hdrstop

#include "Classvalues.h"

#pragma package(smart_init)

ClassValues::~ClassValues()
{
}

void ClassValues::LoadClasses()
{
}

ClassValue ClassValues::GetByIndex(ClassValues *self, int index)
{
    if (index < 0 || (unsigned)index > self->values.size() - 1)
        index = 0;
    return self->values[index];
}

void ClassValues::AddClass(ClassValues *self, int id, int field_4, String name,
                           short f0c, short f0e, short f10, short f12,
                           short f14, short f16, short f18)
{
    ClassValue v(id);
    v.name = name;
    v.field_4 = field_4;
    v.f0c = f0c;
    v.f0e = f0e;
    v.f10 = f10;
    v.f12 = f12;
    v.f14 = f14;
    v.f16 = f16;
    v.f18 = f18;
    self->values.insert(self->values.end(), v);
}

ClassValues::ClassValues()
{
    field_18 = operator new(8);
    field_3c = -1;
    field_10 = 0;
    field_0 = 0;
    field_14 = new TStringList;
    LoadClasses();
}

int ClassValues::DecodeInt(String str)
{
    String t = str;
    int result = 0;
    for (int i = 1; i <= t.Length(); i++)
    {
        unsigned char ch = t[i];
        if (ch == 0xFE || ch == 0)
            break;
        int c = ch - 1;
        if (i == 1)
            result += c;
        if (i == 2)
            result += c * 0xfd;
        if (i == 3)
            result += c * 0xfa09;
        if (i == 4)
            result += c * 0xf71ae5;
    }
    return result;
}
