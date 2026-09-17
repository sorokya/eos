#ifndef PlayerquestH
#define PlayerquestH

#include <Classes.hpp>

// Layout recovered from the reference (Playerquest unit, 0x537530..0x5375b4):
// the constructor stores the quest id at offset 0, two shorts at 4/6, clears the
// five-element short array at 8 and the flag byte at 0x12; the element stride of
// the owning std::vector is 0x14. The class name is the RTTI type name
// ("PlayerQuest").
struct PlayerQuest
{
    int quest_id;
    short field_4;
    short field_6;
    short field_8[5];
    char field_12;

    PlayerQuest(int quest_id, short a, short b);
    ~PlayerQuest();
};

#endif
