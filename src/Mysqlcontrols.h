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
// pinned by the `operator new` arguments in FUN_004752d4 (0x18) and FUN_00475b20
// (0x14) and by the AnsiString/int field writes.
class FilecacheEntry
{
  public:
    int field_0;    // +0x00
    String field_4; // +0x04
    String field_8; // +0x08
    int field_c;    // +0x0c
    int field_10;   // +0x10
    int field_14;   // +0x14

    FilecacheEntry();
};

class FilecacheEntryB
{
  public:
    String field_0; // +0x00
    String field_4; // +0x04
    int field_8;    // +0x08
    int field_c;    // +0x0c
    int field_10;   // +0x10

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
    char _pad[0x14];

    Mysqltask(int query_id, int player_id, int expected_query_id, String p5, String p6);
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
//   +0x18 int           field_18
//   +0x1c int           field_1c
//   +0x20 Mysqlthread * thread_queue
//   +0x24 TThread *     worker_thread
class Mysqlcontrols
{
  public:
    Filecache *file_cache;      // +0x00
    TTimeStamp last_query_time; // +0x04
    TTimeStamp connected_time;  // +0x0c
    int field_14;               // +0x14
    int field_18;               // +0x18
    int field_1c;               // +0x1c
    Mysqlthread *thread_queue;  // +0x20
    TThread *worker_thread;     // +0x24

    Mysqlcontrols();

    static void Free(Mysqlcontrols *self, unsigned char free_flags);
    static bool TestConnection(Mysqlcontrols *self);
    static void
    Connect(Mysqlcontrols *self, int version_patch, int version_minor, int version_major);

    static void FUN_004752d4(Mysqlcontrols *self);
    static void FUN_00475b20(Mysqlcontrols *self);
    static void FUN_004762c8(Mysqlcontrols *self,
                             int refresh,
                             int conn,
                             int idle,
                             int stat,
                             String a,
                             String b);
    static String Db_GetString(Mysqlcontrols *self, String label);
    static int Db_GetInt(Mysqlcontrols *self, String label);
    static void FUN_00476bfc(Mysqlcontrols *self);
    static bool FUN_00476c14(Mysqlcontrols *self);
    static int GetResultCount(Mysqlcontrols *self);
    static bool Mysql_SubmitQuery(Mysqlcontrols *db,
                                  int query_id,
                                  int player_id,
                                  int expected_query_id,
                                  String p5,
                                  String p6);
    static bool Mysql_SubmitQuery_FromCallback(Mysqlcontrols *db,
                                               int query_id,
                                               int player_id,
                                               int expected_query_id,
                                               String p5,
                                               String p6);
    static bool Query(Mysqlcontrols *self, String query);
    static bool Mysql_ExecDirect(Mysqlcontrols *db, int unk, String query);
    static bool Mysql_ExecDirect_FromCallback(Mysqlcontrols *db, int unk, String query);
    static bool ExecDrop(Mysqlcontrols *self, String query);
    static int Db_GetActiveConnectionCount(Mysqlcontrols *db);
    static bool Database_CanReconnect(Mysqlcontrols *db);
    static void FUN_004772a0(Mysqlcontrols *self, int value);
    static bool FUN_004772b8(Mysqlcontrols *self, String value);
    static bool SpamGuard_CheckCooldown(Mysqlcontrols *self, String value);
    static String Mysql_SanitizeString(Mysqlcontrols *db, String value, bool uppercase);
    static String Db_SanitizeString(Mysqlcontrols *db, String value);
    static void Chat_HasBadWords(Mysqlcontrols *ctx, String &message);
    static TTimeStamp Server_GetUptime(Mysqlcontrols *db);
    static String FUN_00477a00(Mysqlcontrols *db, String value);
};

#endif
