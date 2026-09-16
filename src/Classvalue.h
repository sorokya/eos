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
// The element size observed from the owning vector is 0x1C bytes. Offsets 0 and
// 8 are verified from the unit's own code; the remaining fields follow the ECF
// record order and are not individually confirmed yet.
struct ClassValue
{
    int id;
    int field_4;
    String name;
    short str;
    short intl;
    short wis;
    short agi;
    short con;
    short cha;
    short parent_type;
    short stat_group;

    ClassValue(int id);
    ~ClassValue();
};

#endif
