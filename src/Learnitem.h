#ifndef LearnitemH
#define LearnitemH

#include <Classes.hpp>

// Recovered from the reference (Learnitem unit, 0x53328c..0x533300) and the
// Learnvalue AddSkill stores: an int id at offset 0, then the in-memory
// expansion of the EMF SkillMasterSkillRecord (eo-protocol
// xml/pub/server/protocol.xml): the on-disk shorts/chars are widened to their
// in-memory widths (id -> int, price -> int). The element size of
// std::vector<LearnItemVal> is 0x20 (AddSkill advances the iterator by 0x20 and
// the GetSkill copy moves 8 dwords). Class name is the RTTI type name.
struct LearnItemVal
{
    int id;
    short level_requirement;
    short class_requirement;
    int price;
    short skill_requirement_1;
    short skill_requirement_2;
    short skill_requirement_3;
    short skill_requirement_4;
    short str_requirement;
    short int_requirement;
    short wis_requirement;
    short agi_requirement;
    short con_requirement;
    short cha_requirement;

    LearnItemVal();
    LearnItemVal(int id);
    ~LearnItemVal();
};

#endif
