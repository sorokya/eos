#ifndef MsgboardcontrolH
#define MsgboardcontrolH

#include <vector>
#include <fstream.h>
#include "Msgboard.h"

// Recovered from the reference (MsgBoardController unit, 0x4ab128..0x4ae180).
// Object layout pinned by the constructor (0x4ab15c) and the board reader
// (0x4ad190):
//   +0x000 int                       field_0x0
//   +0x004 int                       field_0x4
//   +0x008 std::vector<MsgBoard>     boards[8]      inline fixed array
//   +0x108 String                    misc_text
//   +0x10c int                       field_0x10c
//   +0x110 int                       field_0x110
//   +0x114 char                      field_0x114
//   +0x118 char *                    field_0x118 = operator new(8), encode buffer
//   +0x11c char                      aBoard_enabled[8]
//   +0x124 String                    aBoard_names[8]
//   +0x144 int                       field_0x144[32]
//   +0x1c4 int                       field_0x1c4[32]
//   +0x244 int                       field_0x244[32]
//   +0x2c4 int                       field_0x2c4[32]
//   +0x344 String                    aExtra_strings[32]
// sizeof = 0x3c4.
class MsgBoardController
{
  public:
    int field_0x0;
    int field_0x4;
    std::vector<MsgBoard> boards[8];
    String misc_text;
    int field_0x10c;
    int field_0x110;
    char field_0x114;
    char *field_0x118;
    char aBoard_enabled[8];
    String aBoard_names[8];
    int field_0x144[32];
    int field_0x1c4[32];
    int field_0x244[32];
    int field_0x2c4[32];
    String aExtra_strings[32];

    MsgBoardController();
    ~MsgBoardController();

    static bool LoadBoards(MsgBoardController *self);
    static void SaveBoards(MsgBoardController *self);
    static String AppendEncoded(MsgBoardController *self, unsigned int value, int width);
    static void ClearBoard(MsgBoardController *self, int board);
    static void DeletePost(MsgBoardController *self, int board, int post_id);
    static int CountPosts(MsgBoardController *self, int board, String author);
    static void AddPost(MsgBoardController *self,
                        int board,
                        String poster,
                        String subject,
                        String message,
                        char flag);
    static void SetPostLimit(std::vector<MsgBoard> *posts, unsigned int count);
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
