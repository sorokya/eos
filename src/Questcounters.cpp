#include <vcl.h>
#include <dirent.h>
#include <stdio.h>
#pragma hdrstop

#include "Questcounters.h"

#pragma package(smart_init)

// Largest bucket index (the dead name[1]-'a' clamp mirrors Killcounters).
#define COUNTER_BUCKET_MAX 26

QuestCounters::QuestCounters()
{
    Load(this);
}

QuestCounters::~QuestCounters()
{
}

void QuestCounters::Clear(QuestCounters *self)
{
    vector<QuestCounter>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        QuestCounter::Clear(it);
    }
    self->counters.clear();
    String dir = "./cache/";
    DIR *d = opendir(dir.c_str());
    struct dirent *entry;
    while ((entry = readdir(d)) != 0)
    {
        String file = entry->d_name;
        // Verified: the four-clause test and the two-clause test are separate
        // (nested) ifs; merging them into one && chain drops an EH scope marker.
        if (file != "." && file != ".." && file.Pos(".") > 0 && file.Length() > 6)
        {
            if (file.SubString(1, 5) == "quest" &&
                file.SubString(file.Pos(".") + 1, 4) == "chk")
            {
                String path = dir + file;
                remove(path.c_str());
            }
        }
    }
}

void QuestCounters::Load(QuestCounters *self)
{
    String dir = "./cache/";
    DIR *d = opendir(dir.c_str());
    struct dirent *entry;
    while ((entry = readdir(d)) != 0)
    {
        String file = entry->d_name;
        if (file != "." && file != ".." && file.Pos(".") > 0 && file.Length() > 6)
        {
            if (file.SubString(1, 5) == "quest" &&
                file.SubString(file.Pos(".") + 1, 4) == "chk")
            {
                QuestCounter counter(StrToInt(file.SubString(6, file.Pos(".") - 6)));
                self->counters.insert(self->counters.end(), counter);
            }
        }
    }
    vector<QuestCounter>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        // Verified fidelity fact: the reference loads (*it).quest_id into a
        // local and discards it before QuestCounter::Load; keep the dead read.
        int quest_id = (*it).quest_id;
        QuestCounter::Load(it);
    }
}

void QuestCounters::Save(QuestCounters *self)
{
    vector<QuestCounter>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        QuestCounter::Save(it);
    }
}

int QuestCounters::RecordCompletion(QuestCounters *self, String name, int quest_id)
{
    name = name.LowerCase();
    // Verified fidelity fact: the reference computes and clamps this bucket
    // index (name[1] - 'a' to [0,26]) and discards it; the lookup is the
    // linear quest_id scan below.
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > COUNTER_BUCKET_MAX)
    {
        bucket = COUNTER_BUCKET_MAX;
    }
    vector<QuestCounter>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        if (quest_id == (*it).quest_id)
        {
            return QuestCounter::Increment(it, name);
        }
    }
    QuestCounter counter(quest_id);
    QuestCounter::Increment(&counter, name);
    self->counters.insert(self->counters.end(), counter);
    return 1;
}

int QuestCounters::GetCompletionCount(QuestCounters *self,
                                      String player_name,
                                      int quest_id)
{
    player_name = player_name.LowerCase();
    // Same verified dead bucket computation as RecordCompletion.
    int bucket = player_name[1] - 'a';
    if (bucket < 0 || bucket > COUNTER_BUCKET_MAX)
    {
        bucket = COUNTER_BUCKET_MAX;
    }
    vector<QuestCounter>::iterator it;
    for (it = self->counters.begin(); it != self->counters.end(); it++)
    {
        if (quest_id == (*it).quest_id)
        {
            return QuestCounter::Get(it, player_name);
        }
    }
    return 0;
}
