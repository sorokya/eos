#include <vcl.h>
#pragma hdrstop

#include "Skillvalues.h"
#include "Protocol.h"

#pragma package(smart_init)

SkillValues::~SkillValues()
{
}

void SkillValues::LoadSpells(SkillValues *self)
{
    if (self->loaded == 0)
    {
        int file = 1;
        int count = 0;
        int total = 1;

        do
        {
            String data;
            String path = "./pub/dsl";
            if (file < 10)
                path = path + "00" + IntToStr(file) + ".esf";
            else
                path = path + "0" + IntToStr(file) + ".esf";

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
                if (data[1] != 'E' || data[2] != 'S' || data[3] != 'F')
                    return;
                self->string_list->Add(data);
                if (file == 1)
                {
                    self->rid_1 = self->DecodeNumber(data.SubString(4, 2));
                    self->rid_2 = self->DecodeNumber(data.SubString(6, 2));
                    int parsed = self->DecodeNumber(data.SubString(8, 2));
                    int version = self->DecodeNumber(data.SubString(10, 1));
                    total = parsed;
                    self->num_records = parsed;
                }
                data.Delete(1, 10);
                for (int j = 0; count < total && j < 900; j++)
                {
                    int namelen = self->DecodeNumber(data.SubString(1, 1));
                    int chantlen = self->DecodeNumber(data.SubString(2, 1));
                    int base = namelen + chantlen + 2;
                    AddRecord(self,
                              self->GetCount() + 1,
                              data.SubString(3, namelen),
                              data.SubString(namelen + 3, chantlen),
                              self->DecodeNumber(data.SubString(base + 1, 2)),
                              self->DecodeNumber(data.SubString(base + 3, 2)),
                              self->DecodeNumber(data.SubString(base + 5, 2)),
                              self->DecodeNumber(data.SubString(base + 7, 2)),
                              self->DecodeNumber(data.SubString(base + 9, 1)),
                              self->DecodeNumber(data.SubString(base + 0xa, 1)),
                              self->DecodeNumber(data.SubString(base + 0xb, 1)),
                              self->DecodeNumber(data.SubString(base + 0xc, 3)),
                              self->DecodeNumber(data.SubString(base + 0xf, 1)),
                              self->DecodeNumber(data.SubString(base + 0x10, 2)),
                              self->DecodeNumber(data.SubString(base + 0x12, 1)),
                              self->DecodeNumber(data.SubString(base + 0x13, 1)),
                              self->DecodeNumber(data.SubString(base + 0x14, 1)),
                              self->DecodeNumber(data.SubString(base + 0x15, 1)),
                              self->DecodeNumber(data.SubString(base + 0x16, 2)),
                              self->DecodeNumber(data.SubString(base + 0x18, 2)),
                              self->DecodeNumber(data.SubString(base + 0x1a, 2)),
                              self->DecodeNumber(data.SubString(base + 0x1c, 2)),
                              self->DecodeNumber(data.SubString(base + 0x1e, 2)),
                              self->DecodeNumber(data.SubString(base + 0x20, 2)),
                              self->DecodeNumber(data.SubString(base + 0x22, 1)),
                              self->DecodeNumber(data.SubString(base + 0x23, 2)),
                              self->DecodeNumber(data.SubString(base + 0x25, 2)),
                              self->DecodeNumber(data.SubString(base + 0x27, 1)),
                              self->DecodeNumber(data.SubString(base + 0x28, 2)),
                              self->DecodeNumber(data.SubString(base + 0x2a, 2)),
                              self->DecodeNumber(data.SubString(base + 0x2c, 2)),
                              self->DecodeNumber(data.SubString(base + 0x2e, 2)),
                              self->DecodeNumber(data.SubString(base + 0x30, 2)),
                              self->DecodeNumber(data.SubString(base + 0x32, 2)));
                    count++;
                    data.Delete(1, base + 0x33);
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

void SkillValues::AddRecord(SkillValues *self,
                            int id,
                            String name,
                            String chant,
                            short icon_id,
                            short graphic_id,
                            short tp_cost,
                            short sp_cost,
                            short cast_time,
                            short nature,
                            short unknown1,
                            short skill_type,
                            short element,
                            short element_power,
                            short target_restrict,
                            short target_type,
                            short target_time,
                            short skill_range_area,
                            short max_skill_level,
                            short min_damage,
                            short max_damage,
                            short accuracy,
                            short evade,
                            short armor,
                            short return_damage,
                            short hp_heal,
                            short tp_heal,
                            short sp_heal,
                            short str,
                            short intl,
                            short wis,
                            short agi,
                            short con,
                            short cha)
{
    SkillValue v(id);
    v.name = name;
    v.chant = chant;
    v.icon_id = icon_id;
    v.graphic_id = graphic_id;
    v.max_skill_level = max_skill_level;
    v.tp_cost = tp_cost;
    v.sp_cost = sp_cost;
    v.cast_time = cast_time;
    v.nature = nature;
    v.unknown1 = unknown1;
    v.skill_type = skill_type;
    v.element = element;
    v.element_power = element_power;
    v.target_restrict = target_restrict;
    v.target_type = target_type;
    v.target_time = target_time;
    v.skill_range_area = skill_range_area;
    v.min_damage = min_damage;
    v.max_damage = max_damage;
    v.accuracy = accuracy;
    v.evade = evade;
    v.armor = armor;
    v.return_damage = return_damage;
    v.hp_heal = hp_heal;
    v.tp_heal = tp_heal;
    v.sp_heal = sp_heal;
    v.str = str;
    v.intl = intl;
    v.wis = wis;
    v.agi = agi;
    v.con = con;
    v.cha = cha;
    self->record_list.insert(self->record_list.end(), v);
}

SkillValues::SkillValues()
{
    field_0x18 = operator new(8);
    field_0x3c = -1;
    loaded = 0;
    file_id = 0;
    string_list = new TStringList;
    LoadSpells(this);
}

int SkillValues::GetCount()
{
    return record_list.size();
}

SkillDamage SkillValues::GetDamage(SkillValues *self, int skill_id)
{
    SkillDamage result;
    result.min_damage = 0;
    result.max_damage = 0;
    if (skill_id > 0)
    {
        if (self->GetCount() > skill_id)
        {
            result.min_damage = self->record_list[skill_id - 1].min_damage;
            result.max_damage = self->record_list[skill_id - 1].max_damage;
        }
    }
    return result;
}

SkillElement SkillValues::GetElement(SkillValues *self, int skill_id)
{
    SkillElement result;
    result.element = Element_None;
    result.element_power = 0;
    if (skill_id > 0)
    {
        if (self->GetCount() > skill_id)
        {
            result.element = self->record_list[skill_id - 1].element;
            result.element_power = self->record_list[skill_id - 1].element_power;
        }
    }
    return result;
}

int SkillValues::GetTargetType(SkillValues *self, int skill_id)
{
    int result = SkillTargetType_Normal;
    if (skill_id > 0)
    {
        if (self->GetCount() > skill_id)
        {
            result = self->record_list[skill_id - 1].target_type;
            if (result < 0)
                result = SkillTargetType_Normal;
        }
    }
    return result;
}

int SkillValues::GetSkillType(SkillValues *self, int skill_id)
{
    int result = SkillType_Heal;
    if (skill_id > 0)
    {
        if (self->GetCount() > skill_id)
        {
            result = self->record_list[skill_id - 1].skill_type;
            if (result < 0)
                result = SkillType_Heal;
        }
    }
    return result;
}

int SkillValues::GetTpCost(SkillValues *self, int skill_id)
{
    int result = 0;
    if (skill_id > 0)
    {
        if (self->GetCount() > skill_id)
        {
            result = self->record_list[skill_id - 1].tp_cost;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int SkillValues::GetHpHeal(SkillValues *self, int skill_id)
{
    int result = 0;
    if (skill_id > 0)
    {
        if (self->GetCount() > skill_id)
        {
            result = self->record_list[skill_id - 1].hp_heal;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int SkillValues::GetCastTime(SkillValues *self, int skill_id)
{
    int result = -1;
    SkillValue *it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (skill_id == it->id)
        {
            result = it->cast_time;
            break;
        }
        it++;
    }
    return result;
}

int SkillValues::DecodeNumber(String value)
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
            if (ch == EO_NUM_EMPTY || ch == 0)
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
