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
    field_0x28 = -1;
}

mySQLbuffer::~mySQLbuffer()
{
    delete thread;
}

void mySQLbuffer::EnqueueTask(mySQLtask *task)
{
    if (task->player_id == last_player_id && task->query_id == field_0x2c &&
        task->query_id != 1)
        return;

    for (std::vector<mySQLtask *>::iterator it = job_queue.begin(); it != job_queue.end();
         ++it)
    {
        if ((*it)->player_id == task->player_id && task->query_id != 1 &&
            (*it)->query_id == task->query_id)
            return;
    }

    job_queue.insert(job_queue.end(), task);
}

bool mySQLbuffer::HasPendingTask(int player_id)
{
    for (std::vector<mySQLtask *>::iterator it = job_queue.begin(); it != job_queue.end();
         ++it)
    {
        if ((*it)->expected_query_id == player_id && (*it)->query_id == 1)
            return true;
    }

    if (field_0x28 == player_id && field_0x2c == 1)
        return true;

    return false;
}
