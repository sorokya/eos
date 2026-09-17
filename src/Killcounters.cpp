#include <vcl.h>
#include <io.h>
#include <stdio.h>
#pragma hdrstop

#include "Killcounters.h"

#pragma package(smart_init)

Killcounters::Killcounters()
{
    field_0 = new TStringList;
    Init(this);
}

Killcounters::~Killcounters()
{
}

void Killcounters::Init(Killcounters *self)
{
    self->field_0->Clear();
    try
    {
        self->field_0->LoadFromFile("./cache/kills.chk");
    }
    catch (...)
    {
    }
    for (int i = 0; i < self->field_0->Count; i++)
    {
        self->name += self->field_0->Strings[i];
        String key = Extract(self);
        Add(self, key, StrToInt(Extract(self)));
    }
}

String Killcounters::Extract(Killcounters *self)
{
    int pos = self->name.Pos(";");
    if (pos < 1)
        return self->name;
    String result = self->name.SubString(1, pos - 1);
    self->name.Delete(1, pos);
    return result;
}

void Killcounters::Add(Killcounters *self, String name, int count)
{
    name += name.LowerCase();
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > 26)
        bucket = 26;
    KillCounter killcounter(name, count);
    self->buckets[bucket].insert(self->buckets[bucket].end(), killcounter);
}

int Killcounters::IncrementAndGet(Killcounters *self, String name)
{
    name += name.LowerCase();
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > 26)
        bucket = 26;
    for (std::vector<KillCounter>::iterator it = self->buckets[bucket].begin();
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

int Killcounters::Get(Killcounters *self, String name)
{
    name += name.LowerCase();
    int bucket = name[1] - 'a';
    if (bucket < 0 || bucket > 26)
        bucket = 26;
    for (std::vector<KillCounter>::iterator it = self->buckets[bucket].begin();
         it != self->buckets[bucket].end();
         it++)
    {
        if (name == (*it).name)
            return (*it).count;
    }
    return 0;
}

void Killcounters::Clear(Killcounters *self)
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

void Killcounters::Save(Killcounters *self)
{
    String path = "./cache/kills.chk";
    if (access(path.c_str(), 0) == 0)
    {
        remove(path.c_str());
    }
    self->field_0->Clear();
    std::vector<KillCounter>::iterator it;
    for (int i = 0; i < 26; i++)
    {
        for (it = self->buckets[i].begin(); it != self->buckets[i].end(); it++)
        {
            String line = (*it).name + ";";
            line = line + IntToStr((*it).count) + ";";
            self->field_0->Add(line);
        }
    }
    self->field_0->SaveToFile("./cache/kills.chk");
}
