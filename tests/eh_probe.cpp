// Probe: which source form produces an EH-framed, empty-bodied function that
// returns its pointer argument? The reference's Jukeboxcontrol_InitStub
// (0x4a9c3c) is exactly that: EH prologue, fs:[0] teardown, mov eax,[ebp+8], ret.
#include <vcl.h>
#pragma hdrstop

struct Pod
{
    int a;
};

struct EmptyCtor
{
    Pod pod;
    EmptyCtor();
};

struct StringMember
{
    AnsiString s;
    StringMember();
};

struct Base
{
    Base();
};

struct Derived : Base
{
    Derived();
};

// A: plain free function returning its argument
Pod *A(Pod *p)
{
    return p;
}

// B: empty user ctor, POD-only members
EmptyCtor::EmptyCtor()
{
}

// C: empty user ctor, class member with a ctor
StringMember::StringMember()
{
}

// D: empty user ctor, base with a ctor
Base::Base()
{
}

// E: empty user ctor of a derived class
Derived::Derived()
{
}
