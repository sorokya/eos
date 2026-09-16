#ifndef LearnitemH
#define LearnitemH

#include <Classes.hpp>

// Recovered from the reference (Learnitem unit, 0x53328c..0x533300). A class
// with an int member at offset 0 and two constructors (a no-op default and one
// storing the id) plus the deleting destructor. It is the element type of
// LearnValue's skill vector (std::vector<LearnItemVal> per the RTTI). Remaining
// SkillMasterSkillRecord fields (level_requirement..cha_requirement) are pending
// the Learnvalues parser. Class name is the RTTI type name ("LearnItemVal").
struct LearnItemVal
{
    int id;

    LearnItemVal();
    LearnItemVal(int id);
    ~LearnItemVal();
};

#endif
