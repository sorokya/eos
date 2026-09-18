#ifndef WeaponmapH
#define WeaponmapH

#include <Classes.hpp>

// Recovered from the reference (Weaponmap unit, 0x4b5be4..0x4b5c60):
//   - an 8-byte class whose constructor is trivial (no member stores) and whose
//     destructor is emitted out-of-line as the deleting form
//     `if (this && (flags & 1)) operator delete(this)`;
//   - `Combat_IsRangedWeapon`, a free-function-form unit operation whose first
//     parameter is not referenced by the body.
// Member semantics are not yet established; the layout is pinned only to the
// observed size of 8 bytes.
struct WeaponmapEntry
{
    int field_0;
    int field_4;

    WeaponmapEntry();
    ~WeaponmapEntry();
};

bool Combat_IsRangedWeapon(void *unused, int doll_graphic_id);

#endif
