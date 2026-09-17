#ifndef MsgboardcontrolH
#define MsgboardcontrolH

#include <vector>
#include <fstream>
#include "Msgboard.h"

// Recovered from the reference (Msgboardcontrol unit, 0x4ab128..0x4ae180).
// Object layout pinned by the constructor (0x4ab15c) and the board reader
// (0x4ad190):
//   +0x000 int                       field_0
//   +0x004 int                       field_4
//   +0x008 std::vector<Msgboard>     boards[8]      inline fixed array
//   +0x108 String                    misc_text
//   +0x10c int                       field_10c
//   +0x110 int                       field_110
//   +0x114 char                      field_114
//   +0x118 char *                    field_118 = operator new(8), encode buffer
//   +0x11c char                      aBoard_enabled[8]
//   +0x124 String                    aBoard_names[8]
//   +0x144 int                       field_144[32]
//   +0x1c4 int                       field_1c4[32]
//   +0x244 int                       field_244[32]
//   +0x2c4 int                       field_2c4[32]
//   +0x344 String                    aExtra_strings[32]
// sizeof = 0x3c4.
class Msgboardcontrol
{
  public:
    int field_0;
    int field_4;
    std::vector<Msgboard> boards[8];
    String misc_text;
    int field_10c;
    int field_110;
    char field_114;
    char *field_118;
    char aBoard_enabled[8];
    String aBoard_names[8];
    int field_144[32];
    int field_1c4[32];
    int field_244[32];
    int field_2c4[32];
    String aExtra_strings[32];

    Msgboardcontrol();
    ~Msgboardcontrol();

    static int LoadBoards(Msgboardcontrol *self);
    static void SaveBoards(Msgboardcontrol *self);
    static String AppendEncoded(Msgboardcontrol *self, unsigned int value, int width);
    static void ClearBoard(Msgboardcontrol *self, int board);
    static void DeletePost(Msgboardcontrol *self, int board, int post_id);
    static int CountPosts(Msgboardcontrol *self, int board, String author);
    static void AddPost(Msgboardcontrol *self,
                        int board,
                        String poster,
                        String subject,
                        String message,
                        char flag);
    static void SetPostLimit(std::vector<Msgboard> *posts, unsigned int count);
    static String GetBoard(Msgboardcontrol *self, int board);
    static String GetPost(Msgboardcontrol *self, int board, int post_id);
    static void BuildBoardName(Msgboardcontrol *self, int board);
    static String BuildBoardData(Msgboardcontrol *self, int board);
    static void LoadBoard(Msgboardcontrol *self, int board, String data);
    static void SetDecodeSource(Msgboardcontrol *self, String data, char delimiter);
    static String ReadToken(Msgboardcontrol *self);
    static String ReadRest(Msgboardcontrol *self);
    static int DecodeNumber(Msgboardcontrol *self, String value);
};

#endif
