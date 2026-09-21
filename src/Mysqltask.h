#ifndef MysqltaskH
#define MysqltaskH

#include <Classes.hpp>
#include <SyncObjs.hpp>
#include <vector.h>

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
// pinned by the `operator new(0x30)` in the Mysqlcontrols constructor:
//   +0x00 TCriticalSection *        thread
//   +0x04 vector<mySQLtask *>  job_queue  (32 bytes, +0x04..+0x24)
//   +0x24 int                       last_player_id
//   +0x28 int                       field_0x28
//   +0x2c int                       field_0x2c
class mySQLbuffer
{
  public:
    TCriticalSection *thread;           // +0x00
    vector<mySQLtask *> job_queue; // +0x04
    int last_player_id;                 // +0x24
    int field_0x28;                     // +0x28
    int field_0x2c;                     // +0x2c

    mySQLbuffer();
    ~mySQLbuffer();

    void EnqueueTask(mySQLtask *task);
    bool HasPendingTask(int player_id);
};

#endif
