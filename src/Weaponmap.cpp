#include <vcl.h>
#pragma hdrstop

#include "Weaponmap.h"

#pragma package(smart_init)

WeaponMapper::WeaponMapper()
{
}

WeaponMapper::~WeaponMapper()
{
}

bool Combat_IsRangedWeapon(void *unused, int doll_graphic_id)
{
    bool ranged = false;
    if (doll_graphic_id == 0x2a || doll_graphic_id == 0x2b || doll_graphic_id == 0x31 ||
        doll_graphic_id == 0x32 || doll_graphic_id == 0x3a || doll_graphic_id == 0x49)
        ranged = true;
    return ranged;
}
