#ifndef EffectcontrolH
#define EffectcontrolH

#include <Classes.hpp>
#include "Settings.h"

class Mapcontrol;
class Players;
class Server;

class EffectController
{
  public:
    int aState_countdown[4];
    int aState_value[4];
    int aState_extra[4];
    int nBroadcast_gate;
    char *pEncode_scratch;
    Settings *settings;
    Mapcontrol *map_control;
    Players *players;
    Server *server;

    EffectController(Mapcontrol *map_control,
                     Players *players,
                     Server *server,
                     Settings *settings);
    ~EffectController();

    static void Tick(EffectController *self);
    static String EncodeNumber(EffectController *self, unsigned int value, int width);
};

#endif
