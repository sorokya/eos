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
// std::vector<ShopItemVal> and std::vector<ShopCraftVal> per the RTTI. Offsets
// 8/0xa/0xc hold the ShopRecord header fields min_level/max_level/
// class_requirement, each stored as a short by Pub_LoadShops (0x4b1b11,
// 0x4b1b66, 0x4b1bbb); 8..0xf keeps the same 8-byte extent the constructor
// matched with.
struct ShopValue
{
    int id;
    String name;
    short min_level;
    short max_level;
    short class_requirement;
    short pad_0e;
    std::vector<ShopItemVal> trades;
    std::vector<ShopCraftVal> crafts;

    ShopValue(int id);
    ~ShopValue();

    void AddTrade(int item_id, int buy_price, int sell_price, int max_amount);
    void AddCraft(int item_id, int ingredient_item_id_1, int ingredient_amount_1,
                  int ingredient_item_id_2, int ingredient_amount_2,
                  int ingredient_item_id_3, int ingredient_amount_3,
                  int ingredient_item_id_4, int ingredient_amount_4);
};

#endif
