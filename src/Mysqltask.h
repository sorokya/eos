#ifndef MysqltaskH
#define MysqltaskH

#include <Classes.hpp>
#include <SyncObjs.hpp>
#include <vector.h>

// Callback routing id carried by mySQLtask::query_id. Each value matches the
// SQL submitted from the corresponding Packets.cpp handler and is switched on
// in MysqlCallback_Dispatch. QueryId_Direct is the no-result ExecSQL marker
// Mysql_ExecDirect uses.
enum QueryId
{
    QueryId_Direct = 1,
    QueryId_Login = 0x40,
    QueryId_CharacterList = 0x41,
    QueryId_AccountByIdent = 0x42,
    QueryId_AccountNameCheck = 0x43,
    QueryId_CreateAccount = 0x44,
    QueryId_CharacterNameCheck = 0x45,
    QueryId_CreateCharacter = 0x46,
    QueryId_GuildRank = 0x47,
    QueryId_GuildMemberRank = 0x48,
    QueryId_GuildMemberLookup = 0x49,
    QueryId_GuildDescription = 0x4a,
    QueryId_GuildRanks = 0x4b,
    QueryId_GuildMoney = 0x4c,
    QueryId_GuildMembers = 0x4d,
    QueryId_GuildInfo = 0x4e,
    QueryId_GuildLeaders = 0x4f,
    QueryId_GuildCreateRequest = 0x50,
    QueryId_GuildCreate = 0x51,
    QueryId_GuildAccept = 0x52,
    QueryId_TopGuilds = 0x53
};

// A queued database job (RTTI type name `mySQLtask`, class descriptor at
// 0x5807.. referencing the deleting destructor 0x5338fc). sizeof is 0x14,
// pinned by the `operator new(0x14)` at the submit call sites:
//   +0x00 int    query_id
//   +0x04 int    player_id
//   +0x08 int    expected_query_id
//   +0x0c String data
//   +0x10 String query_text
class mySQLtask
{
  public:
    int query_id;          // +0x00
    int player_id;         // +0x04
    int expected_query_id; // +0x08
    String data;           // +0x0c
    String query_text;     // +0x10

    mySQLtask(int query_id,
              int player_id,
              int expected_query_id,
              String data,
              String query_text);
    ~mySQLtask();
};

// The shared job queue (RTTI type name `mySQLbuffer`, class descriptor at
// 0x5807.. referencing the deleting destructor 0x533b10). sizeof is 0x30,
// pinned by the `operator new(0x30)` in the mySQLdb constructor:
//   +0x00 TCriticalSection *        thread
//   +0x04 vector<mySQLtask *>  job_queue  (32 bytes, +0x04..+0x24)
//   +0x24 int                       last_player_id
//   +0x28 int                       last_expected_query_id
//   +0x2c int                       last_query_id
class mySQLbuffer
{
  public:
    TCriticalSection *thread;      // +0x00
    vector<mySQLtask *> job_queue; // +0x04
    int last_player_id;            // +0x24
    int last_expected_query_id;    // +0x28
    int last_query_id;             // +0x2c

    mySQLbuffer();
    ~mySQLbuffer();

    void EnqueueTask(mySQLtask *task);
    bool HasPendingTask(int expected_query_id);
};

#endif
