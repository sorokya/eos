#ifndef EventcontrolH
#define EventcontrolH

#include <Classes.hpp>

class Mapcontrol;
class Players;
class Server;
class Settings;

// The map timed-events driver. Layout (sizeof 0x14) is pinned by the reference
// constructor (0x52db20): pEncode_scratch at +0x00 (operator new(8)), settings
// at +0x04, map_control at +0x08, players at +0x0c, server at +0x10. The
// argument order is pinned by the FormCreate call site. The class has no virtual
// functions; the reference destructor (0x52db78) is emitted solely in the
// deleting form.
class EventController
{
  public:
    char *pEncode_scratch;   // +0x00
    Settings *settings;      // +0x04
    Mapcontrol *map_control; // +0x08
    Players *players;        // +0x0c
    Server *server;          // +0x10

    EventController(Mapcontrol *map_control,
                    Players *players,
                    Server *server,
                    Settings *settings);
    ~EventController();

    static void Tick(EventController *self);
    static String AppendEncoded(EventController *self, unsigned int value, int width);
};

#endif
