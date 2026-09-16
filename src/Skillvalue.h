#ifndef SkillvalueH
#define SkillvalueH

#include <Classes.hpp>

// Recovered from the reference (Skillvalue unit, 0x4a8eb0..0x4a8f64): the
// constructor default-constructs the name and chant Strings (offsets 4 and 8)
// and then stores the skill id at offset 0; the destructor destroys chant then
// name and deletes. The class name is the RTTI type name ("SkillValue").
// The remaining EsfRecord fields (icon_id..cha) sit after offset 8 and are
// pending the Skillvalues parser.
struct SkillValue
{
    int id;
    String name;
    String chant;

    SkillValue(int id);
    ~SkillValue();
};

#endif
