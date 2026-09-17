#ifndef MapobjectH
#define MapobjectH

#include <Classes.hpp>

// Layout recovered from the reference (MapObject unit, 0x487d58..0x487dc4).
// sizeof is 8: the constructor (0x487d58) stores the two coordinates, the tile
// value and a zeroed tick count, and returns the object in eax. The deleting
// destructor (0x487da8) only calls operator delete; the constructor and
// destructor are members, and the constructor enters a 0x24-byte EH frame.
struct MapObject
{
    short x;
    short y;
    short value;
    short ticks;

    MapObject(int x, int y, int value);
    ~MapObject();
};

#endif
