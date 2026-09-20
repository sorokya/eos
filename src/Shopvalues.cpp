#include <vcl.h>
#pragma hdrstop

#include "Shopvalues.h"

#pragma package(smart_init)

ShopValues::ShopValues()
{
    field_0x24 = -1;
    loaded = 0;
    LoadShops(this);
}

ShopValues::~ShopValues()
{
}

void ShopValues::LoadShops(ShopValues *self)
{
    if (self->loaded == 0)
    {
        Clear(self);
        String data;
        String path = "./pub/dts001.esf";

        int file_handle;
        int size;
        char *buf;
        try
        {
            file_handle = FileOpen(path.c_str(), 0);
            size = FileSeek(file_handle, 0, 2);
            FileSeek(file_handle, 0, 0);
            buf = new char[size + 1];
            FileRead(file_handle, buf, size);
            FileClose(file_handle);
            data += buf;
            data.SetLength(size);
            delete[] buf;
            if (data[1] != 'E' || data[2] != 'S' || data[3] != 'F')
                return;
            data.Delete(1, 3);
            do
            {
                int behavior_id = self->DecodeNumber(data.SubString(1, 2));
                int namelen = self->DecodeNumber(data.SubString(3, 1));
                ShopValue shop(behavior_id);
                shop.name = data.SubString(4, namelen);
                shop.min_level = self->DecodeNumber(data.SubString(namelen + 4, 1));
                shop.max_level = self->DecodeNumber(data.SubString(namelen + 5, 1));
                shop.class_requirement =
                    self->DecodeNumber(data.SubString(namelen + 6, 1));
                int trades_count = self->DecodeNumber(data.SubString(namelen + 7, 2));
                int crafts_count = self->DecodeNumber(data.SubString(namelen + 9, 1));
                data.Delete(1, namelen + 9);
                for (int i = 0; i < trades_count; i++)
                {
                    AddTrade(self,
                             &shop,
                             self->DecodeNumber(data.SubString(1, 2)),
                             self->DecodeNumber(data.SubString(3, 3)),
                             self->DecodeNumber(data.SubString(6, 3)),
                             self->DecodeNumber(data.SubString(9, 1)));
                    data.Delete(1, 9);
                }
                for (int i = 0; i < crafts_count; i++)
                {
                    AddCraft(self,
                             &shop,
                             self->DecodeNumber(data.SubString(1, 2)),
                             self->DecodeNumber(data.SubString(3, 2)),
                             self->DecodeNumber(data.SubString(5, 1)),
                             self->DecodeNumber(data.SubString(6, 2)),
                             self->DecodeNumber(data.SubString(8, 1)),
                             self->DecodeNumber(data.SubString(9, 2)),
                             self->DecodeNumber(data.SubString(0xb, 1)),
                             self->DecodeNumber(data.SubString(0xc, 2)),
                             self->DecodeNumber(data.SubString(0xe, 1)));
                    data.Delete(1, 0xe);
                }
                self->record_list.insert(self->record_list.end(), shop);
            } while (data.Length() > 9);
            self->loaded = 1;
        }
        catch (...)
        {
            FileClose(file_handle);
            self->loaded = 0;
        }
    }
}

void ShopValues::AddTrade(ShopValues *self,
                          ShopValue *shop,
                          int item_id,
                          int buy_price,
                          int sell_price,
                          int max_amount)
{
    shop->AddTrade(item_id, buy_price, sell_price, max_amount);
}

void ShopValues::AddCraft(ShopValues *self,
                          ShopValue *shop,
                          int item_id,
                          int ingredient_item_id_1,
                          int ingredient_amount_1,
                          int ingredient_item_id_2,
                          int ingredient_amount_2,
                          int ingredient_item_id_3,
                          int ingredient_amount_3,
                          int ingredient_item_id_4,
                          int ingredient_amount_4)
{
    shop->AddCraft(item_id,
                   ingredient_item_id_1,
                   ingredient_amount_1,
                   ingredient_item_id_2,
                   ingredient_amount_2,
                   ingredient_item_id_3,
                   ingredient_amount_3,
                   ingredient_item_id_4,
                   ingredient_amount_4);
}

void ShopValues::Clear(ShopValues *self)
{
    std::vector<ShopValue>::iterator it = self->record_list.begin();
    while (it != self->record_list.end())
    {
        it->trades.clear();
        it->crafts.clear();
        it++;
    }
    if ((unsigned int)GetCount(self) >= 1)
    {
        self->record_list.clear();
        self->field_0x24 = -1;
    }
}

ShopCraftIngredient
ShopValues::GetCraftIngredient1(ShopValues *self, int shop_id, int craft_id)
{
    ShopCraftIngredient result;
    result.item_id = -1;
    result.amount = -1;
    if (shop_id > 0)
    {
        if ((int)self->record_list.size() >= shop_id)
        {
            if (self->record_list[shop_id - 1].crafts.size() > 0)
            {
                std::vector<ShopCraftVal>::iterator it =
                    self->record_list[shop_id - 1].crafts.begin();
                while (it != self->record_list[shop_id - 1].crafts.end())
                {
                    if (craft_id == it->id)
                    {
                        result.item_id = it->ingredient_item_ids[0];
                        result.amount = it->ingredient_amounts[0];
                        break;
                    }
                    it++;
                }
            }
        }
    }
    return result;
}

ShopCraftIngredient
ShopValues::GetCraftIngredient2(ShopValues *self, int shop_id, int craft_id)
{
    ShopCraftIngredient result;
    result.item_id = -1;
    result.amount = -1;
    if (shop_id > 0)
    {
        if ((int)self->record_list.size() >= shop_id)
        {
            if (self->record_list[shop_id - 1].crafts.size() > 0)
            {
                std::vector<ShopCraftVal>::iterator it =
                    self->record_list[shop_id - 1].crafts.begin();
                while (it != self->record_list[shop_id - 1].crafts.end())
                {
                    if (craft_id == it->id)
                    {
                        result.item_id = it->ingredient_item_ids[1];
                        result.amount = it->ingredient_amounts[1];
                        break;
                    }
                    it++;
                }
            }
        }
    }
    return result;
}

ShopCraftIngredient
ShopValues::GetCraftIngredient3(ShopValues *self, int shop_id, int craft_id)
{
    ShopCraftIngredient result;
    result.item_id = -1;
    result.amount = -1;
    if (shop_id > 0)
    {
        if ((int)self->record_list.size() >= shop_id)
        {
            if (self->record_list[shop_id - 1].crafts.size() > 0)
            {
                std::vector<ShopCraftVal>::iterator it =
                    self->record_list[shop_id - 1].crafts.begin();
                while (it != self->record_list[shop_id - 1].crafts.end())
                {
                    if (craft_id == it->id)
                    {
                        result.item_id = it->ingredient_item_ids[2];
                        result.amount = it->ingredient_amounts[2];
                        break;
                    }
                    it++;
                }
            }
        }
    }
    return result;
}

ShopCraftIngredient
ShopValues::GetCraftIngredient4(ShopValues *self, int shop_id, int craft_id)
{
    ShopCraftIngredient result;
    result.item_id = -1;
    result.amount = -1;
    if (shop_id > 0)
    {
        if ((int)self->record_list.size() >= shop_id)
        {
            if (self->record_list[shop_id - 1].crafts.size() > 0)
            {
                std::vector<ShopCraftVal>::iterator it =
                    self->record_list[shop_id - 1].crafts.begin();
                while (it != self->record_list[shop_id - 1].crafts.end())
                {
                    if (craft_id == it->id)
                    {
                        result.item_id = it->ingredient_item_ids[3];
                        result.amount = it->ingredient_amounts[3];
                        break;
                    }
                    it++;
                }
            }
        }
    }
    return result;
}

int ShopValues::GetBuyPrice(ShopValues *self, int shop_id, int item_id, int amount)
{
    int result = -1;
    if (shop_id > 0)
    {
        if ((int)self->record_list.size() >= shop_id)
        {
            if (self->record_list[shop_id - 1].trades.size() > 0)
            {
                std::vector<ShopItemVal>::iterator it =
                    self->record_list[shop_id - 1].trades.begin();
                while (it != self->record_list[shop_id - 1].trades.end())
                {
                    if (item_id == it->item_id && it->buy_price > 0)
                    {
                        result = it->buy_price;
                        result = result * amount;
                        break;
                    }
                    it++;
                }
            }
        }
    }
    return result;
}

int ShopValues::GetSellPrice(ShopValues *self, int shop_id, int item_id, int amount)
{
    int result = -1;
    if (shop_id > 0)
    {
        if ((int)self->record_list.size() >= shop_id)
        {
            if (self->record_list[shop_id - 1].trades.size() > 0)
            {
                std::vector<ShopItemVal>::iterator it =
                    self->record_list[shop_id - 1].trades.begin();
                while (it != self->record_list[shop_id - 1].trades.end())
                {
                    if (item_id == it->item_id && it->sell_price > 0)
                    {
                        result = it->sell_price;
                        result = result * amount;
                        break;
                    }
                    it++;
                }
            }
        }
    }
    return result;
}

int ShopValues::GetCount(ShopValues *self)
{
    return self->record_list.size();
}

String ShopValues::EncodeNumber(ShopValues *self, unsigned int value, int width)
{
    unsigned int v = value;
    String result = "";
    try
    {
        unsigned int quotient;
        do
        {
            int rem;
            char c;
            double d = v / 253.0;
            quotient = d;
            rem = v % 0xfd;
            c = rem + 1;
            result.Insert(c, result.Length() + 1);
            if (quotient >= 1)
                v = quotient;
        } while (quotient >= 1);
    }
    catch (...)
    {
        result = "";
    }
    if (result.Length() < width)
    {
        char pad = 0xfe;
        int count = width - result.Length();
        for (int i = 0; i < count; i++)
            result.Insert(pad, result.Length() + 1);
    }
    return result;
}

String ShopValues::BuildOpenData(ShopValues *self, int behavior_id)
{
    String result = "";
    std::vector<ShopValue>::iterator it = self->record_list.begin();
    std::vector<ShopItemVal>::iterator trade_iter;
    std::vector<ShopCraftVal>::iterator craft_iter;
    while (it != self->record_list.end())
    {
        if (it->id == behavior_id)
        {
            result += EncodeNumber(self, it->id, 2);
            result.Insert(it->name, result.Length() + 1);
            result.Insert((char)0xff, result.Length() + 1);
            if (it->trades.size() > 0)
                for (trade_iter = it->trades.begin(); trade_iter != it->trades.end();
                     trade_iter++)
                {
                    result.Insert(EncodeNumber(self, trade_iter->item_id, 2),
                                  result.Length() + 1);
                    result.Insert(EncodeNumber(self, trade_iter->buy_price, 3),
                                  result.Length() + 1);
                    result.Insert(EncodeNumber(self, trade_iter->sell_price, 3),
                                  result.Length() + 1);
                    result.Insert(EncodeNumber(self, trade_iter->max_amount, 1),
                                  result.Length() + 1);
                }
            result.Insert((char)0xff, result.Length() + 1);
            if (it->crafts.size() > 0)
                for (craft_iter = it->crafts.begin(); craft_iter != it->crafts.end();
                     craft_iter++)
                {
                    result.Insert(EncodeNumber(self, craft_iter->id, 2),
                                  result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_item_ids[0], 2),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_amounts[0], 1),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_item_ids[1], 2),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_amounts[1], 1),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_item_ids[2], 2),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_amounts[2], 1),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_item_ids[3], 2),
                        result.Length() + 1);
                    result.Insert(
                        EncodeNumber(self, craft_iter->ingredient_amounts[3], 1),
                        result.Length() + 1);
                }
        }
        it++;
    }
    if (result == "")
        result += EncodeNumber(self, 0, 2);
    return result;
}

int ShopValues::DecodeNumber(String value)
{
    String value_copy = value;
    int result = 0;
    try
    {
        int byte_index = 1;
        while (value_copy.Length() >= byte_index)
        {
            char c = value_copy[byte_index];
            unsigned char ch = c;
            if (ch == 0xfe || ch == 0)
                break;
            int n = ch;
            n = n - 1;
            if (byte_index == 1)
                result = result + n;
            if (byte_index == 2)
                result = result + n * 0xfd;
            if (byte_index == 3)
                result = result + n * 0xfa09;
            if (byte_index == 4)
                result = result + n * 0xf71ae5;
            byte_index = byte_index + 1;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}
