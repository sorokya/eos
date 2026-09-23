#include <vcl.h>
#pragma hdrstop

#include <SyncObjs.hpp>
#include <vector.h>

#include "MySQLthread.h"
#include "Mysqltask.h"
#include "MainForm.h"

#pragma package(smart_init)

extern void MysqlCallback_Dispatch(Packets *server, mySQLtask *query_result);

__fastcall MySQLthread::MySQLthread(TSession *session_,
                                    TDatabase *database_,
                                    TQuery *query_,
                                    mySQLbuffer *queue_,
                                    bool CreateSuspended)
    : TThread(CreateSuspended)
{
    Priority = tpLower;
    session = session_;
    database = database_;
    query = query_;
    queue = queue_;
    FreeOnTerminate = true;
}

void __fastcall MySQLthread::Execute()
{
    while (!Terminated)
    {
        mySQLtask *job = NULL;

        queue->thread->Acquire();

        if (queue->job_queue.size() > 0)
        {
            vector<mySQLtask *>::iterator it = queue->job_queue.begin();
            job = *it;
            queue->last_query_id = job->query_id;
            queue->last_player_id = job->player_id;
            queue->last_expected_query_id = job->expected_query_id;
            queue->job_queue.erase(it);
        }
        else
        {
            queue->last_player_id = -1;
            queue->last_expected_query_id = -1;
        }

        queue->thread->Release();

        if (job != NULL)
        {
            task = job;
            String query_text = task->query_text;
            if (query->Active)
                query->Active = false;
            query->SQL->Clear();
            query->SQL->Add(task->query_text);
            try
            {
                if (task->query_id < 0x3c)
                {
                    query->ExecSQL();
                    if (task->query_id != QueryId_Direct)
                        Synchronize(OnResult);
                }
                else
                {
                    query->Active = true;
                    Synchronize(OnResult);
                }
            }
            catch (...)
            {
            }
        }

        if (job == NULL)
            Suspend();
        else
        {
            delete job;
            Sleep(0x19);
        }
    }
}

void __fastcall MySQLthread::OnResult()
{
    Packets *server = Mainform_GetServer(GUI);
    MysqlCallback_Dispatch(server, task);
}
