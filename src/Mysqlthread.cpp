#include <vcl.h>
#pragma hdrstop

#include <SyncObjs.hpp>
#include <vector>

#include "Mysqlthread.h"
#include "Mysqltask.h"
#include "Mainform.h"

#pragma package(smart_init)

extern void MysqlCallback_Dispatch(Server *server, mySQLtask *query_result);
extern Server *Mainform_GetServer(TGUI *form);

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
            std::vector<mySQLtask *>::iterator it = queue->job_queue.begin();
            job = *it;
            queue->field_2c = job->query_id;
            queue->last_player_id = job->player_id;
            queue->field_28 = job->expected_query_id;
            queue->job_queue.erase(it);
        }
        else
        {
            queue->last_player_id = -1;
            queue->field_28 = -1;
        }

        queue->thread->Release();

        if (job != NULL)
        {
            task = job;
            String query_text = job->param2;
            if (query->Active)
                query->Active = false;
            query->SQL->Clear();
            query->SQL->Add(job->param2);
            if (job->query_id < 0x3c)
            {
                query->Open();
                if (job->query_id != 1)
                    Synchronize(OnResult);
            }
            else
            {
                query->Active = true;
                Synchronize(OnResult);
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
    Server *server = Mainform_GetServer(GUI);
    MysqlCallback_Dispatch(server, task);
}
