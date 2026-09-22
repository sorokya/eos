#include <vcl.h>
#pragma hdrstop

#include "Itemchest.h"

#pragma package(smart_init)

ChestItem::ChestItem(int item_id)
{
    this->item_id = item_id;
}

ChestItem::~ChestItem()
{
}
