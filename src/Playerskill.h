#ifndef PlayerskillH
#define PlayerskillH

#include <Classes.hpp>

// Recovered from the reference (Playerskill unit, 0x411f84..0x411fcd): the
// constructor stores its argument at offset 0 and the destructor is emitted
// out-of-line as the deleting form. Callers fill further fields (at least one at
// offset 4), so the full layout is wider than the single field the unit's own
// code touches. The class name is a placeholder (internal names are
// unobservable in the stripped image).
struct Playerskill
{
    int field_0;

    Playerskill(int value);
    ~Playerskill();
};

#endif
