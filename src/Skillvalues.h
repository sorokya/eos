#ifndef SkillvaluesH
#define SkillvaluesH

#include <vector>
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
// (0x4a3018) and the parser/accessors: sizeof(std::vector<SkillValue>) is 32 and
// sizeof(SkillValue) is 76, matching the reference's element size:
//   +0x00 int                        file_id
//   +0x04 int                        num_skills
//   +0x08 int                        rid1
//   +0x0c int                        rid2
//   +0x10 char                       loaded
//   +0x14 TStringList *              string_list
//   +0x18 void *                     field_0x18 = operator new(8)
//   +0x1c std::vector<SkillValue>    record_list
//   +0x3c int                        field_0x3c = -1
class SkillValues
{
  public:
    int file_id;
    int num_skills;
    int rid1;
    int rid2;
    char loaded;
    char pad_0x11[3];
    TStringList *string_list;
    void *field_0x18;
    std::vector<SkillValue> record_list;
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
