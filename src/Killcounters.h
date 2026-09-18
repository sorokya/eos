#ifndef KillcountersH
#define KillcountersH

#include <Classes.hpp>
#include <vector>
#include "Killcounter.h"

// Recovered from the reference (KillCounters unit, 0x53e4b4..0x53f5e4).
// Object layout pinned by the constructor (0x53e4e8) and IncrementAndGet
// (0x53e784), and by the 0x368-byte allocation at the call site (0x416864):
//   +0x00 TStringList *                             string_list = new TStringList
//   +0x04 String                                    name
//   +0x08 std::vector<KillCounter> buckets[27]      inline fixed array
// The ctor passes &buckets as a *pre-allocated* destination to the 7-argument
// _vector_new_ldtc_(dest, 0x20, 27, flags, elem_ctor, flags, typedesc), i.e. the
// array is constructed in place, and the dtor uses _vector_delete_ldtc_ on it.
// IncrementAndGet/Get/Add push into bucket (name[1] - 'a') clamped to 26.
struct KillCounters
{
    TStringList *string_list;
    String name;
    std::vector<KillCounter> buckets[27];

    KillCounters();
    ~KillCounters();

    static void Init(KillCounters *self);
    static int IncrementAndGet(KillCounters *self, String name);
    static String Extract(KillCounters *self);
    static void Add(KillCounters *self, String name, int count);
    static int Get(KillCounters *self, String name);
    static void Clear(KillCounters *self);
    static void Save(KillCounters *self);
};

#endif
