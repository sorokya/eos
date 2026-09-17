#ifndef JukeboxcontrolH
#define JukeboxcontrolH

#include <Classes.hpp>
#include <vector>
#include "Jukebox.h"

// Recovered from the reference (Jukeboxcontrol unit, 0x4a9b54..0x4aa9b8).
// Object layout pinned by the constructor:
//   +0x00 char *                  field_0 = operator new(8), an encode buffer
//   +0x04 std::vector<JukeBox>    recent_plays (stride 0x18 = sizeof(JukeBox))
class Jukeboxcontrol
{
public:
    char *field_0;
    std::vector<JukeBox> recent_plays;

    Jukeboxcontrol();

    static String EncodeNumber(Jukeboxcontrol *self, int value, int width);
    static String BuildRecentTracksString(Jukeboxcontrol *self, int npc_id);
};

#endif
