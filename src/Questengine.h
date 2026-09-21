#ifndef QuestengineH
#define QuestengineH

#include <Classes.hpp>
#include <vector.h>
#include "Quest.h"
#include "Queststate.h"
#include "Questtype.h"
#include "Settings.h"


// Layout recovered from the reference constructor (0x537ae0), the parser
// (0x5397c8) and the quest accessors; sizeof(Questengine) is 0x94.
//   +0x00 unsigned short              max_quests
//   +0x04 vector<Quest *>        quest_list
//   +0x24 Settings *                  settings
//   +0x28 void *                      encode_scratch
//   +0x2c QuestState *                field_0x2c
//   +0x30 QuestAction *               field_0x30
//   +0x34 QuestRule *                 field_0x34
//   +0x38 char                        field_0x38
//   +0x39 char                        field_0x39
//   +0x3a char                        field_0x3a
//   +0x3c int                         field_0x3c
//   +0x40 char                        field_0x40
//   +0x41 char                        field_0x41
//   +0x42 char                        field_0x42
//   +0x44 int                         field_0x44
//   +0x48 char                        field_0x48
//   +0x4c int                         field_0x4c
//   +0x50 char                        field_0x50
//   +0x51 char                        field_0x51
//   +0x52 char                        field_0x52
//   +0x54 vector<QuestType>      action_names
//   +0x74 vector<QuestType>      cond_names
class Questengine
{
  public:
    unsigned short max_quests;
    vector<Quest *> quest_list;
    Settings *settings;
    void *encode_scratch;
    QuestState *field_0x2c;
    QuestAction *field_0x30;
    QuestRule *field_0x34;
    char field_0x38;
    char field_0x39;
    char field_0x3a;
    char pad_0x3b;
    int field_0x3c;
    char field_0x40;
    char field_0x41;
    char field_0x42;
    char pad_0x43;
    int field_0x44;
    char field_0x48;
    char pad_0x49[3];
    int field_0x4c;
    char field_0x50;
    char field_0x51;
    char field_0x52;
    char pad_0x53;
    vector<QuestType> action_names;
    vector<QuestType> cond_names;

    Questengine(Settings *settings);
    ~Questengine();

    static void LoadQuests(Questengine *self);
    static bool LoadQuest(Questengine *self, int quest_id);
    static void ParseToken(Questengine *self, Quest *quest, String token);
    static QuestState *GetState(Questengine *self, int quest_id, int state_index);
    static void RegisterAction(Questengine *self, int action_id, String name);
    static void RegisterCondition(Questengine *self, int condition_id, String name);
    static String EncodeNumber(Questengine *self, unsigned int value, int width);
    static int GetActionType(Questengine *self, String name);
    static int GetConditionType(Questengine *self, String name);
    static int ParseInt(Questengine *self, String token);
    static int GetQuestVersion(Questengine *self, int quest_id);
    static char GetQuestLoaded(Questengine *self, int quest_id);
    static String GetQuestName(Questengine *self, int quest_id);
    static String
    GetActionData(Questengine *self, int quest_id, int state_index, int arg);
    static String
    GetActionData2(Questengine *self, int quest_id, int state_index, int arg);
    static int
    GetRuleValue(Questengine *self, int quest_id, int state_index, int rule_type);
    static int
    GetRuleValue2(Questengine *self, int quest_id, int state_index, int rule_type);
};

#endif
