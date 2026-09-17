#ifndef ItemchestH
#define ItemchestH

// The class defined by the Itemchest unit is MapItem: the reference RTTI
// type-name table carries `std::vector<MapItem,...>` for the vector that
// MapChest holds, and the Itemchest unit exports `@@Itemchest@Initialize`.
// Layout recovered from those field accesses in ChestController::Tick
// (0x4b5594) and the element size 0x2c pinned by the vector grow code at
// 0x4077f4.
struct MapItem
{
    int item_id;                   // +0x00
    int amount;                    // +0x04
    char item_present;             // +0x08
    char respawn_enabled;          // +0x09
    char pad_0a[2];                // +0x0a
    int respawn_countdown;         // +0x0c
    unsigned short respawn_delay;  // +0x10
    unsigned short alt_item_id[4]; // +0x12
    int alt_amount[4];             // +0x1c

    MapItem(int item_id);
    ~MapItem();
};

#endif
