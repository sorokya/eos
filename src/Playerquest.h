#ifndef PlayerquestH
#define PlayerquestH

#include <Classes.hpp>

// Layout recovered from the reference (Playerquest unit, 0x537530..0x5375b4):
// the constructor stores the quest id at offset 0, state_index at 4, the
// tracked quest version at 6, clears the five-element `counters` array at 8 and
// the `done` flag at 0x12; the element stride of the owning std::vector is
// 0x14. The class name is the RTTI type name ("PlayerQuest").
//
// state_index/counters/done are the fields the quest engine reads:
// Player_EvaluateQuestRules (0x4597c0) writes counters[rule_index] and
// state_index, and Player_FireQuestTriggers (0x459708) erases an entry once
// done is set. `version` is the value of Questengine::GetQuestVersion for the
// quest at tracker creation (0x44825e passes FUN_0053b2ec, whose body matches
// GetQuestVersion returning Quest::version at +8); Character_BuildSaveQuery
// (0x4097df) persists it alongside state_index and counters.
struct PlayerQuest
{
    int quest_id;
    short state_index;
    short version;
    short counters[5];
    char done;

    PlayerQuest(int quest_id, short state_index, short version);
    ~PlayerQuest();
};

#endif
