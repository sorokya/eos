#ifndef ChestcontrolH
#define ChestcontrolH

#include <Classes.hpp>
#include "Mapcontrol.h"
#include "Players.h"
#include "Server.h"
#include "Settings.h"

class ChestController
{
  public:
    char *encode_scratch;
    Settings *settings;
    MapContainer *map_control;
    Players *players;
    Packets *server;

    ChestController(MapContainer *map_control,
                    Players *players,
                    Packets *server,
                    Settings *settings);
    ~ChestController();

    static void Tick(ChestController *self);
    static String EncodeNumber(ChestController *self, unsigned int value, int width);
    static bool InRange(ChestController *self, int x, int y, int player_x, int player_y);
};

#endif
