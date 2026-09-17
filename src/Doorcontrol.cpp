#include <vcl.h>
#pragma hdrstop

#include "Doorcontrol.h"
#include "Mapobject.h"

#include <vector>

#pragma package(smart_init)

// Provisional cross-unit views. Mapcontrol and MapContainer are not yet reconstructed;
// only the fields this unit reads are recovered here, at their reference offsets
// (Doorcontrol_Tick, 0x4aafc8). Mapcontrol's first member is the maps vector at
// +0x00; sizeof(Mapcontrol) is 0x44 (pinned by Mainform.cpp's operator new).
struct MapContainer
{
    char pad_00[0x5c];
    std::vector<MapObject> tile_specs; // +0x5c
    char pad_specs[0x125 - 0x5c - sizeof(std::vector<MapObject>)];
    unsigned char has_open_doors; // +0x125
    char pad_126[0x160 - 0x126];
};

class Mapcontrol
{
  public:
    std::vector<MapContainer> maps;
    char pad_04[0x44 - sizeof(std::vector<MapContainer>)];
};

DoorController::DoorController(Mapcontrol *map_control)
{
    this->map_control = map_control;
}

DoorController::~DoorController()
{
}

void DoorController::Tick(DoorController *self)
{
    TTimeStamp now = DateTimeToTimeStamp(Now());
    int days = now.Date - self->last_tick.Date;
    int ms = now.Time - self->last_tick.Time;

    bool elapsed = false;
    if (ms / 1000 + days * 86400 > 1)
        elapsed = true;

    for (std::vector<MapContainer>::iterator map_iter = self->map_control->maps.begin();
         map_iter != self->map_control->maps.end();
         map_iter++)
    {
        if (map_iter->has_open_doors == 0)
            continue;

        bool found = false;
        for (std::vector<MapObject>::iterator spec_iter = map_iter->tile_specs.begin();
             spec_iter != map_iter->tile_specs.end();
             spec_iter++)
        {
            if (spec_iter->value == 9 || spec_iter->value == 0xb)
            {
                found = true;
                if (elapsed)
                    spec_iter->ticks = 0;
                else
                    spec_iter->ticks--;
                if (spec_iter->ticks <= 0)
                {
                    if (spec_iter->value == 9)
                        spec_iter->value = 7;
                    if (spec_iter->value == 0xb)
                        spec_iter->value = 0xa;
                }
            }
        }

        if (!found)
            map_iter->has_open_doors = 0;
    }

    self->last_tick = now;
}
