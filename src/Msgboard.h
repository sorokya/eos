#ifndef MsgboardH
#define MsgboardH

#include <Classes.hpp>

// Layout recovered from the reference (offsets in bytes):
//   0  short  id
//   4  String poster
//   8  String subject
//  12  String message
struct MsgBoard
{
    short id;
    String poster;
    String subject;
    String message;

    MsgBoard();
    MsgBoard(int post_id, String post_poster, String post_subject, String post_message);
    ~MsgBoard();
};

#endif
