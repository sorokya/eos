#ifndef EffectcontrolH
#define EffectcontrolH

#include <Classes.hpp>
#include "Settings.h"
#include "Mapcontrol.h"
#include "Players.h"
#include "Server.h"

class EffectController
{
  public:
    int aState_countdown[4];
    int aState_value[4];
    int aState_extra[4];
    int nBroadcast_gate;
    char *pEncode_scratch;
    Settings *settings;
    MapContainer *map_control;
    Players *players;
    Packets *server;

    EffectController(MapContainer *map_control,
                     Players *players,
                     Packets *server,
                     Settings *settings);
    ~EffectController();

    static void Tick(EffectController *self);
    static String EncodeNumber(EffectController *self, unsigned int value, int width);
};

#endif
