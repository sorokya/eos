#ifndef LearnvalueH
#define LearnvalueH

#include <vector>
#include <Classes.hpp>
#include "Learnitem.h"

// Recovered from the reference (Learnvalue unit, 0x5303f0..0x530e78): the
// constructor default-constructs `name` at offset 4 and the skills vector at
// offset 0x10, then stores the id at offset 0. The parser also stores
// min_level (+8), max_level (+0xa) and class_requirement (+0xc); the field at
// +0xe is unreferenced padding before the 4-byte-aligned vector.
//
//   +0x00 int                        id
//   +0x04 String                     name
//   +0x08 short                      min_level
//   +0x0a short                      max_level
//   +0x0c short                      class_requirement
//   +0x0e short                      pad
//   +0x10 std::vector<LearnItemVal>  skills
// sizeof = 0x30 (the owning record_list advances its iterator by 0x30).
//
// The SkillMasterSkillRecord fields are the in-memory expansion of the EMF
// record (eo-protocol protocol.xml): id/price widen to int, the rest stay
// short.
struct LearnValue
{
    int id;
    String name;
    short min_level;
    short max_level;
    short class_requirement;
    short pad_0e;
    std::vector<LearnItemVal> skills;

    LearnValue(int id);
    ~LearnValue();

    void AddSkill(int skill_id,
                  int level_requirement,
                  int class_requirement,
                  int price,
                  int skill_requirement_1,
                  int skill_requirement_2,
                  int skill_requirement_3,
                  int skill_requirement_4,
                  int str_requirement,
                  int int_requirement,
                  int wis_requirement,
                  int agi_requirement,
                  int con_requirement,
                  int cha_requirement);
};

#endif
