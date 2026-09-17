#ifndef LearnvaluesH
#define LearnvaluesH

#include <vector>
#include <Classes.hpp>
#include "Learnvalue.h"

// The skill-master table (EMF). Layout recovered from the reference constructor
// (0x530e98) and Clear (0x532528): a byte loaded flag at 0, the record_list
// vector at +4 (sizeof(std::vector<LearnValue>) is 0x20) and a dword at +0x24
// reset to -1 on clear. There is no num/rid/TStringList member (unlike
// ItemValues/SkillValues): the constructor constructs only the vector, stores
// -1 at +0x24 and 0 at 0, then calls the loader.
//   +0x00 char                        loaded
//   +0x04 std::vector<LearnValue>     record_list
//   +0x24 int                         field_24 = -1
class LearnValues
{
public:
    char loaded;
    char pad_01[3];
    std::vector<LearnValue> record_list;
    int field_24;

    LearnValues();

    static void Pub_LoadSkillMasters(LearnValues *self);
    static void Clear(LearnValues *self);
    static bool HasSkill(LearnValues *self, int master_id, int skill_id);
    static LearnItemVal GetSkill(LearnValues *self, int master_id, int skill_id);
    static String BuildOpenData(LearnValues *self, int behavior_id);
    static unsigned int GetRecordCount(LearnValues *self);
    static void AddSkill(LearnValues *self, LearnValue *record, int skill_id,
                         int level_requirement, int class_requirement, int price,
                         int skill_requirement_1, int skill_requirement_2,
                         int skill_requirement_3, int skill_requirement_4,
                         int str_requirement, int int_requirement,
                         int wis_requirement, int agi_requirement,
                         int con_requirement, int cha_requirement);
    static String Pub_EncodeNumber_Learn(LearnValues *self, unsigned int value, int width);
    static int Pub_DecodeNumber_Learn(LearnValues *self, String value);
};

#endif
