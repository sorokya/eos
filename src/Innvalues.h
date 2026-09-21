#ifndef InnvaluesH
#define InnvaluesH

#include <vector.h>
#include "Innvalue.h"

// The inn table (EID). Layout recovered from the reference constructor
// (0x5342dc) and the parser/accessors: sizeof(vector<InnValue>) is 32 and
// the element extent runs to 0x24 / the element type is InnValue:
//   +0x00 char                       loaded
//   +0x04 vector<InnValue>      record_list
//   +0x24 int                        field_0x24 = -1
class InnValues
{
  public:
    char loaded;
    char pad_0x1[3];
    vector<InnValue> record_list;
    int field_0x24;

    InnValues();
    ~InnValues();

    static void LoadInns(InnValues *self);
    int DecodeNumber(String value);
    unsigned int GetCount();

    static void Clear(InnValues *self);

    static String GetName(InnValues *self, int index);
    static int GetSleepMap(InnValues *self, int index);
    static int GetSleepX(InnValues *self, int index);
    static int GetSleepY(InnValues *self, int index);
    static int GetSpawnMap(InnValues *self, int index, int threshold);
    static int GetSpawnX(InnValues *self, int index, int threshold);
    static int GetSpawnY(InnValues *self, int index, int threshold);
    static String GetQuestion(InnValues *self, int index);
    static String GetAnswer(InnValues *self, int index, int number);
};

#endif
