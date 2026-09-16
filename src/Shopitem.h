#ifndef ShopitemH
#define ShopitemH

#include <Classes.hpp>

// Recovered from the reference (Shopitem unit, 0x4b49d4..0x4b4a1e): a class with
// a single int member at offset 0, a constructor storing it, and an out-of-line
// non-virtual destructor that lowers to the deleting form.
// The class name is a Ghidra hint; internal names are unobservable in the
// stripped image. Member semantics are unverified.
struct ShopItemVal
{
    int item_id;

    ShopItemVal(int item_id);
    ~ShopItemVal();
};

#endif
