#include <vcl.h>
#pragma hdrstop

#include "Classvalues.h"

#pragma package(smart_init)

ClassValues::~ClassValues()
{
}

void ClassValues::LoadClasses()
{
    if (field_10 == 0)
    {
    int file = 1;
    int count = 0;
    int total = 1;

    do
    {
        String data;
        String path = "./pub/dat";
        if (file < 10)
            path = path + "00" + IntToStr(file) + ".ecf";
        else
            path = path + "0" + IntToStr(file) + ".ecf";

        int h = FileOpen(path.c_str(), 0);
        int size = FileSeek(h, 0, 2);
        FileSeek(h, 0, 0);
        char *buf = new char[size + 1];
        FileRead(h, buf, size);
        FileClose(h);
        data += buf;
        data.SetLength(size);
        delete[] buf;
        if (data[1] != 'E' || data[2] != 'C' || data[3] != 'F')
            return;
        field_14->Add(data);
        if (file == 1)
        {
            rid_1 = DecodeInt(data.SubString(4, 2));
            rid_2 = DecodeInt(data.SubString(6, 2));
            total = DecodeInt(data.SubString(8, 2));
            DecodeInt(data.SubString(10, 1));
            num_classes = total;
        }
        data.Delete(1, 10);
        for (int j = 0; count < total && j < 0xfa; j++)
        {
            int namelen = DecodeInt(data.SubString(1, 1));
            short cha = DecodeInt(data.SubString(namelen + 0xe, 2));
            short con = DecodeInt(data.SubString(namelen + 0xc, 2));
            short agi = DecodeInt(data.SubString(namelen + 0xa, 2));
            short wis = DecodeInt(data.SubString(namelen + 0x8, 2));
            short intl = DecodeInt(data.SubString(namelen + 0x6, 2));
            short str = DecodeInt(data.SubString(namelen + 0x4, 2));
            short stat_group = DecodeInt(data.SubString(namelen + 0x3, 1));
            String name = data.SubString(2, namelen);
            int ptype = DecodeInt(data.SubString(namelen + 2, 1));
            int id = values.size();
            AddClass(this, id + 1, ptype, name, stat_group, str, intl, wis, agi, con, cha);
            count++;
            data.Delete(1, namelen + 0xf);
        }
        file++;
    } while (count < total);

    field_0 = file;
    field_10 = 1;
    }
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
