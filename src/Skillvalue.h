#ifndef SkillvalueH
#define SkillvalueH

#include <Classes.hpp>

// Recovered from the reference (Skillvalue unit, 0x4a8eb0..0x4a8f64): the
// constructor default-constructs the name and chant Strings (offsets 4 and 8)
// and then stores the skill id at offset 0; the destructor destroys chant then
// name and deletes. The class name is the RTTI type name ("SkillValue").
//
// The remaining fields are the in-memory expansion of the ESF record
// (eo-protocol EsfRecord). Each short (or widened int) was derived from the
// `Skillvalues` parser's stores and the Get* accessors; the element size of
// vector<SkillValue> is 76 bytes (0x4C), matching this layout.
struct SkillValue
{
    int id;
    String name;
    String chant;
    short max_skill_level;
    short icon_id;
    short graphic_id;
    short tp_cost;
    short sp_cost;
    short cast_time;
    short nature;
    short unknown1;
    int skill_type;
    short element;
    short element_power;
    short target_restrict;
    short target_type;
    short target_time;
    short skill_range_area;
    short min_damage;
    short max_damage;
    short accuracy;
    short evade;
    short armor;
    short return_damage;
    short hp_heal;
    short tp_heal;
    short sp_heal;
    short str;
    short intl;
    short wis;
    short agi;
    short con;
    short cha;

    SkillValue(int id);
    ~SkillValue();
};

#endif
