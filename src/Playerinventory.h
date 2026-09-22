#ifndef PlayerinventoryH
#define PlayerinventoryH

#include <Classes.hpp>

// Recovered from the reference (PlayerInventory unit, 0x411ff0..0x412039): the
// constructor stores its argument at offset 0 and the destructor is emitted
// out-of-line as the deleting form. Callers fill offset 4, so sizeof is 8; the
// field roles (item id / stack amount) come from those cross-unit call sites.
// The class name is a placeholder (internal names are unobservable in the
// stripped image).
struct PlayerInventory
{
    int item_id;
    unsigned int amount;

    PlayerInventory(int id);
    ~PlayerInventory();
};

#endif
