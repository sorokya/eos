#include <vcl.h>
#include <io.h>
#include <stdio.h>
#pragma hdrstop

#include "Killcounters.h"

#pragma package(smart_init)

KillCounters::KillCounters()
{
    string_list = new TStringList;
    Init(this);
}

KillCounters::~KillCounters()
{
}

void KillCounters::Clear(KillCounters *self)
{
    for (int i = 0; i < 26; i++)
    {
        self->buckets[i].clear();
    }
    String path = "./cache/kills.chk";
    if (access(path.c_str(), 0) == 0)
    {
        remove(path.c_str());
    }
}

int KillCounters::IncrementAndGet(KillCounters *self, String name)
{
    name = name.LowerCase();
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > 26)
        bucket = 26;
    for (vector<KillCounter>::iterator it = self->buckets[bucket].begin();
         it != self->buckets[bucket].end();
         it++)
    {
        if (name == (*it).name)
        {
            (*it).count++;
            return (*it).count;
        }
    }
    KillCounter killcounter(name);
    self->buckets[bucket].insert(self->buckets[bucket].end(), killcounter);
    return 1;
}

void KillCounters::Add(KillCounters *self, String name, int count)
{
    name = name.LowerCase();
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > 26)
        bucket = 26;
    KillCounter killcounter(name, count);
    self->buckets[bucket].insert(self->buckets[bucket].end(), killcounter);
}

int KillCounters::Get(KillCounters *self, String name)
{
    name = name.LowerCase();
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > 26)
        bucket = 26;
    for (vector<KillCounter>::iterator it = self->buckets[bucket].begin();
         it != self->buckets[bucket].end();
         it++)
    {
        if (name == (*it).name)
            return (*it).count;
    }
    return 0;
}

void KillCounters::Init(KillCounters *self)
{
    self->string_list->Clear();
    try
    {
        self->string_list->LoadFromFile("./cache/kills.chk");
    }
    catch (...)
    {
    }
    for (int i = 0; i < self->string_list->Count; i++)
    {
        self->name = self->string_list->Strings[i];
        String key = Extract(self);
        Add(self, key, StrToInt(Extract(self)));
    }
}

String KillCounters::Extract(KillCounters *self)
{
    int pos = self->name.Pos(";");
    if (pos < 1)
        return self->name;
    String result = self->name.SubString(1, pos - 1);
    self->name.Delete(1, pos);
    return result;
}

void KillCounters::Save(KillCounters *self)
{
    String path = "./cache/kills.chk";
    if (access(path.c_str(), 0) == 0)
    {
        remove(path.c_str());
    }
    self->string_list->Clear();
    vector<KillCounter>::iterator it;
    for (int i = 0; i < 26; i++)
    {
        for (it = self->buckets[i].begin(); it != self->buckets[i].end(); it++)
        {
            String line = (*it).name + ";";
            line = line + IntToStr((*it).count) + ";";
            self->string_list->Add(line);
        }
    }
    self->string_list->SaveToFile("./cache/kills.chk");
}
