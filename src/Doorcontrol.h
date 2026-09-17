#ifndef DoorcontrolH
#define DoorcontrolH

#include <Classes.hpp>
#include <sysutils.hpp>

class Mapcontrol;

// Object layout pinned by the reference (Doorcontrol unit, 0x4aaf7c..0x4ab104):
//   +0x00 TTimeStamp last_tick
//   +0x08 Mapcontrol *map_control
class Doorcontrol
{
  public:
    TTimeStamp last_tick;
    Mapcontrol *map_control;

    Doorcontrol(Mapcontrol *map_control);
    ~Doorcontrol();

    static void Tick(Doorcontrol *self);
};

#endif
