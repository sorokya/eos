#ifndef MysqlcontrolsH
#define MysqlcontrolsH

#include <vector.h>
#include <Classes.hpp>
#include <DBTables.hpp>
#include <SysUtils.hpp>
#include <SyncObjs.hpp>

#include "Mysqltask.h"
#include "MySQLthread.h"
#include "Filecache.h"

// mySQLdb is the DB layer root (0x28 bytes). Layout from the reference
// constructor 0x474668 and the status refresh 0x4762c8:
//   +0x00 FileCache *    file_cache
//   +0x04 TTimeStamp     last_query_time
//   +0x0c TTimeStamp     connected_time
//   +0x14 int            field_0x14
//   +0x18 int            query_error_count
//   +0x1c int            exec_error_count
//   +0x20 mySQLbuffer *  thread_queue
//   +0x24 MySQLthread *  worker_thread
class mySQLdb
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

    mySQLdb();
    ~mySQLdb();

    static bool TestConnection(mySQLdb *self);
    static void
    Connect(mySQLdb *self, int version_major, int version_minor, int version_patch);

    static void LoadCachedPlayers(mySQLdb *self);
    static void LoadCachedGuilds(mySQLdb *self);
    static void UpdateServerStatus(mySQLdb *self,
                                   int refresh_seconds,
                                   int connections,
                                   int players,
                                   int most,
                                   String upload,
                                   String download);
    static String Db_GetString(mySQLdb *self, String label);
    static int Db_GetInt(mySQLdb *self, String label);
    static void NextResultRecord(mySQLdb *self);
    static bool ResultAtEnd(mySQLdb *self);
    static int GetResultCount(mySQLdb *self);
    static bool Mysql_SubmitQuery(mySQLdb *self,
                                  int query_id,
                                  int player_id,
                                  int expected_query_id,
                                  String data,
                                  String query_text);
    static bool Mysql_SubmitQuery_FromCallback(mySQLdb *self,
                                               int query_id,
                                               int player_id,
                                               int expected_query_id,
                                               String data,
                                               String query_text);
    static bool Query(mySQLdb *self, String query);
    static bool Mysql_ExecDirect(mySQLdb *self, int expected_query_id, String query);
    static bool
    Mysql_ExecDirect_FromCallback(mySQLdb *self, int expected_query_id, String query);
    static bool ExecDrop(mySQLdb *self, String query);
    static unsigned int Db_GetActiveConnectionCount(mySQLdb *self);
    static bool Database_CanReconnect(mySQLdb *self);
    static bool IsTaskPending(mySQLdb *self, int player_id);
    static bool IsAsciiText(mySQLdb *self, String value);
    static bool IsAlphabeticText(mySQLdb *self, String value);
    static String Mysql_SanitizeString(mySQLdb *self, String value, bool uppercase);
    static String Db_SanitizeString(mySQLdb *self, String value);
    static void NormalizePlayerText(mySQLdb *self, String &message);
    static TTimeStamp Server_GetUptime(mySQLdb *self);
    static String DecodeString(mySQLdb *self, String value);
};

#endif
