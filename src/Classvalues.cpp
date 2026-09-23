#include <vcl.h>
#pragma hdrstop

#include "Classvalues.h"
#include "Protocol.h"

#pragma package(smart_init)

ClassValues::ClassValues()
{
    field_0x18 = new char[8];
    field_0x3c = -1;
    loaded = 0;
    file_id = 0;
    string_list = new TStringList;
    LoadClasses(this);
}

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

            int file_handle;
            int size;
            char *buf;
            try
            {
                file_handle = FileOpen(path.c_str(), 0);
                size = FileSeek(file_handle, 0, 2);
                FileSeek(file_handle, 0, 0);
                buf = new char[size + 1];
                FileRead(file_handle, buf, size);
                FileClose(file_handle);
                data = buf;
                data.SetLength(size);
                delete[] buf;
                if (data[1] != 'E' || data[2] != 'C' || data[3] != 'F')
                    return;
                self->string_list->Add(data);
                if (file == 1)
                {
                    self->rid_1 = self->DecodeNumber(data.SubString(4, 2));
                    self->rid_2 = self->DecodeNumber(data.SubString(6, 2));
                    int parsed = self->DecodeNumber(data.SubString(8, 2));
                    int version = self->DecodeNumber(data.SubString(10, 1));
                    total = parsed;
                    self->num_classes = parsed;
                }
                data.Delete(1, 10);
                for (int j = 0; count < total && j < 0xfa; j++)
                {
                    int namelen = self->DecodeNumber(data.SubString(1, 1));
                    AddClass(self,
                             self->GetCount() + 1,
                             self->DecodeNumber(data.SubString(namelen + 2, 1)),
                             data.SubString(2, namelen),
                             self->DecodeNumber(data.SubString(namelen + 0x3, 1)),
                             self->DecodeNumber(data.SubString(namelen + 0x4, 2)),
                             self->DecodeNumber(data.SubString(namelen + 0x6, 2)),
                             self->DecodeNumber(data.SubString(namelen + 0x8, 2)),
                             self->DecodeNumber(data.SubString(namelen + 0xa, 2)),
                             self->DecodeNumber(data.SubString(namelen + 0xc, 2)),
                             self->DecodeNumber(data.SubString(namelen + 0xe, 2)));
                    count++;
                    data.Delete(1, namelen + 0xf);
                }
            }
            catch (...)
            {
                FileClose(file_handle);
                self->loaded = 0;
            }
            file++;
        } while (count < total);

        self->file_id = file - 1;
        self->loaded = 1;
    }
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
    self->record_list.insert(self->record_list.end(), v);
}

// Nothing calls this, so ilink32 drops its COMDAT -- but it instantiates
// vector<ClassValue>::operator[] at this point in the unit, which is where the
// reference emits it (0x5371d4), ahead of ClassMatches rather than after it. Only that
// placement is observable.
ClassValue *Classvalues_Get(ClassValues *self, int index)
{
    return &self->record_list[index];
}

bool ClassValues::ClassMatches(ClassValues *self, int class_id, int class_requirement)
{
    if (class_requirement == 0)
        return true;
    while (class_id > 0)
    {
        if (class_id == class_requirement)
            return true;
        class_id = self->record_list[class_id - 1].parent_type;
    }
    return false;
}

ClassValue ClassValues::GetByIndex(ClassValues *self, int index)
{
    if (index < 0 || (unsigned)index > self->record_list.size() - 1)
        index = 0;
    return self->record_list[index];
}

int ClassValues::GetCount()
{
    return record_list.size();
}

int ClassValues::DecodeNumber(String value)
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
            if (ch == EO_PADDING_BYTE || ch == 0)
                break;
            int n = ch;
            n = n - 1;
            if (byte_index == 1)
                result = result + n;
            if (byte_index == 2)
                result = result + n * EO_CHAR_MAX;
            if (byte_index == 3)
                result = result + n * EO_SHORT_MAX;
            if (byte_index == 4)
                result = result + n * EO_THREE_MAX;
            byte_index = byte_index + 1;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}
