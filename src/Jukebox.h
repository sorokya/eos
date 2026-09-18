#ifndef JukeboxH
#define JukeboxH

#include <Classes.hpp>

// Recovered from the reference (Jukebox unit, 0x4aa9c8..0x4aaa6e). The
// constructor takes a short id, stores it at offset 0, clears the byte at 2,
// zero-constructs the double at 8 and default-constructs the AnsiString at
// 0x10 under an EH scope. The destructor destroys only the AnsiString, so the
// 8-byte member is unmanaged POD. The class name is the RTTI type name
// (`std::vector<JukeBox,std::allocator<JukeBox> >`).
struct JukeBox
{
    short map_id;
    char active;
    TDateTime last_play;
    AnsiString track_name;

    JukeBox(int map_id);
    ~JukeBox();
};

#endif
