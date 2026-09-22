#include <vcl.h>
#include <io.h>
#include <stdio.h>
#pragma hdrstop

#include "Filecache.h"

#pragma package(smart_init)

FileCache::FileCache()
{
    dirty = 0;
    accounts_count = 0;
    characters_count = 0;
    guilds_count = 0;
    field_0x58 = 0;
    string_list = new TStringList;
}

FileCache::~FileCache()
{
}

void FileCache::CheckCacheFile(FileCache *self)
{
    String path = "./cache/cacheok.chk";
    if (access(path.c_str(), 0) == 0)
    {
        self->dirty = 1;
        remove(path.c_str());
    }
}

void FileCache::UpdatePlayerCache(FileCache *self, char *record)
{
    if (self->field_0x58 < *(int *)(record + 0xc0) && *(int *)(record + 0x98) == 0)
    {
        vector<TopPlayer *>::iterator it;
        TopPlayer *last = 0;
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
        self->field_0x58 = min_experience;
    }
}

void FileCache::LoadPlayerCache(FileCache *self)
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
        self->field_0x54 = self->string_list->Strings[i];
        TopPlayer *entry = new TopPlayer;
        entry->privilege = StrToInt(NextToken(self));
        entry->name = NextToken(self);
        entry->title = NextToken(self);
        entry->level = StrToInt(NextToken(self));
        entry->experience = StrToInt(NextToken(self));
        entry->gender = StrToInt(NextToken(self));
        self->pending_player_writes.insert(self->pending_player_writes.end(), entry);
    }
}

void FileCache::LoadGuildCache(FileCache *self)
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
        self->field_0x54 = self->string_list->Strings[i];
        TopGuild *entry = new TopGuild;
        entry->ident_guild = NextToken(self);
        entry->guild = NextToken(self);
        entry->exptotal = StrToInt(NextToken(self));
        entry->members = StrToInt(NextToken(self));
        self->pending_guild_writes.insert(self->pending_guild_writes.end(), entry);
    }
}

String FileCache::NextToken(FileCache *self)
{
    int pos = self->field_0x54.Pos(";");
    if (pos < 1)
        return self->field_0x54;
    String result = self->field_0x54.SubString(1, pos - 1);
    self->field_0x54.Delete(1, pos);
    return result;
}

void Database_FlushCache(FileCache *self)
{
    self->string_list->Clear();
    if (self->pending_player_writes.size() > 99)
    {
        for (vector<TopPlayer *>::iterator it = self->pending_player_writes.begin();
             it != self->pending_player_writes.end();
             it++)
        {
            String line = IntToStr((*it)->privilege) + ";";
            line = line + (*it)->name + ";";
            line = line + (*it)->title + ";";
            line = line + IntToStr((*it)->level) + ";";
            line = line + IntToStr((*it)->experience) + ";";
            line = line + IntToStr((*it)->gender) + ";";
            self->string_list->Add(line);
        }
        self->string_list->SaveToFile("./cache/players.chk");
    }
    self->string_list->Clear();
    if (self->pending_guild_writes.size() > 99)
    {
        for (vector<TopGuild *>::iterator it = self->pending_guild_writes.begin();
             it != self->pending_guild_writes.end();
             it++)
        {
            String line = (*it)->ident_guild + ";";
            line = line + (*it)->guild + ";";
            line = line + IntToStr((*it)->exptotal) + ";";
            line = line + IntToStr((*it)->members) + ";";
            self->string_list->Add(line);
        }
        self->string_list->SaveToFile("./cache/guilds.chk");
    }
    FILE *fp = fopen("./cache/cacheok.chk", "wb");
    fclose(fp);
}

TopPlayer::TopPlayer()
{
}

TopPlayer::~TopPlayer()
{
}

TopGuild::TopGuild()
{
}

TopGuild::~TopGuild()
{
}
