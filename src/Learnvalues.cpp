#include <vcl.h>
#pragma hdrstop

#include "Learnvalues.h"
#include "Protocol.h"

#pragma package(smart_init)

LearnValues::LearnValues()
{
    field_0x24 = -1;
    loaded = 0;
    LoadSkillMasters(this);
}

LearnValues::~LearnValues()
{
}

void LearnValues::LoadSkillMasters(LearnValues *self)
{
    if (self->loaded == 0)
    {
        Clear(self);
        String data;
        String path = "./pub/dsm001.emf";
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
            data += buf;
            data.SetLength(size);
            delete[] buf;
            if (data[1] != 'E' || data[2] != 'M' || data[3] != 'F')
                return;
            data.Delete(1, 3);
            do
            {
                int id = DecodeNumber(self, data.SubString(1, 2));
                int namelen = DecodeNumber(self, data.SubString(3, 1));
                LearnValue record(id);
                record.name += data.SubString(4, namelen);
                record.min_level = DecodeNumber(self, data.SubString(namelen + 4, 1));
                record.max_level = DecodeNumber(self, data.SubString(namelen + 5, 1));
                record.class_requirement =
                    DecodeNumber(self, data.SubString(namelen + 6, 1));
                int skill_count = DecodeNumber(self, data.SubString(namelen + 7, 2));
                data.Delete(1, namelen + 8);
                for (int j = 0; j < skill_count; j++)
                {
                    AddSkill(self,
                             &record,
                             DecodeNumber(self, data.SubString(1, 2)),
                             DecodeNumber(self, data.SubString(3, 1)),
                             DecodeNumber(self, data.SubString(4, 1)),
                             DecodeNumber(self, data.SubString(5, 4)),
                             DecodeNumber(self, data.SubString(9, 2)),
                             DecodeNumber(self, data.SubString(0xb, 2)),
                             DecodeNumber(self, data.SubString(0xd, 2)),
                             DecodeNumber(self, data.SubString(0xf, 2)),
                             DecodeNumber(self, data.SubString(0x11, 2)),
                             DecodeNumber(self, data.SubString(0x13, 2)),
                             DecodeNumber(self, data.SubString(0x15, 2)),
                             DecodeNumber(self, data.SubString(0x17, 2)),
                             DecodeNumber(self, data.SubString(0x19, 2)),
                             DecodeNumber(self, data.SubString(0x1b, 2)));
                    data.Delete(1, 0x1c);
                }
                self->record_list.insert(self->record_list.end(), record);
            } while (data.Length() > 9);
            self->loaded = 1;
        }
        catch (...)
        {
            FileClose(file_handle);
            self->loaded = 0;
        }
    }
}

void LearnValues::AddSkill(LearnValues *self,
                           LearnValue *record,
                           int skill_id,
                           int level_requirement,
                           int class_requirement,
                           int price,
                           int skill_requirement_1,
                           int skill_requirement_2,
                           int skill_requirement_3,
                           int skill_requirement_4,
                           int str_requirement,
                           int int_requirement,
                           int wis_requirement,
                           int agi_requirement,
                           int con_requirement,
                           int cha_requirement)
{
    record->AddSkill(skill_id,
                     level_requirement,
                     class_requirement,
                     price,
                     skill_requirement_1,
                     skill_requirement_2,
                     skill_requirement_3,
                     skill_requirement_4,
                     str_requirement,
                     int_requirement,
                     wis_requirement,
                     agi_requirement,
                     con_requirement,
                     cha_requirement);
}

void LearnValues::Clear(LearnValues *self)
{
    std::vector<LearnValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        it->skills.clear();
        it++;
    }
    if (GetCount(self) >= 1)
    {
        self->record_list.clear();
        self->field_0x24 = -1;
    }
}

bool LearnValues::HasSkill(LearnValues *self, int master_id, int skill_id)
{
    bool result = false;
    if (master_id > 0 && master_id <= (int)self->record_list.size())
    {
        if (self->record_list[master_id - 1].skills.size() > 0)
        {
            std::vector<LearnItemVal>::iterator it =
                self->record_list[master_id - 1].skills.begin();
            while (it != self->record_list[master_id - 1].skills.end())
            {
                if (skill_id == it->id)
                {
                    result = true;
                    break;
                }
                it++;
            }
        }
    }
    return result;
}

LearnItemVal LearnValues::GetSkill(LearnValues *self, int master_id, int skill_id)
{
    LearnItemVal result;
    if (master_id > 0 && master_id <= (int)self->record_list.size())
    {
        if (self->record_list[master_id - 1].skills.size() > 0)
        {
            std::vector<LearnItemVal>::iterator it =
                self->record_list[master_id - 1].skills.begin();
            while (it != self->record_list[master_id - 1].skills.end())
            {
                if (skill_id == it->id)
                {
                    result = *it;
                    break;
                }
                it++;
            }
        }
    }
    return result;
}

String LearnValues::BuildOpenData(LearnValues *self, int behavior_id)
{
    String data = "";
    std::vector<LearnValue>::iterator it = self->record_list.begin();
    std::vector<LearnItemVal>::iterator skill_iter;
    while (it != self->record_list.end())
    {
        if (it->id == behavior_id)
        {
            data += EncodeNumber(self, it->id, 2);
            data.Insert(it->name, data.Length() + 1);
            data.Insert((char)-1, data.Length() + 1);
            if (it->skills.size() > 0)
                for (skill_iter = it->skills.begin(); skill_iter != it->skills.end();
                     skill_iter++)
                {
                    data.Insert(EncodeNumber(self, skill_iter->id, 2), data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->level_requirement, 1),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->class_requirement, 1),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->price, 4),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->skill_requirement_1, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->skill_requirement_2, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->skill_requirement_3, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->skill_requirement_4, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->str_requirement, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->int_requirement, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->wis_requirement, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->agi_requirement, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->con_requirement, 2),
                                data.Length() + 1);
                    data.Insert(EncodeNumber(self, skill_iter->cha_requirement, 2),
                                data.Length() + 1);
                }
        }
        it++;
    }
    if (data == "")
        data += EncodeNumber(self, 0, 2);
    return data;
}

unsigned int LearnValues::GetCount(LearnValues *self)
{
    return self->record_list.size();
}

String LearnValues::EncodeNumber(LearnValues *self, unsigned int value, int width)
{
    unsigned int n = value;
    String result = "";
    try
    {
        unsigned int q;
        do
        {
            int r;
            char c;
            double d = n / 253.0;
            q = (int)d;
            r = n % EO_NUM_MAX;
            c = r + 1;
            result.Insert(c, result.Length() + 1);
            if (q >= 1)
                n = q;
        } while (q >= 1);
    }
    catch (...)
    {
        result += "";
    }
    if (result.Length() < width)
    {
        char pad = (char)-2;
        int count = width - result.Length();
        for (int i = 0; i < count; i++)
            result.Insert(pad, result.Length() + 1);
    }
    return result;
}

int LearnValues::DecodeNumber(LearnValues *self, String value)
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
            if (ch == EO_NUM_EMPTY)
                break;
            int n = ch;
            n = n - 1;
            if (byte_index == 1)
                result = result + n;
            if (byte_index == 2)
                result = result + n * EO_NUM_MAX;
            if (byte_index == 3)
                result = result + n * EO_NUM_MAX_2;
            if (byte_index == 4)
                result = result + n * EO_NUM_MAX_3;
            byte_index = byte_index + 1;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}
