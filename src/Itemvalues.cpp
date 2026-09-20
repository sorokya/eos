#include <vcl.h>
#pragma hdrstop

#include "Itemvalues.h"
#include "Protocol.h"

#pragma package(smart_init)

ItemValues::ItemValues()
{
    field_0x18 = operator new(8);
    field_0x3c = -1;
    loaded = 0;
    rid_1 = -1;
    rid_2 = -1;
    file_id = 0;
    string_list = new TStringList;
    LoadItems(this);
}

ItemValues::~ItemValues()
{
}

void ItemValues::LoadItems(ItemValues *self)
{
    if (self->loaded == 0)
    {
        self->string_list->Clear();
        Clear(self);
        int file = 1;
        int count = 0;
        int total = 1;

        do
        {
            String path;
            String data;
            path.Insert("./pub/", 0);
            path.Insert("dat", path.Length() + 1);
            if (file < 10)
                path = path + "00" + IntToStr(file) + ".eif";
            else
                path = path + "0" + IntToStr(file) + ".eif";

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
                if (data[1] != 'E' || data[2] != 'I' || data[3] != 'F')
                    return;
                self->string_list->Add(data);
                if (file == 1)
                {
                    self->rid_1 = self->DecodeNumber(data.SubString(4, 2));
                    self->rid_2 = self->DecodeNumber(data.SubString(6, 2));
                    int parsed = self->DecodeNumber(data.SubString(8, 2));
                    int version = self->DecodeNumber(data.SubString(10, 1));
                    total = parsed;
                    self->num_items = parsed;
                }
                data.Delete(1, 10);
                for (int j = 0; count < total && j < 900; j++)
                {
                    int namelen = self->DecodeNumber(data.SubString(1, 1)) + 1;
                    AddItem(self,
                            GetCount(self) + 1,
                            data.SubString(2, namelen - 1),
                            self->DecodeNumber(data.SubString(namelen + 1, 2)),
                            self->DecodeNumber(data.SubString(namelen + 3, 1)),
                            self->DecodeNumber(data.SubString(namelen + 4, 1)),
                            self->DecodeNumber(data.SubString(namelen + 5, 1)),
                            self->DecodeNumber(data.SubString(namelen + 6, 2)),
                            self->DecodeNumber(data.SubString(namelen + 8, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0xa, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0xc, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0xe, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x10, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x12, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x14, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x15, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x16, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x17, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x18, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x19, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x1a, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x1b, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x1c, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x1d, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x1e, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x1f, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x20, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x21, 3)),
                            self->DecodeNumber(data.SubString(namelen + 0x24, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x25, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x26, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x28, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x29, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x2c, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x2e, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x30, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x32, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x34, 2)),
                            self->DecodeNumber(data.SubString(namelen + 0x36, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x37, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x38, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x39, 1)),
                            self->DecodeNumber(data.SubString(namelen + 0x3a, 1)));
                    count++;
                    data.Delete(1, namelen + 0x3a);
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

void ItemValues::AddItem(ItemValues *self,
                         int id,
                         String name,
                         int graphic_id,
                         short type,
                         short subtype,
                         short special,
                         short hp,
                         short tp,
                         short min_damage,
                         short max_damage,
                         short accuracy,
                         short evade,
                         short armor,
                         short return_damage,
                         short strength,
                         short intelligence,
                         short wisdom,
                         short agility,
                         short constitution,
                         short charisma,
                         short light_resistance,
                         short dark_resistance,
                         short earth_resistance,
                         short air_resistance,
                         short water_resistance,
                         short fire_resistance,
                         int spec1,
                         short spec2,
                         short spec3,
                         short level_requirement,
                         short class_requirement,
                         short strength_requirement,
                         short intelligence_requirement,
                         short wisdom_requirement,
                         short agility_requirement,
                         short constitution_requirement,
                         short charisma_requirement,
                         short element,
                         short element_damage,
                         short weight,
                         short weapon_target_area,
                         short size)
{
    ItemValue *value = new ItemValue(id);
    value->type = type;
    value->subtype = subtype;
    value->special = special;
    value->hp = hp;
    value->tp = tp;
    value->min_damage = min_damage;
    value->max_damage = max_damage;
    value->accuracy = accuracy;
    value->evade = evade;
    value->armor = armor;
    value->return_damage = return_damage;
    value->strength = strength;
    value->intelligence = intelligence;
    value->wisdom = wisdom;
    value->agility = agility;
    value->constitution = constitution;
    value->charisma = charisma;
    value->light_resistance = light_resistance;
    value->dark_resistance = dark_resistance;
    value->earth_resistance = earth_resistance;
    value->air_resistance = air_resistance;
    value->water_resistance = water_resistance;
    value->fire_resistance = fire_resistance;
    value->spec1 = spec1;
    value->spec2 = spec2;
    value->spec3 = spec3;
    value->level_requirement = level_requirement;
    value->class_requirement = class_requirement;
    value->strength_requirement = strength_requirement;
    value->intelligence_requirement = intelligence_requirement;
    value->wisdom_requirement = wisdom_requirement;
    value->agility_requirement = agility_requirement;
    value->constitution_requirement = constitution_requirement;
    value->charisma_requirement = charisma_requirement;
    value->size = size;
    value->weapon_target_area = weapon_target_area;
    value->weight = weight;
    value->element = element;
    value->element_damage = element_damage;
    self->record_list.insert(self->record_list.end(), value);
}

void ItemValues::Clear(ItemValues *self)
{
    std::vector<ItemValue *>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        ItemValue *value = *it;
        it = self->record_list.erase(it);
        delete value;
    }
    self->field_0x3c = -1;
}

ItemValue **ItemValues::GetRecordSlot(std::vector<ItemValue *> *record_list, int index)
{
    return record_list->begin() + index;
}

ItemValue *ItemValues::GetByIndex(ItemValues *self, int index)
{
    if (index < 0 || (unsigned)index > self->record_list.size() - 1)
        index = 0;
    return *GetRecordSlot(&self->record_list, index);
}

int ItemValues::GetCount(ItemValues *self)
{
    return self->record_list.size();
}

int ItemValues::Eif_GetSpec1ForTypes(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            if ((*GetRecordSlot(&self->record_list, item_id - 1))->type >=
                ItemType_Weapon)
            {
                if ((*GetRecordSlot(&self->record_list, item_id - 1))->type <=
                    ItemType_Bracer)
                {
                    result = (*GetRecordSlot(&self->record_list, item_id - 1))->spec1;
                    if (result < 0)
                        result = 0;
                }
            }
        }
    }
    return result;
}

ItemElement ItemValues::Eif_GetElement(ItemValues *self, int item_id)
{
    ItemElement result;
    result.element = Element_None;
    result.element_damage = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            if ((*GetRecordSlot(&self->record_list, item_id - 1))->type ==
                ItemType_Weapon)
            {
                result.element =
                    (*GetRecordSlot(&self->record_list, item_id - 1))->element;
                result.element_damage =
                    (*GetRecordSlot(&self->record_list, item_id - 1))->element_damage;
            }
        }
    }
    return result;
}

ItemSpecXY ItemValues::Eif_GetSpecXY(ItemValues *self, int item_id)
{
    ItemSpecXY result;
    result.spec2 = -1;
    result.spec3 = -1;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            if ((*GetRecordSlot(&self->record_list, item_id - 1))->type ==
                ItemType_Teleport)
            {
                result.spec2 = (*GetRecordSlot(&self->record_list, item_id - 1))->spec2;
                result.spec3 = (*GetRecordSlot(&self->record_list, item_id - 1))->spec3;
            }
        }
    }
    return result;
}

int ItemValues::Eif_GetSpec1(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->spec1;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int ItemValues::Eif_GetScrollMap(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            if ((*GetRecordSlot(&self->record_list, item_id - 1))->type ==
                ItemType_Teleport)
            {
                result = (*GetRecordSlot(&self->record_list, item_id - 1))->spec1;
                if (result < 0)
                    result = 0;
            }
        }
    }
    return result;
}

int ItemValues::Eif_GetGender(ItemValues *self, int item_id)
{
    int result = -1;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            if ((*GetRecordSlot(&self->record_list, item_id - 1))->type == ItemType_Armor)
            {
                result = (*GetRecordSlot(&self->record_list, item_id - 1))->spec2;
                if (result < 0)
                    result = 0;
            }
        }
    }
    return result;
}

int ItemValues::Eif_GetType(ItemValues *self, int item_id)
{
    int result = ItemType_General;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->type;
            if (result < 0)
                result = ItemType_General;
        }
    }
    return result;
}

int ItemValues::Eif_GetSubtype(ItemValues *self, int item_id)
{
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
            return (*GetRecordSlot(&self->record_list, item_id - 1))->subtype;
    }
    return ItemSubtype_None;
}

int ItemValues::Eif_GetLevelRequirement(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->level_requirement;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int ItemValues::Eif_GetSpecial(ItemValues *self, int item_id)
{
    int result = -1;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->special;
            if (result < 0)
                result = ItemSpecial_Normal;
        }
    }
    return result;
}

int ItemValues::Eif_GetWeight(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->weight;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int ItemValues::Eif_GetHP(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->hp;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int ItemValues::Eif_GetTP(ItemValues *self, int item_id)
{
    int result = 0;
    if (item_id > 0)
    {
        if (item_id < GetCount(self))
        {
            result = (*GetRecordSlot(&self->record_list, item_id - 1))->tp;
            if (result < 0)
                result = 0;
        }
    }
    return result;
}

int ItemValues::DecodeNumber(String value)
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
