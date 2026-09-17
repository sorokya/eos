#include <vcl.h>
#include <io.h>
#include <stdio.h>
#pragma hdrstop

#include "Filecache.h"

#pragma package(smart_init)

FilecacheEntry::FilecacheEntry()
{
}

FilecacheEntry::~FilecacheEntry()
{
}

FilecacheEntryB::FilecacheEntryB()
{
}

FilecacheEntryB::~FilecacheEntryB()
{
}

Filecache::Filecache()
{
    dirty = 0;
    accounts_count = 0;
    characters_count = 0;
    guilds_count = 0;
    field_58 = 0;
    string_list = new TStringList;
}

Filecache::~Filecache()
{
}

void Filecache::CheckCacheFile(Filecache *self)
{
    String path = "./cache/cacheok.chk";
    if (access(path.c_str(), 0) == 0)
    {
        self->dirty = 1;
        remove(path.c_str());
    }
}

String Filecache::FUN_0053d754(Filecache *self)
{
    int pos = self->field_54.Pos(";");
    if (pos < 1)
        return self->field_54;
    String result = self->field_54.SubString(1, pos - 1);
    self->field_54.Delete(1, pos);
    return result;
}

void Filecache::LoadPlayerCache(Filecache *self)
{
    self->string_list->Clear();
    try
    {
        self->string_list->LoadFromFile("./cache/players.chk");
    }
    catch (...)
    {
    }
    for (int i = 0; i < self->string_list->Count; i++)
    {
        self->field_54 = self->string_list->Strings[i];
        FilecacheEntry *entry = new FilecacheEntry;
        entry->privilege = StrToInt(FUN_0053d754(self));
        entry->name = FUN_0053d754(self);
        entry->title = FUN_0053d754(self);
        entry->level = StrToInt(FUN_0053d754(self));
        entry->experience = StrToInt(FUN_0053d754(self));
        entry->gender = StrToInt(FUN_0053d754(self));
        self->pending_player_writes.insert(self->pending_player_writes.end(), entry);
    }
}

void Filecache::LoadGuildCache(Filecache *self)
{
    self->string_list->Clear();
    try
    {
        self->string_list->LoadFromFile("./cache/guilds.chk");
    }
    catch (...)
    {
    }
    for (int i = 0; i < self->string_list->Count; i++)
    {
        self->field_54 = self->string_list->Strings[i];
        FilecacheEntryB *entry = new FilecacheEntryB;
        entry->ident_guild = FUN_0053d754(self);
        entry->guild = FUN_0053d754(self);
        entry->exptotal = StrToInt(FUN_0053d754(self));
        entry->members = StrToInt(FUN_0053d754(self));
        self->pending_guild_writes.insert(self->pending_guild_writes.end(), entry);
    }
}

void Filecache::FUN_0053d0e8(Filecache *self, char *record)
{
    if (self->field_58 < *(int *)(record + 0xc0) && *(int *)(record + 0x98) == 0)
    {
        std::vector<FilecacheEntry *>::iterator it;
        FilecacheEntry *last = 0;
        bool found = false;
        int min_experience = 0x7ffffff8;
        for (it = self->pending_player_writes.begin();
             it != self->pending_player_writes.end();
             it++)
        {
            if ((*it)->name == *(String *)(record + 0xa8))
            {
                (*it)->experience = *(int *)(record + 0xc0);
                (*it)->title = *(String *)(record + 0xb0);
                (*it)->level = *(int *)(record + 0xc4);
                (*it)->gender = *(int *)(record + 0xcc);
                found = true;
            }
            if ((*it)->experience < min_experience)
            {
                min_experience = (*it)->experience;
                last = *it;
            }
        }
        if (found == false && last != 0)
        {
            if (last->experience < *(int *)(record + 0xc0))
            {
                last->name = *(String *)(record + 0xa8);
                last->experience = *(int *)(record + 0xc0);
                last->title = *(String *)(record + 0xb0);
                last->level = *(int *)(record + 0xc4);
                last->gender = *(int *)(record + 0xcc);
            }
            min_experience = 0x7ffffff8;
            for (it = self->pending_player_writes.begin();
                 it != self->pending_player_writes.end();
                 it++)
            {
                if ((*it)->experience < min_experience)
                    min_experience = (*it)->experience;
            }
        }
        self->field_58 = min_experience;
    }
}

void Database_FlushCache(Filecache *cache)
{
    cache->string_list->Clear();
    if (cache->pending_player_writes.size() > 99)
    {
        for (std::vector<FilecacheEntry *>::iterator it =
                 cache->pending_player_writes.begin();
             it != cache->pending_player_writes.end();
             it++)
        {
            String line = IntToStr((*it)->privilege) + ";";
            line = line + (*it)->name + ";";
            line = line + (*it)->title + ";";
            line = line + IntToStr((*it)->level) + ";";
            line = line + IntToStr((*it)->experience) + ";";
            line = line + IntToStr((*it)->gender) + ";";
            cache->string_list->Add(line);
        }
        cache->string_list->SaveToFile("./cache/players.chk");
    }
    cache->string_list->Clear();
    if (cache->pending_guild_writes.size() > 99)
    {
        for (std::vector<FilecacheEntryB *>::iterator it =
                 cache->pending_guild_writes.begin();
             it != cache->pending_guild_writes.end();
             it++)
        {
            String line = (*it)->ident_guild + ";";
            line = line + (*it)->guild + ";";
            line = line + IntToStr((*it)->exptotal) + ";";
            line = line + IntToStr((*it)->members) + ";";
            cache->string_list->Add(line);
        }
        cache->string_list->SaveToFile("./cache/guilds.chk");
    }
    FILE *fp = fopen("./cache/cacheok.chk", "wb");
    fclose(fp);
}
