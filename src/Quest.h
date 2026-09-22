#ifndef QuestH
#define QuestH

#include <Classes.hpp>
#include <vector.h>
#include "Queststate.h"

// Layout recovered from the reference constructor (0x5376c0), the deleting
// destructor (0x5378a4) and the QuestContainer parser (0x5397c8); the class name is
// the RTTI type name.
//
// Quest (0x34): int quest_id (+0), String name (+4), int version (+8),
//               int state_count (+0xc), char loaded (+0x10),
//               vector<QuestState *> states (+0x14)
class Quest
{
  public:
    int quest_id;
    String name;
    int version;
    int state_count;
    char loaded;
    vector<QuestState *> states;

    Quest(int quest_id);
    ~Quest();
};

#endif
