#include <vcl.h>
#pragma hdrstop

#include "Mapcontrol.h"

// Caller-form probe for the chest coordinate ABI. The reference passes the
// (x, y) pair to these helpers as a MapCoord by value, which bcc32 lowers to
// `add esp,-8; mov [esp],x; mov [esp+4],y`. Two separate scalars lower to two
// `push`es instead. Player_HandlePacket (Packets.cpp) is the real caller; this
// probe exercises the same declarations so the emitted form can be inspected
// with `bcc32 -S`.
void Chest_CallerProbe(Mapcontrol *map_control, int map_id, int x, int y, int item_id)
{
    MapCoord coords;
    coords.x = x;
    coords.y = y;
    Mapcontrol::Mapcontrol_GetChestSlotCount(map_control, map_id, coords);
    Mapcontrol::Mapcontrol_AddChestItem(map_control, map_id, coords, item_id, 1);
    Mapcontrol::Mapcontrol_TakeChestItem(map_control, map_id, coords, item_id);
    FUN_00486e64((int)map_control, map_id, coords);
    FUN_0047badc(map_control, map_id, coords);
}
