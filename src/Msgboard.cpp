#include <vcl.h>
#pragma hdrstop

#include "Msgboard.h"

#pragma package(smart_init)

Msgboard::Msgboard()
{
}

Msgboard::Msgboard(short post_id,
                   String post_poster,
                   String post_subject,
                   String post_message)
{
    id = post_id;
    poster = post_poster;
    subject = post_subject;
    message = post_message;
}

Msgboard::~Msgboard()
{
}
