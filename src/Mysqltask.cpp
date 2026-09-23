#include <vcl.h>
#pragma hdrstop

#include "Mysqltask.h"

#pragma package(smart_init)

mySQLtask::mySQLtask(int query_id_,
                     int player_id_,
                     int expected_query_id_,
                     String data_,
                     String query_text_)
{
    query_id = query_id_;
    player_id = player_id_;
    expected_query_id = expected_query_id_;
    data = data_;
    query_text = query_text_;
}

mySQLtask::~mySQLtask()
{
}

mySQLbuffer::mySQLbuffer()
{
    thread = new TCriticalSection;
    last_player_id = -1;
    last_expected_query_id = -1;
}

mySQLbuffer::~mySQLbuffer()
{
    delete thread;
}

void mySQLbuffer::EnqueueTask(mySQLtask *task)
{
    if (task->player_id == last_player_id && task->query_id == last_query_id &&
        task->query_id != QueryId_Direct)
        return;

    for (vector<mySQLtask *>::iterator it = job_queue.begin(); it != job_queue.end();
         ++it)
    {
        if ((*it)->player_id == task->player_id && task->query_id != QueryId_Direct &&
            (*it)->query_id == task->query_id)
            return;
    }

    job_queue.insert(job_queue.end(), task);
}

bool mySQLbuffer::HasPendingTask(int expected_query_id)
{
    for (vector<mySQLtask *>::iterator it = job_queue.begin(); it != job_queue.end();
         ++it)
    {
        if ((*it)->expected_query_id == expected_query_id &&
            (*it)->query_id == QueryId_Direct)
            return true;
    }

    if (last_expected_query_id == expected_query_id && last_query_id == QueryId_Direct)
        return true;

    return false;
}
