#ifndef MapwarpH
#define MapwarpH

#include <Classes.hpp>

// Recovered from the reference (Mapwarp unit, 0x487de4..0x487e64). The
// constructor stores the six arguments in order at offsets 0x0/0x2/0x4/0x8/0xC/
// 0xE and returns `this`; the class holds only these six plain fields
// (sizeof 0x10), so its destructor is emitted solely in the deleting form
// (0x487e48).
struct Mapwarp
{
    short from_x;
    short from_y;
    int dest_map;
    int level;
    short to_x;
    short to_y;

    Mapwarp(short from_x, short from_y, int dest_map, int level, short to_x, short to_y);
    ~Mapwarp();
};

#endif
