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

bool Combat_IsRangedWeapon(void *unused, int weapon_graphic_id)
{
    bool ranged = false;
    if (weapon_graphic_id == 0x2a || weapon_graphic_id == 0x2b ||
        weapon_graphic_id == 0x31 || weapon_graphic_id == 0x32 ||
        weapon_graphic_id == 0x3a || weapon_graphic_id == 0x49)
        ranged = true;
    return ranged;
}
