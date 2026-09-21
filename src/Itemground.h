#ifndef ItemgroundH
#define ItemgroundH

#include <Classes.hpp>
#include <SysUtils.hpp>

// Layout recovered from the reference (Itemground unit, 0x488494..0x4884d8): the
// constructor is trivial (no member stores; just the EH frame and returning
// `this`) and the destructor is emitted out-of-line as the deleting form.
// Mapcontrol_AddGroundItem allocates 0x24 bytes and fills index/item_id/x/y/
// amount at +0x00..+0x10, a TTimeStamp at +0x14, owner_player_id at +0x1c and
// protect_ticks at +0x20. The class name is ChestItem: the reference RTTI
// type-name table carries `vector<ChestItem,...>` for MapContainer's
// ground-item list, which holds ChestItem pointers.
struct ChestItem
{
    int index;            // +0x00
    int item_id;          // +0x04
    int x;                // +0x08
    int y;                // +0x0c
    int amount;           // +0x10
    TTimeStamp drop_time; // +0x14
    int owner_player_id;  // +0x1c
    short protect_ticks;  // +0x20
    char pad_0x22[2];     // +0x22

    ChestItem();
    ~ChestItem();
};

#endif
