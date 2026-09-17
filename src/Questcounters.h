#ifndef QuestcountersH
#define QuestcountersH

#include <Classes.hpp>
#include <vector>
#include "Questcounter.h"

// Recovered from the reference (Questcounters unit, 0x54084c..0x541e84):
// a plain collection of QuestCounter records, one per quest id, held by value
// (0x2c element stride). RTTI type name `QuestCounters`; the member is
// `std::vector<QuestCounter>`.
class QuestCounters
{
  public:
    std::vector<QuestCounter> counters;

    QuestCounters();
    ~QuestCounters();

    static void Clear(QuestCounters *self);
    static void Load(QuestCounters *self);
    static void Save(QuestCounters *self);
    static int RecordCompletion(QuestCounters *self, String name, int quest_id);
    static int GetCompletionCount(QuestCounters *self, String player_name, int quest_id);
};

#endif
