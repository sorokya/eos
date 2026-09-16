#ifndef ClassValuesH
#define ClassValuesH

#include <vector>
#include "Classvalue.h"

// The class table (ECF). Layout recovered from the reference constructor
// (0x535eec); sizeof(std::vector<ClassValue>) is 32 and sizeof(ClassValue) is
// 28, matching the reference's element size and member extent:
//   +0x00 int                       field_0
//   +0x10 char                      field_10
//   +0x14 int                       field_14
//   +0x18 void *                    field_18 = operator new(8)
//   +0x1c std::vector<ClassValue>   values
//   +0x3c int                       field_3c = -1
// The offsets 4 and 8 are unnamed padding pending evidence.
class ClassValues
{
public:
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    char field_10;
    char pad_11[3];
    TStringList *field_14;
    void *field_18;
    std::vector<ClassValue> values;
    int field_3c;

    ClassValues();
    ~ClassValues();

    void LoadClasses();
    int DecodeInt(String str);

    static ClassValue GetByIndex(ClassValues *self, int index);
    static void AddClass(ClassValues *self, int id, int field_4, String name,
                         short f0c, short f0e, short f10, short f12,
                         short f14, short f16, short f18);
};

#endif
