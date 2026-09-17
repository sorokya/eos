#ifndef ItemgroundH
#define ItemgroundH

#include <Classes.hpp>

// Recovered from the reference (Itemground unit, 0x488494..0x4884d8): the
// constructor is trivial (no member stores; just the EH frame and returning
// `this`) and the destructor is emitted out-of-line as the deleting form.
// The class name is ChestItem: the reference RTTI type-name table carries
// `std::vector<ChestItem,...>` for MapContainer's ground-item list, which is the
// only place the Itemground class is used. Layout beyond the size is not
// established here.
struct ChestItem
{
    ChestItem();
    ~ChestItem();
};

#endif
