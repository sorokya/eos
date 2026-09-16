#ifndef ItemgroundH
#define ItemgroundH

#include <Classes.hpp>

// Recovered from the reference (Itemground unit, 0x488494..0x4884d8): the
// constructor is trivial (no member stores; just the EH frame and returning
// `this`) and the destructor is emitted out-of-line as the deleting form.
// The full record size is set by callers and is not established here; the class
// name is a Ghidra hint (internal names are unobservable in the stripped image).
struct GroundItem
{
    GroundItem();
    ~GroundItem();
};

#endif
