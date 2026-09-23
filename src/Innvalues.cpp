#include <vcl.h>
#pragma hdrstop

#include "Innvalues.h"
#include "Protocol.h"

#pragma package(smart_init)

InnValues::InnValues()
{
    field_0x24 = -1;
    loaded = 0;
    LoadInns(this);
}

InnValues::~InnValues()
{
}

void InnValues::LoadInns(InnValues *self)
{
    if (self->loaded == 0)
    {
        Clear(self);
        String data;
        String path = "./pub/din001.eid";
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
            if (data[1] != 'E' || data[2] != 'I' || data[3] != 'D')
                return;
            data.Delete(1, 3);
            do
            {
                int id = self->DecodeNumber(data.SubString(1, 2));
                int namelen = self->DecodeNumber(data.SubString(3, 1));
                InnValue v(id);
                v.name = data.SubString(4, namelen);
                v.spawn_map = self->DecodeNumber(data.SubString(namelen + 4, 2));
                v.spawn_x = self->DecodeNumber(data.SubString(namelen + 6, 1));
                v.spawn_y = self->DecodeNumber(data.SubString(namelen + 7, 1));
                v.sleep_map = self->DecodeNumber(data.SubString(namelen + 8, 2));
                v.sleep_x = self->DecodeNumber(data.SubString(namelen + 0xa, 1));
                v.sleep_y = self->DecodeNumber(data.SubString(namelen + 0xb, 1));
                v.alternate_spawn_enabled =
                    self->DecodeNumber(data.SubString(namelen + 0xc, 1));
                v.alternate_spawn_map =
                    self->DecodeNumber(data.SubString(namelen + 0xd, 2));
                v.alternate_spawn_x =
                    self->DecodeNumber(data.SubString(namelen + 0xf, 1));
                v.alternate_spawn_y =
                    self->DecodeNumber(data.SubString(namelen + 0x10, 1));
                for (int i = 0; i < 3; i++)
                {
                    int qlen = self->DecodeNumber(data.SubString(namelen + 0x11, 1));
                    if (qlen > 0)
                        v.question[i] = data.SubString(namelen + 0x12, qlen);
                    namelen = namelen + qlen + 1;
                    int alen = self->DecodeNumber(data.SubString(namelen + 0x11, 1));
                    if (alen > 0)
                        v.answer[i] = data.SubString(namelen + 0x12, alen);
                    namelen = namelen + alen + 1;
                }
                data.Delete(1, namelen + 0x10);
                self->record_list.insert(self->record_list.end(), v);
            } while (data.Length() > 0xf);
            self->loaded = 1;
        }
        catch (...)
        {
            FileClose(file_handle);
            self->loaded = 0;
        }
    }
}

String InnValues::GetName(InnValues *self, int index)
{
    if (index >= self->record_list.size())
        return "";
    return self->record_list[index].name;
}

int InnValues::GetSleepMap(InnValues *self, int index)
{
    if (self->record_list.size() < index)
        return -1;
    return self->record_list[index].sleep_map;
}

int InnValues::GetSleepX(InnValues *self, int index)
{
    return self->record_list[index].sleep_x;
}

int InnValues::GetSleepY(InnValues *self, int index)
{
    return self->record_list[index].sleep_y;
}

int InnValues::GetSpawnMap(InnValues *self, int index, int threshold)
{
    int result = -1;
    if (index < self->record_list.size())
    {
        result = self->record_list[index].spawn_map;
        if (self->record_list[index].alternate_spawn_enabled > 0)
        {
            if (self->record_list[index].alternate_spawn_enabled <= threshold)
                result = self->record_list[index].alternate_spawn_map;
        }
    }
    return result;
}

int InnValues::GetSpawnX(InnValues *self, int index, int threshold)
{
    int result = self->record_list[index].spawn_x;
    if (self->record_list[index].alternate_spawn_enabled > 0)
    {
        if (self->record_list[index].alternate_spawn_enabled <= threshold)
            result = self->record_list[index].alternate_spawn_x;
    }
    return result;
}

int InnValues::GetSpawnY(InnValues *self, int index, int threshold)
{
    int result = self->record_list[index].spawn_y;
    if (self->record_list[index].alternate_spawn_enabled > 0)
    {
        if (self->record_list[index].alternate_spawn_enabled <= threshold)
            result = self->record_list[index].alternate_spawn_y;
    }
    return result;
}

String InnValues::GetQuestion(InnValues *self, int index)
{
    if (index >= self->record_list.size())
        return "";
    String result;
    for (int i = 0; i < 3; i++)
    {
        result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
        result.Insert(self->record_list[index].question[i], result.Length() + 1);
    }
    return result;
}

String InnValues::GetAnswer(InnValues *self, int index, int number)
{
    if (index >= self->record_list.size())
        return "";
    return self->record_list[index].answer[number];
}

void InnValues::Clear(InnValues *self)
{
    if (self->GetCount() >= 1)
    {
        self->record_list.clear();
        self->field_0x24 = -1;
    }
}

unsigned int InnValues::GetCount()
{
    return record_list.size();
}

int InnValues::DecodeNumber(String value)
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
            if (ch == EO_PADDING_BYTE)
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
