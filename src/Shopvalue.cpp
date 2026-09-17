#include <vcl.h>
#pragma hdrstop

#include "Shopvalue.h"

#pragma package(smart_init)

ShopValue::ShopValue(int id)
{
    this->id = id;
    trades.clear();
    crafts.clear();
}

ShopValue::~ShopValue()
{
}

void ShopValue::AddTrade(int item_id, int buy_price, int sell_price, int max_amount)
{
    ShopItemVal val(item_id);
    val.buy_price = buy_price;
    val.sell_price = sell_price;
    val.max_amount = max_amount;
    trades.insert(trades.end(), val);
}

void ShopValue::AddCraft(int item_id,
                         int ingredient_item_id_1,
                         int ingredient_amount_1,
                         int ingredient_item_id_2,
                         int ingredient_amount_2,
                         int ingredient_item_id_3,
                         int ingredient_amount_3,
                         int ingredient_item_id_4,
                         int ingredient_amount_4)
{
    ShopCraftVal val(item_id);
    val.ingredient_item_ids[0] = ingredient_item_id_1;
    val.ingredient_amounts[0] = ingredient_amount_1;
    val.ingredient_item_ids[1] = ingredient_item_id_2;
    val.ingredient_amounts[1] = ingredient_amount_2;
    val.ingredient_item_ids[2] = ingredient_item_id_3;
    val.ingredient_amounts[2] = ingredient_amount_3;
    val.ingredient_item_ids[3] = ingredient_item_id_4;
    val.ingredient_amounts[3] = ingredient_amount_4;
    crafts.insert(crafts.end(), val);
}
