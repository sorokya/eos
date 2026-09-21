#ifndef ClassvaluesH
#define ClassvaluesH

#include <vector.h>
#include "Classvalue.h"

// The class table (ECF). Layout recovered from the reference constructor
// (0x535eec); sizeof(vector<ClassValue>) is 32 and sizeof(ClassValue) is
// 28, matching the reference's element size and member extent:
//   +0x00 int                       file_id
//   +0x10 char                      loaded
//   +0x14 TStringList *             string_list
//   +0x18 void *                    field_0x18 = operator new(8)
//   +0x1c vector<ClassValue>   record_list
//   +0x3c int                       field_0x3c = -1
// The offsets 4 and 8 are unnamed padding pending evidence.
class ClassValues
{
  public:
    int file_id;
    int num_records;
    int rid_1;
    int rid_2;
    char loaded;
    char pad_0x11[3];
    TStringList *string_list;
    void *field_0x18;
    vector<ClassValue> record_list;
    int field_0x3c;

    ClassValues();
    ~ClassValues();

    static void LoadClasses(ClassValues *self);
    int DecodeNumber(String value);
    int GetCount();

    static ClassValue GetByIndex(ClassValues *self, int index);
    static bool ClassMatches(ClassValues *self, int class_id, int class_requirement);
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
