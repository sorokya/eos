#ifndef InnvalueH
#define InnvalueH

#include <Classes.hpp>

// Recovered from the reference (Innvalue unit, 0x535db4..0x535ecc; accessors
// GetSpawnMap/GetSpawnX/GetSpawnY) and the Endless Inn File layout (InnRecord
// in eo-protocol). The constructor default-constructs `name` (offset 4) and the
// two String[3] question/answer arrays (offsets 0x1c/0x28), then stores the inn
// id at offset 0; the destructor destroys them. Each on-disk char/short
// coordinate is widened to a `short` in memory. Class name is the RTTI type
// name ("InnValue").
struct InnValue
{
    int id;
    String name;
    short spawn_map;
    short spawn_x;
    short spawn_y;
    short sleep_map;
    short sleep_x;
    short sleep_y;
    short alternate_spawn_enabled;
    short alternate_spawn_map;
    short alternate_spawn_x;
    short alternate_spawn_y;
    String question[3];
    String answer[3];

    InnValue(int id);
    ~InnValue();
};

#endif
