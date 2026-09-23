#include <vcl.h>
#pragma hdrstop

#include "Questengine.h"
#include "Protocol.h"
#include "Settings.h"

#pragma package(smart_init)

QuestContainer::QuestContainer(Settings *settings)
{
    this->settings = settings;
    max_quests = Settings::GetMaxQuests(this->settings);
    encode_scratch = new char[8];

    RegisterAction(this, 1, "AddNpcText");
    RegisterAction(this, 2, "AddNpcInput");
    RegisterAction(this, 3, "AddNpcChat");
    RegisterAction(this, 4, "SetMap");
    RegisterAction(this, 5, "GiveItem");
    RegisterAction(this, 6, "RemoveItem");
    RegisterAction(this, 7, "End");
    RegisterAction(this, 8, "Reset");
    RegisterAction(this, 9, "SetClass");
    RegisterAction(this, 10, "PlayMusic");
    RegisterAction(this, 11, "PlaySound");
    RegisterAction(this, 12, "ShowHint");
    RegisterAction(this, 13, "GiveExp");
    RegisterAction(this, 14, "RemoveExp");
    RegisterAction(this, 15, "GiveKarma");
    RegisterAction(this, 16, "RemoveKarma");
    RegisterAction(this, 17, "Quake");
    RegisterAction(this, 18, "EffectOnPlayer");
    RegisterAction(this, 19, "EffectOnCoord");
    RegisterAction(this, 20, "ResetDaily");

    RegisterRule(this, 1, "TalkedToNpc");
    RegisterRule(this, 2, "InputNpc");
    RegisterRule(this, 3, "GotItems");
    RegisterRule(this, 4, "LostItems");
    RegisterRule(this, 5, "Die");
    RegisterRule(this, 6, "Disconnect");
    RegisterRule(this, 7, "TimeElapsed");
    RegisterRule(this, 8, "KilledNpcs");
    RegisterRule(this, 9, "KilledPlayers");
    RegisterRule(this, 10, "EnterCoord");
    RegisterRule(this, 11, "EnterMap");
    RegisterRule(this, 12, "LeaveMap");
    RegisterRule(this, 13, "Always");
    RegisterRule(this, 14, "DoneDaily");

    LoadQuests(this);
}

QuestContainer::~QuestContainer()
{
}

void QuestContainer::LoadQuests(QuestContainer *self)
{
    self->quest_list.clear();
    for (int quest_id = 1; quest_id <= 0xfa00 && quest_id <= (int)self->max_quests;
         quest_id++)
        LoadQuest(self, quest_id);
}

bool QuestContainer::LoadQuest(QuestContainer *self, int quest_id)
{
    String text;
    String unused_str;

    int handle;
    int file_size;
    char *file_buf;

    Quest *quest = new Quest(quest_id);
    self->quest_list.insert(self->quest_list.end(), quest);

    try
    {
        text = IntToStr(quest_id);

        for (int i = text.Length(); i <= 4; i++)
            text.Insert("0", 0);

        text.Insert("./quests/", 0);
        text.Insert(".txt", text.Length() + 1);

        handle = FileOpen(text.c_str(), 0);
        if (handle < 0)
            return false;

        file_size = FileSeek(handle, 0, 2);
        FileSeek(handle, 0, 0);
        file_buf = new char[file_size + 1];
        FileRead(handle, file_buf, file_size);
        FileClose(handle);

        text = file_buf;
        text.SetLength(file_size);
        delete[] file_buf;

        self->pending_state_body = 0;
        self->pending_state_name = 0;
        self->in_state_body = 0;
        self->pending_quest_name = 0;
        self->pending_version = 0;
        self->pending_description = 0;
        self->rule_goto_seen = 0;
        self->rule_closed = 0;
        self->state_count = 0;
        self->current_action_type = 0;
        self->current_rule_type = 0;

        int pos = 1;
        bool in_quote = false;
        bool in_comment = false;

        while (text.Length() > 0)
        {
            if (in_quote)
            {
                if (text[pos] == '"')
                {
                    in_quote = false;
                    if (pos > 1)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 1));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == '\n')
                {
                    in_quote = false;
                    if (pos > 2)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 2));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else
                {
                    if (text.Length() == pos)
                        break;
                    pos++;
                }
            }
            else if (in_comment)
            {
                if (text[pos] == '\n')
                {
                    in_comment = false;
                    text.Delete(1, pos);
                    pos = 1;
                }
                else
                {
                    if (text.Length() == pos)
                        break;
                    pos++;
                }
            }
            else
            {
                if (text[pos] == '"')
                {
                    in_quote = true;
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == '/' && text.Length() != pos && text[pos + 1] == '/')
                {
                    in_comment = true;
                    text.Delete(1, pos + 1);
                    pos = 1;
                }
                else if (text[pos] == ' ')
                {
                    if (pos > 1)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 1));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == ',')
                {
                    if (pos > 1)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 1));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == '(' || text[pos] == ')' || text[pos] == '{' ||
                         text[pos] == '}')
                {
                    if (pos > 1)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 1));
                    }
                    ParseToken(self, quest, text[pos]);
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == ';')
                {
                    if (pos > 1)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 1));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == '\t')
                {
                    if (pos > 1)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 1));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else if (text[pos] == '\n')
                {
                    if (pos > 2)
                    {
                        ParseToken(self, quest, text.SubString(1, pos - 2));
                    }
                    text.Delete(1, pos);
                    pos = 1;
                }
                else
                {
                    if (text.Length() == pos)
                    {
                        if (pos > 0)
                            ParseToken(self, quest, text);
                        break;
                    }
                    pos++;
                }
            }
        }
    }
    catch (...)
    {
        FileClose(handle);
        return false;
    }

    vector<QuestState *>::iterator step;
    vector<QuestState *>::iterator lookup;
    vector<QuestRule *>::iterator ref;
    for (step = quest->states.begin(); step != quest->states.end(); ++step)
    {
        for (ref = (*step)->rules.begin(); ref != (*step)->rules.end(); ++ref)
        {
            for (lookup = quest->states.begin(); lookup != quest->states.end(); ++lookup)
            {
                if ((*lookup)->name == (*ref)->name)
                {
                    *(short *)&(*ref)->goto_state_index = (short)(*lookup)->state_index;
                    break;
                }
            }
        }
    }

    quest->state_count = self->state_count;
    quest->loaded = 1;
    return true;
}

void QuestContainer::ParseToken(QuestContainer *self, Quest *quest, String token)
{
    if (self->in_state_body != 0)
    {
        if (self->current_action_type > 0)
        {
            if (token == ")")
            {
                self->current_action_type = 0;
                self->in_action_args = 0;
                return;
            }
            if (self->in_action_args != 0)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (self->current_action->data[i] == "")
                    {
                        self->current_action->data[i] = token;
                        self->current_action->args[i] = ParseInt(self, token);
                        return;
                    }
                }
            }
            if (token == "(")
            {
                self->current_action = new QuestAction(self->current_action_type);
                self->current_state->actions.insert(self->current_state->actions.end(),
                                                    self->current_action);
                self->in_action_args = 1;
                return;
            }
        }

        if (self->rule_closed != 0)
        {
            if (self->rule_goto_seen != 0)
            {
                self->current_rule->name = token;
                self->rule_goto_seen = 1;
                self->rule_closed = 0;
                return;
            }
            if (token == "goto")
            {
                self->rule_goto_seen = 1;
                return;
            }
            self->rule_closed = 0;
        }

        if (self->current_rule_type > 0)
        {
            if (token == ")")
            {
                self->current_rule_type = 0;
                self->in_rule_args = 0;
                self->rule_closed = 1;
                return;
            }
            if (self->in_rule_args != 0)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (self->current_rule->data[i] == "")
                    {
                        self->current_rule->data[i] = token;
                        self->current_rule->args[i] = ParseInt(self, token);
                        return;
                    }
                }
            }
            if (token == "(")
            {
                if (self->current_state->fast_dispatch_rule_type == 0 &&
                    (self->current_rule_type == 3 || self->current_rule_type == 8 ||
                     self->current_rule_type == 9 || self->current_rule_type == 10 ||
                     self->current_rule_type == 11))
                {
                    self->current_state->fast_dispatch_rule_type =
                        self->current_rule_type;
                    self->current_state->fast_dispatch_rule_index =
                        self->current_state->rules.size();
                }
                self->current_rule = new QuestRule(self->current_rule_type);
                self->current_state->rules.insert(self->current_state->rules.end(),
                                                  self->current_rule);
                self->in_rule_args = 1;
                self->rule_goto_seen = 0;
                self->rule_closed = 0;
                return;
            }
        }

        if (LowerCase(token) == "desc")
        {
            self->pending_description = 1;
            return;
        }
        if (self->pending_description != 0)
        {
            self->current_state->description = token;
            self->pending_description = 0;
            return;
        }

        if (GetActionType(self, token) > 0)
        {
            self->current_action_type = GetActionType(self, token);
            self->in_action_args = 0;
            return;
        }

        if (GetRuleType(self, token) > 0)
        {
            self->current_rule_type = GetRuleType(self, token);
            self->in_rule_args = 0;
            return;
        }
    }

    if (LowerCase(token) == "questname")
    {
        self->pending_quest_name = 1;
        return;
    }
    if (LowerCase(token) == "version")
    {
        self->pending_version = 1;
        return;
    }
    if (LowerCase(token) == "state")
    {
        self->pending_state_body = 1;
        self->pending_state_name = 1;
        return;
    }

    if (self->pending_quest_name != 0)
    {
        quest->name = token;
        self->pending_quest_name = 0;
    }
    if (self->pending_version != 0)
    {
        try
        {
            quest->version = StrToInt(token);
        }
        catch (...)
        {
        }
        self->pending_version = 0;
    }
    if (self->pending_state_name != 0)
    {
        self->current_state = new QuestState(self->state_count, token);
        self->state_count++;
        self->pending_state_name = 0;
    }

    if (token == "{")
    {
        if (self->pending_state_body != 0)
        {
            self->pending_state_body = 0;
            self->in_state_body = 1;
            return;
        }
        self->pending_state_body = 0;
        self->in_state_body = 0;
        return;
    }
    if (token == "}")
    {
        if (self->in_state_body != 0)
        {
            quest->states.insert(quest->states.end(), self->current_state);
            self->pending_description = 0;
            self->in_state_body = 0;
            return;
        }
    }
}

char QuestContainer::GetQuestLoaded(QuestContainer *self, int quest_id)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return 0;
    return self->quest_list[quest_id - 1]->loaded;
}

int QuestContainer::GetQuestVersion(QuestContainer *self, int quest_id)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return 0;
    return self->quest_list[quest_id - 1]->version;
}

String QuestContainer::GetQuestName(QuestContainer *self, int quest_id)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return "";
    return self->quest_list[quest_id - 1]->name;
}

// Nothing calls this, so ilink32 drops its COMDAT -- but it instantiates
// vector<QuestState *>::operator[] at this point in the unit, which is where
// the reference emits it (0x53b3dc), ahead of GetActionData rather than after
// GetState. Only that placement is observable.
QuestState *Questengine_StateAt(Quest *quest, int index)
{
    return quest->states[index];
}

// Nothing calls this, so ilink32 drops the COMDAT -- but its empty-string
// literals stay in the unit's _DATA pool. The reference has 2 unreferenced
// NUL bytes exactly here in the pool (`0x5818c3`-`0x5818c4`, between GetQuestName's
// and GetActionData's); only their count and position
// are observable (as with NpcValues::ClearDrops).
void Questengine_EmptyLiterals()
{
    "";
    "";
}

String QuestContainer::GetActionData(QuestContainer *self,
                                     int quest_id,
                                     int state_index,
                                     int arg)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return "";

    String data = "";
    for (vector<QuestAction *>::iterator it = state->actions.begin();
         it != state->actions.end();
         ++it)
    {
        if ((*it)->args[0] == arg && (*it)->action == QuestAction_AddNpcChat)
        {
            data.Insert((*it)->data[1], data.Length() + 1);
            data.Insert((char)-1, data.Length() + 1);
        }
    }
    return data;
}

String QuestContainer::GetActionData2(QuestContainer *self,
                                      int quest_id,
                                      int state_index,
                                      int arg)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return "";
    String data = "";
    for (vector<QuestAction *>::iterator it = state->actions.begin();
         it != state->actions.end();
         ++it)
    {
        if ((*it)->args[0] == arg)
        {
            if ((*it)->action == QuestAction_AddNpcText)
            {
                data.Insert(EncodeNumber(self, (*it)->action, 2), data.Length() + 1);
                data.Insert((*it)->data[1], data.Length() + 1);
                data.Insert((char)-1, data.Length() + 1);
            }
            if ((*it)->action == QuestAction_AddNpcInput)
            {
                data.Insert(EncodeNumber(self, (*it)->action, 2), data.Length() + 1);
                data.Insert(EncodeNumber(self, (*it)->args[1], 2), data.Length() + 1);
                data.Insert((*it)->data[2], data.Length() + 1);
                data.Insert((char)-1, data.Length() + 1);
            }
        }
    }
    return data;
}

// Nothing calls this, so ilink32 drops the COMDAT -- but its empty-string
// literals stay in the unit's _DATA pool. The reference has 2 unreferenced
// NUL bytes exactly here in the pool (`0x5818c9`-`0x5818ca`, between
// GetActionData2's and GetRuleValue's); only their count and position
// are observable (as with NpcValues::ClearDrops).
void Questengine_EmptyLiterals2()
{
    "";
    "";
}

int QuestContainer::GetRuleValue(QuestContainer *self,
                                 int quest_id,
                                 int state_index,
                                 int rule_type)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return -1;

    String unused = "";
    for (vector<QuestRule *>::iterator it = state->rules.begin();
         it != state->rules.end();
         ++it)
    {
        if ((*it)->rule == QuestRule_TalkedToNpc && (*it)->args[0] == rule_type)
            return *(short *)&(*it)->goto_state_index;
    }
    return -1;
}

int QuestContainer::GetRuleValue2(QuestContainer *self,
                                  int quest_id,
                                  int state_index,
                                  int rule_type)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return -1;

    String unused = "";
    for (vector<QuestRule *>::iterator it = state->rules.begin();
         it != state->rules.end();
         ++it)
    {
        if ((*it)->rule == QuestRule_InputNpc && (*it)->args[0] == rule_type)
            return *(short *)&(*it)->goto_state_index;
    }
    return -1;
}

QuestState *QuestContainer::GetState(QuestContainer *self, int quest_id, int state_index)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return NULL;
    if (state_index < 0 ||
        self->quest_list[quest_id - 1]->states.size() <= (unsigned)state_index)
        return NULL;
    return self->quest_list[quest_id - 1]->states[state_index];
}

void QuestContainer::RegisterAction(QuestContainer *self, int action_id, String name)
{
    QuestType entry(action_id, name);
    self->action_names.insert(self->action_names.end(), entry);
}

void QuestContainer::RegisterRule(QuestContainer *self, int rule_id, String name)
{
    QuestType entry(rule_id, name);
    self->rule_names.insert(self->rule_names.end(), entry);
}

int QuestContainer::GetActionType(QuestContainer *self, String name)
{
    if (name.Length() < 3)
        return 0;

    for (vector<QuestType>::iterator it = self->action_names.begin();
         it != self->action_names.end();
         ++it)
    {
        if (it->name == name)
            return it->value;
    }
    return 0;
}

int QuestContainer::GetRuleType(QuestContainer *self, String name)
{
    if (name.Length() < 3)
        return 0;

    for (vector<QuestType>::iterator it = self->rule_names.begin();
         it != self->rule_names.end();
         ++it)
    {
        if (it->name == name)
            return it->value;
    }
    return 0;
}

int QuestContainer::ParseInt(QuestContainer *self, String token)
{
    for (int i = 1; i <= token.Length(); i++)
    {
        if ((unsigned char)token[i] < '0' || (unsigned char)token[i] > '9')
            return 0;
    }
    return StrToInt(token);
}

String QuestContainer::EncodeNumber(QuestContainer *self, unsigned int value, int width)
{
    int rem;
    char c;
    try
    {
        unsigned int quotient = 1;
        bool leading = true;
        for (int i = 0; i < width; i++)
        {
            if (leading)
            {
                double d = value / 253.0;
                quotient = d;
                rem = value % EO_CHAR_MAX;
                c = rem + 1;
                ((char *)self->encode_scratch)[i] = c;
                value = quotient;
                if (quotient < 1)
                    leading = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = EO_PADDING_BYTE;
                ((char *)self->encode_scratch)[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded_str((char *)self->encode_scratch, width);
    return encoded_str;
}
