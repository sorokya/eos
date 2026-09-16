#ifndef ClassValueH
#define ClassValueH

#include <Classes.hpp>

// Recovered from the reference (ClassValue unit, 0x537480..0x53750f) and the ECF
// file format:
//   ECF    = "ECF", short rid[2], short total_classes_count, char version,
//            EcfRecord classes[]
//   EcfRecord = char name_length, char name[name_length], char parent_type,
//               char stat_group, short str/intl/wis/agi/con/cha
// The constructor stores the class id at offset 0 and default-constructs the
// String at offset 8; the destructor destroys that String and then deletes.
// The element size observed from the owning vector is 0x1C bytes, and the copy
// performed by Classvalues::GetByIndex copies seven shorts at 0xC..0x1A (plus 2
// bytes of tail padding). Offsets 0 and 8 are verified from the unit's own code;
// the individual short meanings are not confirmed yet.
struct ClassValue
{
    int id;
    int field_4;
    String name;
    short f0c;
    short f0e;
    short f10;
    short f12;
    short f14;
    short f16;
    short f18;

    ClassValue(int id);
    ~ClassValue();
};

#endif
