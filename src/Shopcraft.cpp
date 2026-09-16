#include <vcl.h>
#pragma hdrstop

#include "Shopcraft.h"

#pragma package(smart_init)

ShopCraftVal::ShopCraftVal(int id)
{
    this->id = id;
    for (int i = 0; i < 4; i++)
    {
        ingredient_item_ids[i] = 0;
        ingredient_amounts[i] = 0;
    }
}

ShopCraftVal::~ShopCraftVal()
{
}
