#ifndef FilecacheH
#define FilecacheH

#include <vector>
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

class Filecache
{
  public:
    char dirty;
    char pad_01[3];
    int accounts_count;
    int characters_count;
    int guilds_count;
    std::vector<FilecacheEntry *> pending_player_writes;
    std::vector<FilecacheEntryB *> pending_guild_writes;
    TStringList *string_list;
    String field_54;
    int field_58;

    Filecache();
    ~Filecache();

    static void CheckCacheFile(Filecache *self);
    static void LoadPlayerCache(Filecache *self);
    static void LoadGuildCache(Filecache *self);
    static String FUN_0053d754(Filecache *self);
    static void FUN_0053d0e8(Filecache *self, char *record);
};

void Database_FlushCache(Filecache *cache);

#endif
