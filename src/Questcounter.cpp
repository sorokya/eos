#include <vcl.h>
#include <io.h>
#include <stdio.h>
#pragma hdrstop

#include "Questcounter.h"

#pragma package(smart_init)

QuestCounter::QuestCounter(int quest_id)
{
    this->quest_id = quest_id;
    string_list = new TStringList;
}

QuestCounter::~QuestCounter()
{
}

void QuestCounter::Clear(QuestCounter *self)
{
    self->counters.clear();
}

void QuestCounter::Load(QuestCounter *self)
{
    self->string_list->Clear();
    try
    {
        self->string_list->LoadFromFile("./cache/quest" + IntToStr(self->quest_id) +
                                        ".chk");
    }
    catch (...)
    {
    }
    for (int i = 0; i < self->string_list->Count; i++)
    {
        self->label += self->string_list->Strings[i];
        String name = Extract(self);
        Set(self, name, StrToInt(Extract(self)));
    }
}

void QuestCounter::Save(QuestCounter *self)
{
    String path = "./cache/quest" + IntToStr(self->quest_id) + ".chk";
    if (access(path.c_str(), 0) == 0)
    {
        remove(path.c_str());
    }
    self->string_list->Clear();
    vector<QuestCounterList>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        String line = (*it).name + ";";
        line = line + IntToStr((*it).count) + ";";
        self->string_list->Add(line);
    }
    self->string_list->SaveToFile(path);
}

String QuestCounter::Extract(QuestCounter *self)
{
    int pos = self->label.Pos(";");
    if (pos < 1)
        return self->label;
    String result = self->label.SubString(1, pos - 1);
    self->label.Delete(1, pos);
    return result;
}

int QuestCounter::Increment(QuestCounter *self, String name)
{
    name += name.LowerCase();
    // Verified: the iterator is declared before the loop. Moving it into the
    // for-init arms an extra EH scope marker and changes the bytes.
    vector<QuestCounterList>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        if (name == (*it).name)
        {
            (*it).count++;
            return (*it).count;
        }
    }
    QuestCounterList counter(name);
    self->counters.insert(self->counters.end(), counter);
    return 1;
}

int QuestCounter::Get(QuestCounter *self, String name)
{
    name += name.LowerCase();
    vector<QuestCounterList>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        if (name == (*it).name)
        {
            return (*it).count;
        }
    }
    return 0;
}

void QuestCounter::Set(QuestCounter *self, String name, int count)
{
    name += name.LowerCase();
    vector<QuestCounterList>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        if (name == (*it).name)
        {
            (*it).count = count;
            return;
        }
    }
    QuestCounterList counter(name, count);
    self->counters.insert(self->counters.end(), counter);
}
