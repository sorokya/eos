#ifndef ChestcontrolH
#define ChestcontrolH

#include <Classes.hpp>

class Mapcontrol;
class Players;
class Server;
class Settings;

class Chestcontrol
{
  public:
    char *encode_scratch;
    Settings *settings;
    Mapcontrol *map_control;
    Players *players;
    Server *server;

    Chestcontrol(Mapcontrol *map_control,
                 Players *players,
                 Server *server,
                 Settings *settings);
    ~Chestcontrol();

    static void Tick(Chestcontrol *self);
    static String AppendEncoded(Chestcontrol *self, unsigned int value, int width);
    static bool InRange(Chestcontrol *self, int x, int y, int player_x, int player_y);
};

#endif
