#ifndef WeddingsH
#define WeddingsH

#include <Classes.hpp>
#include <vector>
#include "Wedding.h"

class Players;
class Server;

// Layout recovered from the reference constructor (0x52e37c); class name from the
// RTTI type name `WeddingController`, vector element type from the RTTI string
// `std::vector<Wedding *,std::allocator<Wedding *> >`. sizeof is 0x2c:
//   +0x00 char *                  encode_scratch = new char[8] (base-253 buffer)
//   +0x04 Players *               players
//   +0x08 Server *                server
//   +0x0c std::vector<Wedding *>  active_weddings (sizeof 0x20)
class WeddingController
{
  public:
    char *encode_scratch;
    Players *players;
    Server *server;
    std::vector<Wedding *> active_weddings;

    WeddingController(Players *players, Server *server);
    ~WeddingController();

    static bool Has(WeddingController *self, int map_id, int priest_line);
    static void Add(WeddingController *self,
                    int map_id,
                    int priest_line,
                    int player1_id,
                    String player1_name,
                    int player2_id,
                    String player2_name);
    static void
    Confirm(WeddingController *self, int map_id, int priest_line, int player_id);
    static void Tick(WeddingController *self);
    static void
    BroadcastPriestLine(WeddingController *self, Wedding *record, String text);
    static bool BothPresent(WeddingController *self, Wedding *record);
    static String AppendEncoded(WeddingController *self, unsigned int value, int width);
};

#endif
