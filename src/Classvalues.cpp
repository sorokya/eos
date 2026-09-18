#include <vcl.h>
#pragma hdrstop

#include "Classvalues.h"

#pragma package(smart_init)

ClassValues::~ClassValues()
{
}

void ClassValues::LoadClasses(ClassValues *self)
{
    if (self->loaded == 0)
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

            int h;
            int size;
            char *buf;
            try
            {
                h = FileOpen(path.c_str(), 0);
                size = FileSeek(h, 0, 2);
                FileSeek(h, 0, 0);
                buf = new char[size + 1];
                FileRead(h, buf, size);
                FileClose(h);
                data += buf;
                data.SetLength(size);
                delete[] buf;
                if (data[1] != 'E' || data[2] != 'C' || data[3] != 'F')
                    return;
                self->string_list->Add(data);
                if (file == 1)
                {
                    self->rid_1 = self->DecodeInt(data.SubString(4, 2));
                    self->rid_2 = self->DecodeInt(data.SubString(6, 2));
                    int parsed = self->DecodeInt(data.SubString(8, 2));
                    int version = self->DecodeInt(data.SubString(10, 1));
                    total = parsed;
                    self->num_classes = parsed;
                }
                data.Delete(1, 10);
                for (int j = 0; count < total && j < 0xfa; j++)
                {
                    int namelen = self->DecodeInt(data.SubString(1, 1));
                    AddClass(self,
                             self->size() + 1,
                             self->DecodeInt(data.SubString(namelen + 2, 1)),
                             data.SubString(2, namelen),
                             self->DecodeInt(data.SubString(namelen + 0x3, 1)),
                             self->DecodeInt(data.SubString(namelen + 0x4, 2)),
                             self->DecodeInt(data.SubString(namelen + 0x6, 2)),
                             self->DecodeInt(data.SubString(namelen + 0x8, 2)),
                             self->DecodeInt(data.SubString(namelen + 0xa, 2)),
                             self->DecodeInt(data.SubString(namelen + 0xc, 2)),
                             self->DecodeInt(data.SubString(namelen + 0xe, 2)));
                    count++;
                    data.Delete(1, namelen + 0xf);
                }
            }
            catch (...)
            {
                FileClose(h);
                self->loaded = 0;
            }
            file++;
        } while (count < total);

        self->field_0 = file - 1;
        self->loaded = 1;
    }
}

ClassValue ClassValues::GetByIndex(ClassValues *self, int index)
{
    if (index < 0 || (unsigned)index > self->values.size() - 1)
        index = 0;
    return self->values[index];
}

void ClassValues::AddClass(ClassValues *self,
                           int id,
                           int parent_type,
                           String name,
                           short stat_group,
                           short str,
                           short intl,
                           short wis,
                           short agi,
                           short con,
                           short cha)
{
    ClassValue v(id);
    v.name = name;
    v.parent_type = parent_type;
    v.stat_group = stat_group;
    v.str = str;
    v.intl = intl;
    v.wis = wis;
    v.agi = agi;
    v.con = con;
    v.cha = cha;
    self->values.insert(self->values.end(), v);
}

ClassValues::ClassValues()
{
    field_18 = operator new(8);
    field_3c = -1;
    loaded = 0;
    field_0 = 0;
    string_list = new TStringList;
    LoadClasses(this);
}

int ClassValues::size()
{
    return values.size();
}

int ClassValues::DecodeInt(String value)
{
    String value_copy = value;
    int result = 0;
    try
    {
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
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}
