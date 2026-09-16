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
