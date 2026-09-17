#ifndef NpcDropItemH
#define NpcDropItemH

#include <Classes.hpp>

// Recovered from the reference (NpcDropItem unit, 0x4a9ae8..0x4a9b31): the
// constructor stores its argument at offset 0 and the destructor is emitted
// out-of-line as the deleting form. Callers place the object in a 16-byte
// stack record and fill the remaining fields themselves. The full layout is
// pinned by Npcvalues::AddDrop (0x4a9298) and Npc_GetDrop (0x4a6e50); the
// on-disk DropRecord is item_id[short], min_amount[three], max_amount[three],
// rate[short] (eo-protocol), widened to int in memory by the EDF decoder.
// The class name is a placeholder (internal names are unobservable in the
// stripped image).
struct NpcDropItem
{
    int item_id;
    int min_amount;
    int max_amount;
    int rate;

    NpcDropItem(int item_id);
    ~NpcDropItem();
};

#endif
