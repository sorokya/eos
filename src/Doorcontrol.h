#ifndef DoorcontrolH
#define DoorcontrolH

#include <Classes.hpp>
#include <sysutils.hpp>
#include "Mapcontrol.h"

// Object layout pinned by the reference (DoorController unit, 0x4aaf7c..0x4ab104):
//   +0x00 TTimeStamp last_tick
//   +0x08 MapContainer *map_control
class DoorController
{
  public:
    TTimeStamp last_tick;
    MapContainer *map_control;

    DoorController(MapContainer *map_control);
    ~DoorController();

    static void Tick(DoorController *self);
};

#endif
