#ifndef ShopitemH
#define ShopitemH

#include <Classes.hpp>

// Recovered from the reference (Shopitem unit, 0x4b49d4..0x4b4a1e, and the
// Shopvalues parser / BuildOpenData accesses): a 16-byte trade record. The
// constructor stores only item_id; AddTrade fills the remaining three fields.
// On-disk ShopTradeRecord = item_id[short], buy_price[three], sell_price[three],
// max_amount[char] (eo-protocol), widened to int in memory by the ESF decoder.
// The class name is a Ghidra hint; internal names are unobservable in the
// stripped image.
struct ShopItemVal
{
    int item_id;
    int buy_price;
    int sell_price;
    int max_amount;

    ShopItemVal(int item_id);
    ~ShopItemVal();
};

#endif
