#ifndef ClassValuesH
#define ClassValuesH

#include <vector>
#include "Classvalue.h"

// The class table (ECF). Layout recovered from the reference constructor
// (0x535eec); sizeof(std::vector<ClassValue>) is 32 and sizeof(ClassValue) is
// 28, matching the reference's element size and member extent:
//   +0x00 int                       field_0
//   +0x10 char                      loaded
//   +0x14 TStringList *             string_list
//   +0x18 void *                    field_18 = operator new(8)
//   +0x1c std::vector<ClassValue>   values
//   +0x3c int                       field_3c = -1
// The offsets 4 and 8 are unnamed padding pending evidence.
class ClassValues
{
  public:
    int field_0;
    int num_classes;
    int rid_1;
    int rid_2;
    char loaded;
    char pad_11[3];
    TStringList *string_list;
    void *field_18;
    std::vector<ClassValue> values;
    int field_3c;

    ClassValues();
    ~ClassValues();

    static void LoadClasses(ClassValues *self);
    int DecodeInt(String value);
    int size();

    static ClassValue GetByIndex(ClassValues *self, int index);
    static void AddClass(ClassValues *self,
                         int id,
                         int parent_type,
                         String name,
                         short stat_group,
                         short str,
                         short intl,
                         short wis,
                         short agi,
                         short con,
                         short cha);
};

#endif
