#ifndef MsgboardcontrolH
#define MsgboardcontrolH

#include <vector.h>
#include "Msgboard.h"

// Number of message boards the controller holds (the `boards[8]` inline array).
#define MSG_BOARD_COUNT 8

// Recovered from the reference (MsgBoardController unit, 0x4ab128..0x4ae180).
// Object layout pinned by the constructor (0x4ab15c) and the board reader
// (0x4ad190):
//   +0x000 int                       max_posts
//   +0x004 int                       next_post_id
//   +0x008 vector<MsgBoard>     boards[8]      inline fixed array
//   +0x108 String                    misc_text
//   +0x10c int                       decode_pos
//   +0x110 int                       decode_length
//   +0x114 char                      decode_delimiter
//   +0x118 char *                    encode_buffer = operator new(8), encode scratch
//   +0x11c char                      aBoard_enabled[8]
//   +0x124 String                    aBoard_names[8]
//   +0x144 int                       poster_lengths[32]
//   +0x1c4 int                       subject_lengths[32]
//   +0x244 int                       message_lengths[32]
//   +0x2c4 int                       extra_lengths[32]
//   +0x344 String                    aExtra_strings[32]
// sizeof = 0x3c4.
class MsgBoardController
{
  public:
    int max_posts;
    int next_post_id;
    vector<MsgBoard> boards[MSG_BOARD_COUNT];
    String misc_text;
    int decode_pos;
    int decode_length;
    char decode_delimiter;
    char *encode_buffer;
    char aBoard_enabled[MSG_BOARD_COUNT];
    String aBoard_names[MSG_BOARD_COUNT];
    int poster_lengths[32];
    int subject_lengths[32];
    int message_lengths[32];
    int extra_lengths[32];
    String aExtra_strings[32];

    MsgBoardController();
    ~MsgBoardController();

    static bool LoadBoards(MsgBoardController *self);
    static void SaveBoards(MsgBoardController *self);
    static String EncodeNumber(MsgBoardController *self, unsigned int value, int width);
    static void ClearBoard(MsgBoardController *self, int board);
    static void DeletePost(MsgBoardController *self, int board, int post_id);
    static int CountPosts(MsgBoardController *self, int board, String author);
    static void AddPost(MsgBoardController *self,
                        int board,
                        String poster,
                        String subject,
                        String message,
                        char flag);
    static String GetBoard(MsgBoardController *self, int board);
    static String GetPost(MsgBoardController *self, int board, int post_id);
    static void BuildBoardName(MsgBoardController *self, int board);
    static String BuildBoardData(MsgBoardController *self, int board);
    static void LoadBoard(MsgBoardController *self, int board, String data);
    static void SetDecodeSource(MsgBoardController *self, String data, char delimiter);
    static String ReadToken(MsgBoardController *self);
    static String ReadRest(MsgBoardController *self);
    static int DecodeNumber(MsgBoardController *self, String value);
};

#endif
