#ifndef ItemvaluesH
#define ItemvaluesH

#include <vector.h>
#include <Classes.hpp>
#include "Itemvalue.h"
#include "Protocol.h"

// The item table (EIF). Layout recovered from the reference constructor
// (0x47826c); sizeof(vector<ItemValue*>) is 32 and the member extent runs
// to 0x3c / the vector element type is ItemValue*:
//   +0x00 int                         file_id
//   +0x04 int                         num_records
//   +0x08 int                         rid_1
//   +0x0c int                         rid_2
//   +0x10 char                        loaded
//   +0x14 TStringList *               string_list
//   +0x18 void *                      field_0x18 = operator new(8)
//   +0x1c vector<ItemValue *>    record_list
//   +0x3c int                         field_0x3c = -1
class ItemValues
{
  public:
    int file_id;
    int num_records;
    int rid_1;
    int rid_2;
    char loaded;
    char pad_0x11[3];
    TStringList *string_list;
    void *field_0x18;
    vector<ItemValue *> record_list;
    int field_0x3c;

    ItemValues();
    ~ItemValues();

    int DecodeNumber(String value);

    static void LoadItems(ItemValues *self);
    static void Clear(ItemValues *self);
    static int GetCount(ItemValues *self);
    static ItemValue *GetByIndex(ItemValues *self, int index);
    static ItemValue **GetRecordSlot(vector<ItemValue *> *record_list, int index);
    static void AddItem(ItemValues *self,
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
                        short size);

    static int GetType(ItemValues *self, int item_id);
    static int GetSubtype(ItemValues *self, int item_id);
    static int GetSpecial(ItemValues *self, int item_id);
    static int GetWeight(ItemValues *self, int item_id);
    static int GetHP(ItemValues *self, int item_id);
    static int GetTP(ItemValues *self, int item_id);
    static int GetSpec1(ItemValues *self, int item_id);
    static int GetSpec1ForTypes(ItemValues *self, int item_id);
    static int GetLevelRequirement(ItemValues *self, int item_id);
    static int GetScrollMap(ItemValues *self, int item_id);
    static int GetGender(ItemValues *self, int item_id);
    static ItemElement GetElement(ItemValues *self, int item_id);
    static ItemSpecXY GetSpecXY(ItemValues *self, int item_id);
};

#endif
