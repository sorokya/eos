#ifndef MapchestH
#define MapchestH

#include <vector>

// sizeof(Itemchest) is 0x2c, recovered from the std::vector<Itemchest> deleting
// destructor (0x4077f4), whose deallocation element count divides the byte span
// by 0x2c. Placeholder definition pending the Itemchest unit's reconstruction.
struct Itemchest
{
    char data[0x2c];
};

// Layout recovered from the reference constructor (0x47ac5c): x/y/key_id are
// stored at +0/+2/+4, updated is zeroed at +6, and the std::vector<Itemchest>
// member occupies +8 (sizeof(std::vector<Itemchest>) is 0x20). sizeof = 0x28.
struct Mapchest
{
    short x;
    short y;
    short key_id;
    char updated;
    std::vector<Itemchest> slots;

    Mapchest(short x, short y, short key_id);
    ~Mapchest();
};

#endif
