#ifndef QueststateH
#define QueststateH

#include <Classes.hpp>
#include <vector.h>

// Layouts recovered from the reference constructors and the Questengine parser
// (0x5397c8); class names are the RTTI type names.
//
// QuestAction (0x24): int action (+0), int args[4] (+4), AnsiString data[4] (+0x14)
// QuestRule   (0x2c): int rule (+0), int args[4] (+4), AnsiString data[4] (+0x14),
//                     AnsiString name (+0x24), int goto_state_index (+0x28) -- the
//                     low short is the goto state index (Ghidra Questrule::
//                     goto_state_index; Questengine sets it from the target
//                     state and reads it back through
//                     `*(short *)&goto_state_index`)
// QuestState  (0x54): int state_index (+0), String name (+4), String description
//                     (+8), int fast_dispatch_rule_index (+0xc),
//                     int fast_dispatch_condition_type (+0x10),
//                     vector<QuestAction *> actions (+0x14),
//                     vector<QuestRule *> rules (+0x34)
class QuestAction
{
  public:
    int action;
    int args[4];
    AnsiString data[4];

    QuestAction(int action);
    ~QuestAction();
};

class QuestRule
{
  public:
    int rule;
    int args[4];
    AnsiString data[4];
    AnsiString name;
    int goto_state_index;

    QuestRule(int rule);
    ~QuestRule();
};

class QuestState
{
  public:
    int state_index;
    String name;
    String description;
    int fast_dispatch_rule_index;
    int fast_dispatch_condition_type;
    vector<QuestAction *> actions;
    vector<QuestRule *> rules;

    QuestState(int state_index, String name);
    ~QuestState();
};

#endif
