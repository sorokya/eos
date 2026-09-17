#include <vcl.h>
#pragma hdrstop

#include "Packets.h"
#include "Player.h"

#pragma package(smart_init)

// The reference helpers are the out-of-line copies of `std::vector<T>::begin`/
// `end`; `-v` keeps them as calls, so the bodies reduce to the vector's start
// and finish pointers. `Mapcontrol::maps` is the vector at +0x00, whose
// `_M_start`/`_M_finish` land at +0x04/+0x08 (pinned by Mapcontrol_GetCount's
// end-minus-begin over 0x160 and by the type's constructor).
MapContainer *MapVector_Begin(Mapcontrol *map_control)
{
    return *(MapContainer **)((char *)map_control + 0x04);
}

MapContainer *MapVector_End(Mapcontrol *map_control)
{
    return *(MapContainer **)((char *)map_control + 0x08);
}

int Mapcontrol_GetCount(Mapcontrol *map_control)
{
    return map_control->maps.end() - map_control->maps.begin();
}

MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index)
{
    return MapVector_Begin(map_control) + index;
}

int Math_Abs(int value)
{
    return __abs__(value);
}
