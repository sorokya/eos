#ifndef WeddingH
#define WeddingH

#include <Classes.hpp>

// Layout recovered from the reference (Wedding unit, 0x52e224..0x52e35c).
// sizeof is 0x28 (operator new(0x28) at the Weddings::Add site, 0x52e611). The
// constructor (0x52e224) default-constructs the two AnsiString members at
// +0xc/+0x18, then stores map_id/priest_line and clears the two byte flags at
// +0x10/+0x1c. The deleting destructor (0x52e2a0) destroys the strings at
// +0x18 then +0xc. player1_id (+0x08) and player2_id (+0x14) are the arguments
// to Players_GetById in Weddings_BothPresent; priest_line (+0x04) is the value
// encoded by Weddings_BroadcastPriestLine; step (+0x20) and countdown (+0x24)
// drive the Weddings_Tick sequencer (initialized 0 and 0x14 by the Add site).
struct Wedding
{
    int map_id;          // +0x00
    int priest_line;     // +0x04
    int player1_id;      // +0x08
    String player1_name; // +0x0c
    char field_10;       // +0x10
    int player2_id;      // +0x14
    String player2_name; // +0x18
    char field_1c;       // +0x1c
    int step;            // +0x20
    int countdown;       // +0x24

    Wedding(int map_id, int priest_line);
    ~Wedding();
};

#endif
