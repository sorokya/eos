#ifndef ShopcraftH
#define ShopcraftH

#include <Classes.hpp>

// Recovered from the reference (Shopcraft unit, 0x4b548c..0x4b5500) and the
// Endless Shop File layout (ShopCraftRecord in eo-protocol):
//   ShopCraftRecord = short item_id, ShopCraftIngredientRecord ingredients[4]
//   ShopCraftIngredientRecord = short item_id, char amount
// The constructor stores the crafted item id at offset 0 and zero-fills the
// (int-expanded) ingredient id/amount arrays at offsets 4 and 0x14; the
// destructor is the trivial deleting form. The class name is the RTTI type
// name ("ShopCraftVal"); offsets are confirmed by the constructor's stores.
struct ShopCraftVal
{
    int id;
    int ingredient_item_ids[4];
    int ingredient_amounts[4];

    ShopCraftVal(int id);
    ~ShopCraftVal();
};

#endif
