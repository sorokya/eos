#ifndef ItemvaluesH
#define ItemvaluesH

#include <vector>
#include <Classes.hpp>
#include "Itemvalue.h"

// Two-int result records returned by value from the EIF accessors that expose a
// pair of fields (element id/power, scroll spec2/spec3). The reference calls an
// empty user constructor on the local at entry (the folded EH-frame-only
// constructor at 0x44f58c) and then moves the record to the caller's return slot
// with a single memcpy-style copy; bcc32 emits that copy only when the two
// fields sit in one anonymous aggregate member (multiple scalar members yield a
// different, member-wise copy).
struct ItemElement
{
    struct { int element; int element_damage; };
    ItemElement() {}
};

struct ItemSpecXY
{
    struct { int spec2; int spec3; };
    ItemSpecXY() {}
};

// The item table (EIF). Layout recovered from the reference constructor
// (0x47826c); sizeof(std::vector<ItemValue*>) is 32 and the member extent runs
// to 0x3c / the vector element type is ItemValue*:
//   +0x00 int                         field_0
//   +0x04 int                         num_items
//   +0x08 int                         rid_1
//   +0x0c int                         rid_2
//   +0x10 char                        loaded
//   +0x14 TStringList *               field_14
//   +0x18 void *                      field_18 = operator new(8)
//   +0x1c std::vector<ItemValue *>    values
//   +0x3c int                         field_3c = -1
class ItemValues
{
public:
    int field_0;
    int num_items;
    int rid_1;
    int rid_2;
    char loaded;
    char pad_11[3];
    TStringList *field_14;
    void *field_18;
    std::vector<ItemValue *> values;
    int field_3c;

    ItemValues();
    ~ItemValues();

    int DecodeNumber(String value);

    static void LoadItems(ItemValues *self);
    static void Clear(ItemValues *self);
    static int GetCount(ItemValues *self);
    static ItemValue *GetByIndex(ItemValues *self, int index);
    static ItemValue **GetRecordSlot(std::vector<ItemValue *> *values, int index);
    static void AddItem(ItemValues *self, int id, String name, int graphic_id,
                        short type, short subtype, short special, short hp, short tp,
                        short min_damage, short max_damage, short accuracy, short evade,
                        short armor, short return_damage, short strength,
                        short intelligence, short wisdom, short agility,
                        short constitution, short charisma, short light_resistance,
                        short dark_resistance, short earth_resistance,
                        short air_resistance, short water_resistance,
                        short fire_resistance, int spec1, short spec2, short spec3,
                        short level_requirement, short class_requirement,
                        short strength_requirement, short intelligence_requirement,
                        short wisdom_requirement, short agility_requirement,
                        short constitution_requirement, short charisma_requirement,
                        short element, short element_damage, short weight,
                        short unused, short size);

    static int Eif_GetType(ItemValues *self, int item_id);
    static int Eif_GetSubtype(ItemValues *self, int item_id);
    static int Eif_GetSpecial(ItemValues *self, int item_id);
    static int Eif_GetWeight(ItemValues *self, int item_id);
    static int Eif_GetHP(ItemValues *self, int item_id);
    static int Eif_GetTP(ItemValues *self, int item_id);
    static int Eif_GetSpec1(ItemValues *self, int item_id);
    static int Eif_GetSpec1ForTypes(ItemValues *self, int item_id);
    static int Eif_GetLevelRequirement(ItemValues *self, int item_id);
    static int Eif_GetScrollMap(ItemValues *self, int item_id);
    static int Eif_GetGender(ItemValues *self, int item_id);
    static ItemElement Eif_GetElement(ItemValues *self, int item_id);
    static ItemSpecXY Eif_GetSpecXY(ItemValues *self, int item_id);
};

#endif
