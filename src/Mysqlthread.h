#ifndef MysqlthreadH
#define MysqlthreadH

#include <Classes.hpp>
#include <DBTables.hpp>
#include "Mysqltask.h"

// mySQLtask and mySQLbuffer are defined in the Mysqltask unit; this unit only
// holds pointers to them.

// TThread-derived worker (class name from the reference RTTI, VA 0x533720 and
// the VMT name string at 0x580845). Layout: TThread occupies 0x00..0x2f, then
//   +0x30 TSession *  +0x34 TDatabase *  +0x38 TQuery *
//   +0x3c int         +0x40 mySQLtask *  +0x44 mySQLbuffer *
// sizeof is 0x48 (pinned by the class-descriptor size at 0x58080c).
class MySQLthread : public TThread
{
  public:
    TSession *session;   // +0x30
    TDatabase *database; // +0x34
    TQuery *query;       // +0x38
    int field_0x3c;      // +0x3c
    mySQLtask *task;     // +0x40
    mySQLbuffer *queue;  // +0x44

    __fastcall MySQLthread(TSession *session_,
                           TDatabase *database_,
                           TQuery *query_,
                           mySQLbuffer *queue_,
                           bool CreateSuspended);

    virtual void __fastcall Execute();

  private:
    void __fastcall OnResult();
};

#endif
