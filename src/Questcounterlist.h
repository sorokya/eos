#ifndef QuestcounterlistH
#define QuestcounterlistH

#include <Classes.hpp>

// Recovered from the reference (Questcounterlist unit, 0x5406f4..0x54082c):
// String name (+0), int count (+4), sizeof 8, confirmed by the 8-byte element
// stride of QuestCounter::counters. RTTI type name `QuestCounterList`; element
// type of `std::vector<QuestCounterList>` (see Questcounter.h).
class QuestCounterList
{
  public:
    String name;
    int count;

    QuestCounterList(String name);
    QuestCounterList(String name, int count);
    ~QuestCounterList();
};

#endif
