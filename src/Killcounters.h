#ifndef KillcountersH
#define KillcountersH

#include <Classes.hpp>
#include <vector>
#include "Killcounter.h"

// Recovered from the reference (Killcounters unit, 0x53e4b4..0x53f5e4).
// Object layout pinned by the constructor (0x53e4e8) and IncrementAndGet
// (0x53e784):
//   +0x00 TStringList *                             field_0 = new TStringList
//   +0x04 String                                    name
//   +0x08 std::vector<KillCounter> buckets[27]      inline fixed array
// The ctor passes &buckets as a *pre-allocated* destination to the 7-argument
// _vector_new_ldtc_(dest, 0x20, 27, flags, elem_ctor, flags, typedesc), i.e. the
// array is constructed in place, and the dtor uses _vector_delete_ldtc_ on it.
// IncrementAndGet pushes into bucket (name[1] - 'a') clamped to 26.
struct Killcounters
{
    TStringList *field_0;
    String name;
    std::vector<KillCounter> buckets[27];

    Killcounters();
    ~Killcounters();

    static void Init(Killcounters *self);
    static int IncrementAndGet(Killcounters *self, String name);
};

#endif
