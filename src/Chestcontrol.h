#ifndef ChestcontrolH
#define ChestcontrolH

#include <Classes.hpp>

class Mapcontrol;
class Players;
class Server;
class Settings;

class ChestController
{
  public:
    char *encode_scratch;
    Settings *settings;
    Mapcontrol *map_control;
    Players *players;
    Server *server;

    ChestController(Mapcontrol *map_control,
                    Players *players,
                    Server *server,
                    Settings *settings);
    ~ChestController();

    static void Tick(ChestController *self);
    static String EncodeNumber(ChestController *self, unsigned int value, int width);
    static bool InRange(ChestController *self, int x, int y, int player_x, int player_y);
};

#endif
