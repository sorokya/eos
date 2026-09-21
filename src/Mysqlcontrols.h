#ifndef MysqlcontrolsH
#define MysqlcontrolsH

#include <vector.h>
#include <Classes.hpp>
#include <DBTables.hpp>
#include <SysUtils.hpp>
#include <SyncObjs.hpp>

#include "Mysqltask.h"
#include "Mysqlthread.h"
#include "Filecache.h"

// Mysqlcontrols is the DB layer root (0x28 bytes). Layout from the reference
// constructor 0x474668 and the status refresh 0x4762c8:
//   +0x00 FileCache *    file_cache
//   +0x04 TTimeStamp     last_query_time
//   +0x0c TTimeStamp     connected_time
//   +0x14 int            field_0x14
//   +0x18 int            query_error_count
//   +0x1c int            exec_error_count
//   +0x20 mySQLbuffer *  thread_queue
//   +0x24 MySQLthread *  worker_thread
class Mysqlcontrols
{
  public:
    FileCache *file_cache;      // +0x00
    TTimeStamp last_query_time; // +0x04
    TTimeStamp connected_time;  // +0x0c
    int field_0x14;             // +0x14
    int query_error_count;      // +0x18
    int exec_error_count;       // +0x1c
    mySQLbuffer *thread_queue;  // +0x20
    MySQLthread *worker_thread; // +0x24

    Mysqlcontrols();
    ~Mysqlcontrols();

    static void Free(Mysqlcontrols *self, unsigned char free_flags);
    static bool TestConnection(Mysqlcontrols *self);
    static void
    Connect(Mysqlcontrols *self, int version_major, int version_minor, int version_patch);

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
                                  String query_text);
    static bool Mysql_SubmitQuery_FromCallback(Mysqlcontrols *self,
                                               int query_id,
                                               int player_id,
                                               int expected_query_id,
                                               String data,
                                               String query_text);
    static bool Query(Mysqlcontrols *self, String query);
    static bool
    Mysql_ExecDirect(Mysqlcontrols *self, int expected_query_id, String query);
    static bool Mysql_ExecDirect_FromCallback(Mysqlcontrols *self,
                                              int expected_query_id,
                                              String query);
    static bool ExecDrop(Mysqlcontrols *self, String query);
    static unsigned int Db_GetActiveConnectionCount(Mysqlcontrols *self);
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
