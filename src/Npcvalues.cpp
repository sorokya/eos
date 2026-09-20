#include <vcl.h>
#pragma hdrstop

#include "Npcvalues.h"

#pragma package(smart_init)

NpcValues::NpcValues()
{
    field_0x18 = operator new(8);
    field_0x3c = -1;
    loaded = 0;
    drops_loaded = 0;
    talk_loaded = 0;
    file_id = 0;
    string_list = new TStringList;
    Pub_LoadNpcs(this);
    Pub_LoadDrops(this);
    Pub_LoadTalk(this);
}

NpcValues::~NpcValues()
{
}

void NpcValues::Pub_LoadNpcs(NpcValues *self)
{
    if (self->loaded == 0)
    {
        int file = 1;
        int count = 0;
        int total = 1;

        do
        {
            String data;
            String path = "./pub/dtn";
            if (file < 10)
                path = path + "00" + IntToStr(file) + ".enf";
            else
                path = path + "0" + IntToStr(file) + ".enf";

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
                if (data[1] != 'E' || data[2] != 'N' || data[3] != 'F')
                    return;
                self->string_list->Add(data);
                if (file == 1)
                {
                    self->rid1 = self->DecodeNumber(data.SubString(4, 2));
                    self->rid2 = self->DecodeNumber(data.SubString(6, 2));
                    int parsed = self->DecodeNumber(data.SubString(8, 2));
                    total = parsed;
                    self->count = parsed;
                }
                data.Delete(1, 10);
                for (int i = 0; count < total && i < 900; i++)
                {
                    int namelen = self->DecodeNumber(data.SubString(1, 1)) + 1;
                    AddNpc(self,
                           GetCount(self) + 1,
                           data.SubString(2, namelen - 1),
                           self->DecodeNumber(data.SubString(namelen + 1, 2)),
                           self->DecodeNumber(data.SubString(namelen + 3, 1)),
                           self->DecodeNumber(data.SubString(namelen + 4, 2)),
                           self->DecodeNumber(data.SubString(namelen + 6, 2)),
                           self->DecodeNumber(data.SubString(namelen + 8, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0xa, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0xc, 3)),
                           self->DecodeNumber(data.SubString(namelen + 0xf, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x11, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x13, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x15, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x17, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x19, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x1b, 1)),
                           self->DecodeNumber(data.SubString(namelen + 0x1c, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x1e, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x20, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x22, 2)),
                           self->DecodeNumber(data.SubString(namelen + 0x24, 1)),
                           self->DecodeNumber(data.SubString(namelen + 0x25, 3)));
                    count++;
                    data.Delete(1, namelen + 0x27);
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

void NpcValues::Pub_LoadDrops(NpcValues *self)
{
    if (self->drops_loaded == 0)
    {
        String data;
        String path = "./pub/dtd001.edf";

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
            if (data[1] != 'E' || data[2] != 'D' || data[3] != 'F')
                return;
            data.Delete(1, 3);
            do
            {
                int npc_id = self->DecodeNumber(data.SubString(1, 2));
                int drops_count = self->DecodeNumber(data.SubString(3, 2));
                data.Delete(1, 4);
                for (int i = 0; i < drops_count; i++)
                {
                    AddDrop(self,
                            npc_id,
                            self->DecodeNumber(data.SubString(1, 2)),
                            self->DecodeNumber(data.SubString(3, 3)),
                            self->DecodeNumber(data.SubString(6, 3)),
                            self->DecodeNumber(data.SubString(9, 2)));
                    data.Delete(1, 10);
                }
            } while (data.Length() > 3);
            self->drops_loaded = 1;
        }
        catch (...)
        {
            FileClose(file_handle);
            self->drops_loaded = 0;
        }
    }
}

void NpcValues::Pub_LoadTalk(NpcValues *self)
{
    if (self->talk_loaded == 0)
    {
        String data;
        String path = "./pub/ttd001.etf";

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
            if (data[1] != 'E' || data[2] != 'T' || data[3] != 'F')
                return;
            data.Delete(1, 3);
            do
            {
                int npc_id = self->DecodeNumber(data.SubString(1, 2));
                int rate = self->DecodeNumber(data.SubString(3, 1));
                int messages_count = self->DecodeNumber(data.SubString(4, 1));
                data.Delete(1, 4);
                for (int i = 0; i < messages_count; i++)
                {
                    int message_length = self->DecodeNumber(data.SubString(1, 1));
                    SetTalk(self, npc_id, rate, data.SubString(2, message_length));
                    data.Delete(1, message_length + 1);
                }
            } while (data.Length() > 3);
            self->talk_loaded = 1;
        }
        catch (...)
        {
            FileClose(file_handle);
            self->talk_loaded = 0;
        }
    }
}

void NpcValues::AddDrop(
    NpcValues *self, int npc_id, int item_id, int min_amount, int max_amount, int rate)
{
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (npc_id == it->id)
        {
            it->AddDrop(item_id, min_amount, max_amount, rate);
            break;
        }
        it++;
    }
}

void NpcValues::SetTalk(NpcValues *self, int npc_id, int rate, String message)
{
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (npc_id == it->id)
        {
            it->AddTalkMessage(rate, message);
            break;
        }
        it++;
    }
}

String NpcValues::RollTalk(NpcValues *self, int enf_id)
{
    String result = "";
    if (enf_id > 0)
    {
        if (self->record_list.size() >= enf_id)
        {
            if (self->record_list[enf_id - 1].has_talk != 0)
            {
                int count = self->record_list[enf_id - 1].talk_lines.size();
                if (count > 0)
                {
                    int roll = RandRange(100);
                    if (roll < self->record_list[enf_id - 1].talk_rate)
                    {
                        roll = RandRange(count);
                        result = self->record_list[enf_id - 1].talk_lines[roll];
                    }
                }
            }
        }
    }
    return result;
}

NpcDropInfo NpcValues::GetDrop(NpcValues *self, int npc_id)
{
    NpcDropInfo result;
    result.item_id = 0;
    result.amount = 0;
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (it->id == npc_id)
        {
            int roll = RandRange(64000);
            int sum = 0;
            std::vector<NpcDropItem>::iterator drop = it->drops.begin();
            while (drop != it->drops.end())
            {
                sum = sum + drop->rate;
                if (sum >= roll)
                {
                    if (drop->min_amount > 0)
                    {
                        int range = drop->max_amount - drop->min_amount;
                        result.item_id = drop->item_id;
                        if (range > 0)
                            result.amount = RandRange(range + 1) + drop->min_amount;
                        else
                            result.amount = drop->min_amount;
                    }
                    break;
                }
                drop++;
            }
        }
        it++;
    }
    return result;
}

void NpcValues::AddNpc(NpcValues *self,
                       int id,
                       String name,
                       short graphic_id,
                       short race,
                       short boss,
                       short child,
                       short npc_type,
                       short behavior_id,
                       int hp,
                       short tp,
                       short min_damage,
                       short max_damage,
                       short accuracy,
                       short evade,
                       short armor,
                       short return_damage,
                       short element,
                       short element_damage,
                       short element_weakness,
                       short element_weakness_damage,
                       short level,
                       int experience)
{
    NpcValue value(id);
    value.id = id;
    value.name = name;
    value.graphic_id = graphic_id;
    value.race = race;
    value.boss = boss;
    value.child = child;
    value.npc_type = npc_type;
    value.behavior_id = behavior_id;
    value.hp = hp;
    value.tp = tp;
    value.min_damage = min_damage;
    value.max_damage = max_damage;
    value.accuracy = accuracy;
    value.evade = evade;
    value.armor = armor;
    value.return_damage = return_damage;
    value.element = element;
    value.element_damage = element_damage;
    value.element_weakness = element_weakness;
    value.element_weakness_damage = element_weakness_damage;
    value.level = level;
    value.experience = experience;
    if (value.hp < 1)
        value.hp = 1;
    self->record_list.insert(self->record_list.end(), value);
}

int NpcValues::GetCount(NpcValues *self)
{
    return self->record_list.size();
}

NpcValue NpcValues::GetNpc(NpcValues *self, int id)
{
    NpcValue result;
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    NpcValue *value;
    while (it != self->record_list.end())
    {
        if (it->id == id)
        {
            value = it;
            result.name = value->name;
            result.drops = value->drops;
            result.talk_lines = value->talk_lines;
            result.id = value->id;
            result.graphic_id = value->graphic_id;
            result.race = value->race;
            result.boss = value->boss;
            result.child = value->child;
            result.npc_type = value->npc_type;
            result.behavior_id = value->behavior_id;
            result.hp = value->hp;
            result.tp = value->tp;
            result.min_damage = value->min_damage;
            result.max_damage = value->max_damage;
            result.accuracy = value->accuracy;
            result.evade = value->evade;
            result.armor = value->armor;
            result.return_damage = value->return_damage;
            result.element = value->element;
            result.element_damage = value->element_damage;
            result.element_weakness = value->element_weakness;
            result.element_weakness_damage = value->element_weakness_damage;
            result.level = value->level;
            result.experience = value->experience;
            result.talk_rate = value->talk_rate;
            result.has_talk = value->has_talk;
        }
        it++;
    }
    return result;
}

NpcTypeInfo NpcValues::GetType(NpcValues *self, int enf_id)
{
    NpcTypeInfo result;
    result.type = -1;
    result.behavior_id = -1;
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (enf_id == it->id)
        {
            result.type = it->npc_type;
            result.behavior_id = it->behavior_id;
            break;
        }
        it++;
    }
    return result;
}

int NpcValues::GetExp(NpcValues *self, int npc_id)
{
    int result = 0;
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (npc_id == it->id)
        {
            result = it->experience;
            break;
        }
        it++;
    }
    return result;
}

int NpcValues::GetMaxHp(NpcValues *self, int npc_id)
{
    int result = 1;
    std::vector<NpcValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        if (npc_id == it->id)
        {
            result = it->hp;
            break;
        }
        it++;
    }
    return result;
}

int NpcValues::DecodeNumber(String value)
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
            if (ch == 0xfe || ch == 0)
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

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
#pragma warn.8057
// END GENERATED STUBS
