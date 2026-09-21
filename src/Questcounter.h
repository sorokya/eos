#ifndef QuestcounterH
#define QuestcounterH

#include <Classes.hpp>
#include <vector.h>
#include "Questcounterlist.h"

// Recovered from the reference (Questcounter unit, 0x53f604..0x5406d4):
//   +0x00 int quest_id
//   +0x04 TStringList *string_list        (new TStringList in the ctor)
//   +0x08 String label
//   +0x0c vector<QuestCounterList> counters   (8-byte element stride)
// sizeof 0x2c. RTTI type name `QuestCounter`; the container QuestCounters
// holds these by value (0x2c stride).
class QuestCounter
{
  public:
    int quest_id;
    TStringList *string_list;
    String label;
    vector<QuestCounterList> counters;

    QuestCounter(int quest_id);
    ~QuestCounter();

    static void Clear(QuestCounter *self);
    static void Load(QuestCounter *self);
    static void Save(QuestCounter *self);
    static String Extract(QuestCounter *self);
    static int Increment(QuestCounter *self, String name);
    static int Get(QuestCounter *self, String name);
    static void Set(QuestCounter *self, String name, int count);
};

#endif
