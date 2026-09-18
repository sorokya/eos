#include <vcl.h>
#pragma hdrstop

#include "Questengine.h"
#include "Settings.h"

#pragma package(smart_init)

Questengine::Questengine(Settings *settings)
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

    RegisterCondition(this, 1, "TalkedToNpc");
    RegisterCondition(this, 2, "InputNpc");
    RegisterCondition(this, 3, "GotItems");
    RegisterCondition(this, 4, "LostItems");
    RegisterCondition(this, 5, "Die");
    RegisterCondition(this, 6, "Disconnect");
    RegisterCondition(this, 7, "TimeElapsed");
    RegisterCondition(this, 8, "KilledNpcs");
    RegisterCondition(this, 9, "KilledPlayers");
    RegisterCondition(this, 10, "EnterCoord");
    RegisterCondition(this, 11, "EnterMap");
    RegisterCondition(this, 12, "LeaveMap");
    RegisterCondition(this, 13, "Always");
    RegisterCondition(this, 14, "DoneDaily");

    LoadQuests(this);
}

Questengine::~Questengine()
{
}

void Questengine::RegisterAction(Questengine *self, int action_id, String name)
{
    QuestType entry(action_id, name);
    self->action_names.insert(self->action_names.end(), entry);
}

void Questengine::RegisterCondition(Questengine *self, int condition_id, String name)
{
    QuestType entry(condition_id, name);
    self->cond_names.insert(self->cond_names.end(), entry);
}

void Questengine::LoadQuests(Questengine *self)
{
    self->quest_list.clear();
    for (int quest_id = 1; quest_id <= 0xfa00 && quest_id <= (int)self->max_quests;
         quest_id++)
        LoadQuest(self, quest_id);
}

bool Questengine::LoadQuest(Questengine *self, int quest_id)
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

        self->field_38 = 0;
        self->field_39 = 0;
        self->field_3a = 0;
        self->field_40 = 0;
        self->field_41 = 0;
        self->field_42 = 0;
        self->field_51 = 0;
        self->field_52 = 0;
        self->field_3c = 0;
        self->field_44 = 0;
        self->field_4c = 0;

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

    std::vector<QuestState *>::iterator step;
    std::vector<QuestState *>::iterator lookup;
    std::vector<QuestRule *>::iterator ref;
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

    quest->state_count = self->field_3c;
    quest->loaded = 1;
    return true;
}

void Questengine::ParseToken(Questengine *self, Quest *quest, String token)
{
    if (self->field_3a != 0)
    {
        if (self->field_44 > 0)
        {
            if (token == ")")
            {
                self->field_44 = 0;
                self->field_48 = 0;
                return;
            }
            if (self->field_48 != 0)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (self->field_30->data[i] == "")
                    {
                        self->field_30->data[i] = token;
                        self->field_30->args[i] = ParseInt(self, token);
                        return;
                    }
                }
            }
            if (token == "(")
            {
                self->field_30 = new QuestAction(self->field_44);
                self->field_2c->actions.insert(self->field_2c->actions.end(),
                                               self->field_30);
                self->field_48 = 1;
                return;
            }
        }

        if (self->field_52 != 0)
        {
            if (self->field_51 != 0)
            {
                self->field_34->name = token;
                self->field_51 = 1;
                self->field_52 = 0;
                return;
            }
            if (token == "goto")
            {
                self->field_51 = 1;
                return;
            }
            self->field_52 = 0;
        }

        if (self->field_4c > 0)
        {
            if (token == "}")
            {
                self->field_4c = 0;
                self->field_50 = 0;
                self->field_52 = 1;
                return;
            }
            if (self->field_50 != 0)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (self->field_34->data[i] == "")
                    {
                        self->field_34->data[i] = token;
                        self->field_34->args[i] = ParseInt(self, token);
                        return;
                    }
                }
            }
            if (token == "(")
            {
                if (self->field_2c->fast_dispatch_condition_type == 0 &&
                    (self->field_4c == 3 || self->field_4c == 8 || self->field_4c == 9 ||
                     self->field_4c == 10 || self->field_4c == 11))
                {
                    self->field_2c->fast_dispatch_condition_type = self->field_4c;
                    self->field_2c->fast_dispatch_rule_index =
                        self->field_2c->rules.size();
                }
                self->field_34 = new QuestRule(self->field_4c);
                self->field_2c->rules.insert(self->field_2c->rules.end(), self->field_34);
                self->field_50 = 1;
                self->field_51 = 0;
                self->field_52 = 0;
                return;
            }
        }

        if (AnsiLowerCase(token) == "desc")
        {
            self->field_42 = 1;
            return;
        }
        if (self->field_42 != 0)
        {
            self->field_2c->description = token;
            self->field_42 = 0;
            return;
        }

        if (GetActionType(self, token) > 0)
        {
            self->field_44 = GetActionType(self, token);
            self->field_48 = 0;
            return;
        }

        if (GetConditionType(self, token) > 0)
        {
            self->field_4c = GetConditionType(self, token);
            self->field_50 = 0;
            return;
        }
    }

    if (AnsiLowerCase(token) == "questname")
    {
        self->field_40 = 1;
        return;
    }
    if (AnsiLowerCase(token) == "version")
    {
        self->field_41 = 1;
        return;
    }
    if (AnsiLowerCase(token) == "state")
    {
        self->field_38 = 1;
        self->field_39 = 1;
        return;
    }

    if (self->field_40 != 0)
    {
        quest->name = token;
        self->field_40 = 0;
    }
    if (self->field_41 != 0)
    {
        try
        {
            quest->version = StrToInt(token);
        }
        catch (...)
        {
        }
        self->field_41 = 0;
    }
    if (self->field_39 != 0)
    {
        self->field_2c = new QuestState(self->field_3c, token);
        self->field_3c++;
        self->field_39 = 0;
    }

    if (token == "{")
    {
        if (self->field_38 != 0)
        {
            self->field_38 = 0;
            self->field_3a = 1;
            return;
        }
        self->field_38 = 0;
        self->field_3a = 0;
        return;
    }
    if (token == "}")
    {
        if (self->field_3a != 0)
        {
            quest->states.insert(quest->states.end(), self->field_2c);
            self->field_42 = 0;
            self->field_3a = 0;
        }
        return;
    }
}

QuestState *Questengine::GetState(Questengine *self, int quest_id, int state_index)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return NULL;
    if (state_index < 0 ||
        self->quest_list[quest_id - 1]->states.size() <= (unsigned)state_index)
        return NULL;
    return self->quest_list[quest_id - 1]->states[state_index];
}

int Questengine::GetActionType(Questengine *self, String name)
{
    if (name.Length() < 3)
        return 0;

    for (std::vector<QuestType>::iterator it = self->action_names.begin();
         it != self->action_names.end();
         ++it)
    {
        if (it->name == name)
            return it->value;
    }
    return 0;
}

int Questengine::GetConditionType(Questengine *self, String name)
{
    if (name.Length() < 3)
        return 0;

    for (std::vector<QuestType>::iterator it = self->cond_names.begin();
         it != self->cond_names.end();
         ++it)
    {
        if (it->name == name)
            return it->value;
    }
    return 0;
}

int Questengine::ParseInt(Questengine *self, String token)
{
    for (int i = 1; i <= token.Length(); i++)
    {
        if ((unsigned char)token[i] < '0' || (unsigned char)token[i] > '9')
            return 0;
    }
    return StrToInt(token);
}

String Questengine::AppendEncoded(Questengine *self, unsigned int value, int width)
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
                rem = value % 0xfd;
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
                char pad = 0xfe;
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

int Questengine::GetQuestVersion(Questengine *self, int quest_id)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return 0;
    return self->quest_list[quest_id - 1]->version;
}

char Questengine::GetQuestLoaded(Questengine *self, int quest_id)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return 0;
    return self->quest_list[quest_id - 1]->loaded;
}

String Questengine::GetQuestName(Questengine *self, int quest_id)
{
    if (quest_id < 1 || self->quest_list.size() < (unsigned)quest_id)
        return "";
    return self->quest_list[quest_id - 1]->name;
}

String
Questengine::GetActionData(Questengine *self, int quest_id, int state_index, int arg)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return "";

    String data = "";
    for (std::vector<QuestAction *>::iterator it = state->actions.begin();
         it != state->actions.end();
         ++it)
    {
        if ((*it)->args[0] == arg && (*it)->action == 3)
        {
            data.Insert((*it)->data[1], data.Length() + 1);
            data.Insert((char)-1, data.Length() + 1);
        }
    }
    return data;
}

String
Questengine::GetActionData2(Questengine *self, int quest_id, int state_index, int arg)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return "";
    String data = "";
    for (std::vector<QuestAction *>::iterator it = state->actions.begin();
         it != state->actions.end();
         ++it)
    {
        if ((*it)->args[0] == arg)
        {
            if ((*it)->action == 1)
            {
                data.Insert(AppendEncoded(self, (*it)->action, 2), data.Length() + 1);
                data.Insert((*it)->data[1], data.Length() + 1);
                data.Insert((char)-1, data.Length() + 1);
            }
            if ((*it)->action == 2)
            {
                data.Insert(AppendEncoded(self, (*it)->action, 2), data.Length() + 1);
                data.Insert(AppendEncoded(self, (*it)->args[1], 2), data.Length() + 1);
                data.Insert((*it)->data[2], data.Length() + 1);
                data.Insert((char)-1, data.Length() + 1);
            }
        }
    }
    return data;
}

int Questengine::GetRuleValue(Questengine *self,
                              int quest_id,
                              int state_index,
                              int rule_type)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return -1;

    String unused = "";
    for (std::vector<QuestRule *>::iterator it = state->rules.begin();
         it != state->rules.end();
         ++it)
    {
        if ((*it)->rule == 1 && (*it)->args[0] == rule_type)
            return *(short *)&(*it)->goto_state_index;
    }
    return -1;
}

int Questengine::GetRuleValue2(Questengine *self,
                               int quest_id,
                               int state_index,
                               int rule_type)
{
    QuestState *state = GetState(self, quest_id, state_index);
    if (state == NULL)
        return -1;

    String unused = "";
    for (std::vector<QuestRule *>::iterator it = state->rules.begin();
         it != state->rules.end();
         ++it)
    {
        if ((*it)->rule == 2 && (*it)->args[0] == rule_type)
            return *(short *)&(*it)->goto_state_index;
    }
    return -1;
}
