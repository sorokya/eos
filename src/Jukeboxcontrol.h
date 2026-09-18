#ifndef JukeboxcontrolH
#define JukeboxcontrolH

#include <Classes.hpp>
#include <vector>
#include "Jukebox.h"

// Recovered from the reference (JukeBoxController unit, 0x4a9b54..0x4aa9b8).
// Object layout pinned by the constructor:
//   +0x00 char *                  encode_scratch = operator new(8), an encode buffer
//   +0x04 std::vector<JukeBox>    recent_plays (stride 0x18 = sizeof(JukeBox))
class JukeBoxController
{
  public:
    char *encode_scratch;
    std::vector<JukeBox> recent_plays;

    JukeBoxController();

    static String EncodeNumber(JukeBoxController *self, unsigned int value, int width);
    static String BuildRecentTracksString(JukeBoxController *self, int map_id);
    static bool TryPlayTrack(JukeBoxController *self, int map_id, String track);
};

#endif
