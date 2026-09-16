#ifndef PlayerinventoryH
#define PlayerinventoryH

#include <Classes.hpp>

// Recovered from the reference (Playerinventory unit, 0x411ff0..0x412039): the
// constructor stores its argument at offset 0 and the destructor is emitted
// out-of-line as the deleting form. Callers fill further fields, so the full
// layout is wider than the single field the unit's own code touches. The class
// name is a placeholder (internal names are unobservable in the stripped image).
struct Playerinventory
{
    int field_0;

    Playerinventory(int value);
    ~Playerinventory();
};

#endif
