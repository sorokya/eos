#ifndef PlayerskillH
#define PlayerskillH

#include <Classes.hpp>

// Recovered from the reference (PlayerSkill unit, 0x411f84..0x411fcd): the
// constructor stores its argument at offset 0 and the destructor is emitted
// out-of-line as the deleting form. Callers fill offset 4, so sizeof is 8; the
// field roles (skill id / level) come from those cross-unit call sites. The class
// name is a placeholder (internal names are unobservable in the stripped image).
struct PlayerSkill
{
    int skill_id;
    unsigned int level;

    PlayerSkill(int id);
    ~PlayerSkill();
};

#endif
