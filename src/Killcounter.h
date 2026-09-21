#ifndef KillcounterH
#define KillcounterH

#include <Classes.hpp>

// Recovered from the reference (Killcounter unit, 0x53e268..0x53e39a). Two
// constructors, both taking the name by value and storing it at offset 0; the
// single-argument form sets the count at offset 4 to 1, the two-argument form
// stores the caller's count. RTTI type name: `vector<KillCounter, ...>`.
struct KillCounter
{
    String name;
    int count;

    KillCounter(String name);
    KillCounter(String name, int count);
    ~KillCounter();
};

#endif
