#ifndef MapchestH
#define MapchestH

#include <vector.h>

#include "Itemchest.h"

// Layout recovered from the reference constructor (0x47ac5c): x/y/key_id are
// stored at +0/+2/+4, updated is zeroed at +6, and the vector<ChestItem>
// member occupies +8 (sizeof(vector<ChestItem>) is 0x20). sizeof = 0x28.
struct MapChest
{
    unsigned short x;
    unsigned short y;
    short key_id;
    char updated;
    vector<ChestItem> slots;

    MapChest();
    MapChest(int x, int y, int key_id);
    ~MapChest();
};

#endif
