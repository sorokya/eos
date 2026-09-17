// Probe: which ctor body reproduces Jukeboxcontrol::Jukeboxcontrol
// (reference 0x4a9b54: frame 0x2c, exactly one EH scope, call sequence
// InitStub -> InitRecentPlays -> operator new(8) -> ClearRecentPlays)?
#include <vcl.h>
#pragma hdrstop

#include <vector>
#include "../src/Jukebox.h"

void InitRecentPlays(std::vector<JukeBox> &v, const void *seed);
void ClearRecentPlays(std::vector<JukeBox> &v);

struct P1
{
    void *field_0;
    std::vector<JukeBox> recent_plays;
    P1();
};

struct P2
{
    void *field_0;
    std::vector<JukeBox> recent_plays;
    P2();
};

struct P3
{
    void *field_0;
    std::vector<JukeBox> recent_plays;
    P3();
};

void InitRecentPlays(std::vector<JukeBox> &v, const void *seed)
{
    v.clear();
    (void)seed;
}

void ClearRecentPlays(std::vector<JukeBox> &v)
{
    v.erase(v.begin(), v.end());
}

// P1: only the allocation
P1::P1()
{
    field_0 = operator new(8);
}

// P2: allocation + clear
P2::P2()
{
    field_0 = operator new(8);
    recent_plays.clear();
}

// P3: helper call + allocation + clear
P3::P3()
{
    char seed[0x18];
    InitRecentPlays(recent_plays, seed);
    field_0 = operator new(8);
    ClearRecentPlays(recent_plays);
}
