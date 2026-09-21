#ifndef ItemchestH
#define ItemchestH

// The class defined by the Itemchest unit is MapItem: the reference RTTI
// type-name table carries `vector<MapItem,...>` for the vector that
// MapChest holds, and the Itemchest unit exports `@@Itemchest@Initialize`.
// Layout recovered from those field accesses in ChestController::Tick
// (0x4b5594) and the element size 0x2c pinned by the vector grow code at
// 0x4077f4.
struct MapItem
{
    int item_id;                  // +0x00
    int amount;                   // +0x04
    char item_present;            // +0x08
    char respawn_enabled;         // +0x09
    int respawn_countdown;        // +0x0c
    unsigned short respawn_delay; // +0x10
    unsigned short alt_item_id0;  // +0x12
    unsigned short alt_item_id1;  // +0x14
    unsigned short alt_item_id2;  // +0x16
    unsigned short alt_item_id3;  // +0x18
    int alt_amount0;              // +0x1c
    int alt_amount1;              // +0x20
    int alt_amount2;              // +0x24
    int alt_amount3;              // +0x28

    MapItem(int item_id);
    ~MapItem();
};

#endif
