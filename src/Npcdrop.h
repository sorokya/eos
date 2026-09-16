#ifndef NpcdropH
#define NpcdropH

#include <Classes.hpp>

// Recovered from the reference (Npcdrop unit, 0x4a9ae8..0x4a9b31): the
// constructor stores its argument at offset 0 and the destructor is emitted
// out-of-line as the deleting form. Callers place the object in a 16-byte
// stack record and fill the remaining fields themselves, so the full layout is
// wider than the single field the unit's own code touches. The class name is a
// placeholder (internal names are unobservable in the stripped image).
struct Npcdrop
{
    int field_0;

    Npcdrop(int value);
    ~Npcdrop();
};

#endif
