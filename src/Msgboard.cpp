#include <vcl.h>
#pragma hdrstop

#include "Msgboard.h"

#pragma package(smart_init)

MsgBoard::MsgBoard()
{
}

MsgBoard::MsgBoard(int post_id,
                   String post_poster,
                   String post_subject,
                   String post_message)
{
    id = post_id;
    poster = post_poster;
    subject = post_subject;
    message = post_message;
}

MsgBoard::~MsgBoard()
{
}
