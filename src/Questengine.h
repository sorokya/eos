#ifndef QuestengineH
#define QuestengineH

#include <Classes.hpp>
#include <vector.h>
#include "Quest.h"
#include "Queststate.h"
#include "Questtype.h"
#include "Settings.h"

// Layout recovered from the reference constructor (0x537ae0), the parser
// (0x5397c8) and the quest accessors; sizeof(QuestContainer) is 0x94.
//   +0x00 unsigned short              max_quests
//   +0x04 vector<Quest *>        quest_list
//   +0x24 Settings *                  settings
//   +0x28 void *                      encode_scratch
//   +0x2c QuestState *                current_state
//   +0x30 QuestAction *               current_action
//   +0x34 QuestRule *                 current_rule
//   +0x38 char                        pending_state_body
//   +0x39 char                        pending_state_name
//   +0x3a char                        in_state_body
//   +0x3c int                         state_count
//   +0x40 char                        pending_quest_name
//   +0x41 char                        pending_version
//   +0x42 char                        pending_description
//   +0x44 int                         current_action_type
//   +0x48 char                        in_action_args
//   +0x4c int                         current_rule_type
//   +0x50 char                        in_rule_args
//   +0x51 char                        rule_goto_seen
//   +0x52 char                        rule_closed
//   +0x54 vector<QuestType>      action_names
//   +0x74 vector<QuestType>      cond_names
class QuestContainer
{
  public:
    unsigned short max_quests;
    vector<Quest *> quest_list;
    Settings *settings;
    void *encode_scratch;
    QuestState *current_state;
    QuestAction *current_action;
    QuestRule *current_rule;
    char pending_state_body;
    char pending_state_name;
    char in_state_body;
    char pad_0x3b;
    int state_count;
    char pending_quest_name;
    char pending_version;
    char pending_description;
    char pad_0x43;
    int current_action_type;
    char in_action_args;
    char pad_0x49[3];
    int current_rule_type;
    char in_rule_args;
    char rule_goto_seen;
    char rule_closed;
    char pad_0x53;
    vector<QuestType> action_names;
    vector<QuestType> cond_names;

    QuestContainer(Settings *settings);
    ~QuestContainer();

    static void LoadQuests(QuestContainer *self);
    static bool LoadQuest(QuestContainer *self, int quest_id);
    static void ParseToken(QuestContainer *self, Quest *quest, String token);
    static QuestState *GetState(QuestContainer *self, int quest_id, int state_index);
    static void RegisterAction(QuestContainer *self, int action_id, String name);
    static void RegisterCondition(QuestContainer *self, int condition_id, String name);
    static String EncodeNumber(QuestContainer *self, unsigned int value, int width);
    static int GetActionType(QuestContainer *self, String name);
    static int GetConditionType(QuestContainer *self, String name);
    static int ParseInt(QuestContainer *self, String token);
    static int GetQuestVersion(QuestContainer *self, int quest_id);
    static char GetQuestLoaded(QuestContainer *self, int quest_id);
    static String GetQuestName(QuestContainer *self, int quest_id);
    static String
    GetActionData(QuestContainer *self, int quest_id, int state_index, int arg);
    static String
    GetActionData2(QuestContainer *self, int quest_id, int state_index, int arg);
    static int
    GetRuleValue(QuestContainer *self, int quest_id, int state_index, int rule_type);
    static int
    GetRuleValue2(QuestContainer *self, int quest_id, int state_index, int rule_type);
};

#endif
