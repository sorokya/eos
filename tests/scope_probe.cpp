// Probe: the try/catch EH fingerprint.
//
// A `try { ... } catch (...) {}` around a body that raises no new value code
// still changes the EH tables: it arms an extra clause scope before the body
// and a catch scope at the handler, and adds a flags-3 cleanup-table entry.
// A has no try; B wraps the same statement in a try/catch. Comparing the two
// `-S` listings shows B's marker stream gains the try arming (0x20) before the
// body and the catch arming (0x28); its ECT gains a flags-3 entry.
//
// Compile with the reference flags:
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/probe.asm tests/scope_probe.cpp
#include <vcl.h>
#pragma hdrstop

#pragma warn - 8057

struct P
{
    int x;
};

AnsiString enc(int v, int w)
{
    AnsiString r = "";
    return r;
}

AnsiString *A(AnsiString *out, P *p)
{
    AnsiString s = "a";
    if (p->x > 0)
        s.Insert(enc(p->x, 1), s.Length() + 1);
    *out += s;
    return out;
}

AnsiString *B(AnsiString *out, P *p)
{
    AnsiString s = "a";
    try
    {
        if (p->x > 0)
            s.Insert(enc(p->x, 1), s.Length() + 1);
    }
    catch (...)
    {
    }
    *out += s;
    return out;
}

#pragma warn .8057
