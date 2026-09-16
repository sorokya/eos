#ifndef ItemvalueH
#define ItemvalueH

#include <Classes.hpp>

// Recovered from the reference (Itemvalue unit, 0x4781b8..0x478248). The
// constructor stores the item id at offset 0; the EH cleanup table (0x5766f0)
// records one stack pointer member at offset 8 with the "ItemValue *" type
// descriptor and DTCVF_PTRVAL, so the class holds a self-pointer there. The
// class name is the RTTI type name ("ItemValue"). The remaining EifRecord
// fields are pending the Itemvalues parser.
struct ItemValue
{
    int id;
    int field_4;
    ItemValue *field_8;

    ItemValue(int id);
    ~ItemValue();
};

#endif
