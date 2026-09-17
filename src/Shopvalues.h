#ifndef ShopvaluesH
#define ShopvaluesH

#include <Classes.hpp>
#include <vector>
#include "Shopvalue.h"

// The two-int result returned by GetCraftIngredient1..4. The reference calls an
// empty user constructor on the local at entry (the folded EH-frame-only
// constructor at 0x44f58c) before storing -1 in both fields, and returns it in
// two registers; the same shape the Itemvalues pair records use.
struct ShopCraftIngredient
{
    struct { int item_id; int amount; };
    ShopCraftIngredient() {}
};

// The shop table (ESF). Layout recovered from the reference constructor
// (0x4b15c0) and the parser/accessors: the loader reads ./pub/dts001.esf once.
//   +0x00 char                        loaded
//   +0x04 std::vector<ShopValue>      record_list
//   +0x24 int                         field_24 = -1
class ShopValues
{
public:
    char loaded;
    char pad_1[3];
    std::vector<ShopValue> record_list;
    int field_24;

    ShopValues();

    int DecodeNumber(String value);
    static String EncodeNumber(ShopValues *self, unsigned int value, int width);

    static void LoadShops(ShopValues *self);
    static void Clear(ShopValues *self);
    static int GetRecordCount(ShopValues *self);

    static ShopCraftIngredient GetCraftIngredient1(ShopValues *self, int shop_id, int craft_id);
    static ShopCraftIngredient GetCraftIngredient2(ShopValues *self, int shop_id, int craft_id);
    static ShopCraftIngredient GetCraftIngredient3(ShopValues *self, int shop_id, int craft_id);
    static ShopCraftIngredient GetCraftIngredient4(ShopValues *self, int shop_id, int craft_id);

    static int GetBuyPrice(ShopValues *self, int shop_id, int item_id, int amount);
    static int GetSellPrice(ShopValues *self, int shop_id, int item_id, int amount);
    static String BuildOpenData(ShopValues *self, int behavior_id);

    static void AddTrade(ShopValues *self, ShopValue *shop, int item_id, int buy_price,
                         int sell_price, int max_amount);
    static void AddCraft(ShopValues *self, ShopValue *shop, int item_id,
                         int ingredient_item_id_1, int ingredient_amount_1,
                         int ingredient_item_id_2, int ingredient_amount_2,
                         int ingredient_item_id_3, int ingredient_amount_3,
                         int ingredient_item_id_4, int ingredient_amount_4);
};

#endif
