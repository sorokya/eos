#ifndef ShopvalueH
#define ShopvalueH

#include <Classes.hpp>
#include <vector>
#include "Shopitem.h"
#include "Shopcraft.h"

// Recovered from the reference (Shopvalue unit, 0x4b4ae8..0x4b4bf3). The
// constructor default-constructs `name` (offset 4) and the trade/craft vectors
// (offsets 0x10/0x30), stores the shop id at offset 0 and clears both vectors;
// the destructor destroys crafts, trades then name and deletes. The vectors are
// std::vector<ShopItemVal> and std::vector<ShopCraftVal> per the RTTI. The two
// int fields at 8/0xc are placeholders for the ShopRecord header fields
// (min_level/max_level/class_requirement) pending the Shopvalues parser.
struct ShopValue
{
    int id;
    String name;
    int field_8;
    int field_c;
    std::vector<ShopItemVal> trades;
    std::vector<ShopCraftVal> crafts;

    ShopValue(int id);
    ~ShopValue();
};

#endif
