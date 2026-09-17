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
//   +0x08 DynamicArray< std::vector<KillCounter> >   buckets, Length = 27
// The dynamic-array element is the std::vector itself (size 0x20), not a
// wrapper; 0x53e4b4 is the RTL's per-slot default-construction helper, which is
// why it has no `mov eax,[ebp+8]` return-this. IncrementAndGet maps name[1] to
// `c - 'a'` clamped to 26 and pushes into that bucket.
struct Killcounters
{
    TStringList *field_0;
    String name;
    DynamicArray< std::vector<KillCounter> > buckets;

    Killcounters();
    ~Killcounters();

    static int IncrementAndGet(Killcounters *self, String name);
};

#endif
