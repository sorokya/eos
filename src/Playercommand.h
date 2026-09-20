#ifndef PlayercommandH
#define PlayercommandH

#include <Classes.hpp>

// Recovered from the reference (Playercommand unit, 0x53e3c0..0x53e493). The
// RTTI type name is "PlayerCommand" (referenced by the Player action queue as
// std::vector<PlayerCommand>, std::allocator<PlayerCommand>). Layout: an int at
// +0, an int at +4, and an AnsiString at +8; the constructor stores the two ints
// and assigns the by-value string, the destructor is the normal deleting form.
struct PlayerCommand
{
    int family;
    int action;
    String text;

    PlayerCommand(int family, int action, String text);
    ~PlayerCommand();
};

#endif
