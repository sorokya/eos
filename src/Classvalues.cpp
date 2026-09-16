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

int ClassValues::DecodeInt(String value)
{
    String value_copy = value;
    int result = 0;
    try {
    int byte_index = 1;
    while (value_copy.Length() >= byte_index)
    {
        char c = value_copy[byte_index];
        unsigned char ch = c;
        if (ch == 0xFE || ch == 0)
            break;
        int n = ch;
        n = n - 1;
        if (byte_index == 1)
            result = result + n;
        if (byte_index == 2)
            result = result + n * 0xfd;
        if (byte_index == 3)
            result = result + n * 0xfa09;
        if (byte_index == 4)
            result = result + n * 0xf71ae5;
        byte_index = byte_index + 1;
    }
    } catch (...) { result = 0; }
    return result;
}
