#ifndef FilecacheH
#define FilecacheH

#include <vector.h>
#include <Classes.hpp>

class FilecacheEntry
{
  public:
    int privilege;
    String name;
    String title;
    int level;
    int experience;
    int gender;

    FilecacheEntry();
    ~FilecacheEntry();
};

class FilecacheEntryB
{
  public:
    String ident_guild;
    String guild;
    int exptotal;
    int exphigh;
    int members;

    FilecacheEntryB();
    ~FilecacheEntryB();
};

class FileCache
{
  public:
    char dirty;
    char pad_0x1[3];
    int accounts_count;
    int characters_count;
    int guilds_count;
    vector<FilecacheEntry *> pending_player_writes;
    vector<FilecacheEntryB *> pending_guild_writes;
    TStringList *string_list;
    String field_0x54;
    int field_0x58;

    FileCache();
    ~FileCache();

    static void CheckCacheFile(FileCache *self);
    static void LoadPlayerCache(FileCache *self);
    static void LoadGuildCache(FileCache *self);
    static String NextToken(FileCache *self);
    static void UpdatePlayerCache(FileCache *self, char *record);
};

void Database_FlushCache(FileCache *self);

#endif
