#ifndef MysqlcontrolsH
#define MysqlcontrolsH

#include <vector>
#include <Classes.hpp>
#include <DBTables.hpp>
#include <SysUtils.hpp>
#include <SyncObjs.hpp>

// Cross-unit classes (reconstructed separately). Only the layouts and the
// members used by this unit are declared here; their method code lives in their
// own translation units.
class FilecacheEntry;
class FilecacheEntryB;
class Mysqltask;

// Element records held (by pointer) in the Filecache write queues. sizeof is
// pinned by the `operator new` arguments in LoadCachedPlayers (0x18) and
// LoadCachedGuilds (0x14); the field names are the SELECT column names the
// loaders read.
class FilecacheEntry
{
  public:
    int privilege;  // +0x00
    String name;    // +0x04
    String title;   // +0x08
    int level;      // +0x0c
    int experience; // +0x10
    int gender;     // +0x14

    FilecacheEntry();
};

class FilecacheEntryB
{
  public:
    String ident_guild; // +0x00
    String guild;       // +0x04
    int exptotal;       // +0x08
    int exphigh;        // +0x0c
    int members;        // +0x10

    FilecacheEntryB();
};

// Filecache (unit Filecache) is 0x5c bytes; layout recovered from its ctor
// 0x53cd48 and the pending-write loaders 0x53d2a8/0x53d53c:
//   +0x00 char  dirty                     +0x04 int accounts_count
//   +0x08 int   characters_count          +0x0c int guilds_count
//   +0x10 std::vector<FilecacheEntry *>   pending_player_writes
//   +0x30 std::vector<FilecacheEntryB *>  pending_guild_writes
//   +0x50 TStringList * string_list       +0x54 String field_54
//   +0x58 int   field_58
class Filecache
{
  public:
    char dirty; // +0x00
    char pad_01[3];
    int accounts_count;                                  // +0x04
    int characters_count;                                // +0x08
    int guilds_count;                                    // +0x0c
    std::vector<FilecacheEntry *> pending_player_writes; // +0x10
    std::vector<FilecacheEntryB *> pending_guild_writes; // +0x30
    TStringList *string_list;                            // +0x50
    String field_54;                                     // +0x54
    int field_58;                                        // +0x58

    Filecache();
};

class Mysqltask
{
  public:
    int query_id;          // +0x00
    int player_id;         // +0x04
    int expected_query_id; // +0x08
    String data;           // +0x0c
    String param2;         // +0x10

    Mysqltask(
        int query_id, int player_id, int expected_query_id, String data, String param2);
};

// Mysqlthread (unit Mysqlthread); only the members touched by this unit.
class Mysqlthread
{
  public:
    TCriticalSection *thread;           // +0x00
    std::vector<Mysqltask *> job_queue; // +0x04
    int last_player_id;                 // +0x24
    int field_28;                       // +0x28

    Mysqlthread();

    static TThread *__fastcall Spawn(void *vmt,
                                     bool alloc,
                                     TSession *session,
                                     bool create_suspended,
                                     Mysqlthread *thread_queue,
                                     TQuery *query,
                                     TDatabase *db);
    static void EnqueueTask(Mysqlthread *thread, Mysqltask *task);
};

// Mysqlcontrols is the DB layer root (0x28 bytes). Layout from the reference
// constructor 0x474668 and the status refresh 0x4762c8:
//   +0x00 Filecache *   file_cache
//   +0x04 TTimeStamp    last_query_time
//   +0x0c TTimeStamp    connected_time
//   +0x14 int           field_14
//   +0x18 int           query_error_count
//   +0x1c int           exec_error_count
//   +0x20 Mysqlthread * thread_queue
//   +0x24 TThread *     worker_thread
class Mysqlcontrols
{
  public:
    Filecache *file_cache;      // +0x00
    TTimeStamp last_query_time; // +0x04
    TTimeStamp connected_time;  // +0x0c
    int field_14;               // +0x14
    int query_error_count;      // +0x18
    int exec_error_count;       // +0x1c
    Mysqlthread *thread_queue;  // +0x20
    TThread *worker_thread;     // +0x24

    Mysqlcontrols();

    static void Free(Mysqlcontrols *self, unsigned char free_flags);
    static bool TestConnection(Mysqlcontrols *self);
    static void
    Connect(Mysqlcontrols *self, int version_patch, int version_minor, int version_major);

    static void LoadCachedPlayers(Mysqlcontrols *self);
    static void LoadCachedGuilds(Mysqlcontrols *self);
    static void UpdateServerStatus(Mysqlcontrols *self,
                                   int refresh_seconds,
                                   int connections,
                                   int players,
                                   int most,
                                   String upload,
                                   String download);
    static String Db_GetString(Mysqlcontrols *self, String label);
    static int Db_GetInt(Mysqlcontrols *self, String label);
    static void NextResultRecord(Mysqlcontrols *self);
    static bool ResultAtEnd(Mysqlcontrols *self);
    static int GetResultCount(Mysqlcontrols *self);
    static bool Mysql_SubmitQuery(Mysqlcontrols *self,
                                  int query_id,
                                  int player_id,
                                  int expected_query_id,
                                  String data,
                                  String param2);
    static bool Mysql_SubmitQuery_FromCallback(Mysqlcontrols *self,
                                               int query_id,
                                               int player_id,
                                               int expected_query_id,
                                               String data,
                                               String param2);
    static bool Query(Mysqlcontrols *self, String query);
    static bool
    Mysql_ExecDirect(Mysqlcontrols *self, int expected_query_id, String query);
    static bool Mysql_ExecDirect_FromCallback(Mysqlcontrols *self,
                                              int expected_query_id,
                                              String query);
    static bool ExecDrop(Mysqlcontrols *self, String query);
    static int Db_GetActiveConnectionCount(Mysqlcontrols *self);
    static bool Database_CanReconnect(Mysqlcontrols *self);
    static bool IsTaskPending(Mysqlcontrols *self, int player_id);
    static bool IsAsciiText(Mysqlcontrols *self, String value);
    static bool IsAlphabeticText(Mysqlcontrols *self, String value);
    static String Mysql_SanitizeString(Mysqlcontrols *self, String value, bool uppercase);
    static String Db_SanitizeString(Mysqlcontrols *self, String value);
    static void NormalizePlayerText(Mysqlcontrols *self, String &message);
    static TTimeStamp Server_GetUptime(Mysqlcontrols *self);
    static String DecodeString(Mysqlcontrols *self, String value);
};

#endif
