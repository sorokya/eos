#ifndef SkillvaluesH
#define SkillvaluesH

#include <vector.h>
#include "Skillvalue.h"

// Two-int return values of the Get* accessors below. The names are not
// recoverable from the reference (they do not appear in mangles or RTTI); the
// layouts are fixed by the accessors' stores.
struct SkillDamage
{
    struct
    {
        int min_damage;
        int max_damage;
    };
    SkillDamage()
    {
    }
};

struct SkillElement
{
    struct
    {
        int element;
        int element_power;
    };
    SkillElement()
    {
    }
};

// The skill table (ESF). Layout recovered from the reference constructor
// (0x4a3018) and the parser/accessors: sizeof(vector<SkillValue>) is 32 and
// sizeof(SkillValue) is 76, matching the reference's element size:
//   +0x00 int                        file_id
//   +0x04 int                        num_records
//   +0x08 int                        rid_1
//   +0x0c int                        rid_2
//   +0x10 char                       loaded
//   +0x14 TStringList *              string_list
//   +0x18 void *                     field_0x18 = operator new(8)
//   +0x1c vector<SkillValue>    record_list
//   +0x3c int                        field_0x3c = -1
class SkillValues
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
    vector<SkillValue> record_list;
    int field_0x3c;

    SkillValues();
    ~SkillValues();

    static void LoadSpells(SkillValues *self);
    int DecodeNumber(String value);
    int GetCount();

    static void AddRecord(SkillValues *self,
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
                          short cha);

    static SkillDamage GetDamage(SkillValues *self, int skill_id);
    static SkillElement GetElement(SkillValues *self, int skill_id);
    static int GetTargetType(SkillValues *self, int skill_id);
    static int GetSkillType(SkillValues *self, int skill_id);
    static int GetTpCost(SkillValues *self, int skill_id);
    static int GetHpHeal(SkillValues *self, int skill_id);
    static int GetCastTime(SkillValues *self, int skill_id);
};

#endif
