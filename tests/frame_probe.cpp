// Probe: do per-section nested blocks let bcc reuse AnsiString temp slots?
//
// The Mapcontrol loader parses five length-prefixed sections, each a
// `SubString` temp fed to a decode call. If the reference wraps each section in
// its own `{ }`, bcc allocates the temps in disjoint scopes and reuses their
// stack slots, shrinking the frame. The flat form keeps every temp live to the
// end and grows the frame.
//
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/frame_probe.asm tests/frame_probe.cpp
#include <vcl.h>
#pragma hdrstop

#pragma warn - 8057

struct M
{
    int w, h, rid, a, b, c;
};

int dec(int v)
{
    return v;
}

int flat(int id)
{
    AnsiString buf;
    try
    {
        AnsiString t = AnsiString(id);
        buf = t;
        M *m = new M;
        AnsiString s1 = buf.SubString(1, 1);
        int v1 = dec(s1.ToInt());
        AnsiString s2 = buf.SubString(2, 1);
        int v2 = dec(s2.ToInt());
        AnsiString s3 = buf.SubString(3, 1);
        int v3 = dec(s3.ToInt());
        AnsiString s4 = buf.SubString(4, 1);
        int v4 = dec(s4.ToInt());
        AnsiString s5 = buf.SubString(5, 1);
        int v5 = dec(s5.ToInt());
        m->w = v1 + v2 + v3 + v4 + v5;
        delete m;
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

int nested(int id)
{
    AnsiString buf;
    try
    {
        AnsiString t = AnsiString(id);
        buf = t;
        M *m = new M;
        {
            AnsiString s = buf.SubString(1, 1);
            m->w = dec(s.ToInt());
        }
        {
            AnsiString s = buf.SubString(2, 1);
            m->h = dec(s.ToInt());
        }
        {
            AnsiString s = buf.SubString(3, 1);
            m->rid = dec(s.ToInt());
        }
        {
            AnsiString s = buf.SubString(4, 1);
            m->a = dec(s.ToInt());
        }
        {
            AnsiString s = buf.SubString(5, 1);
            m->b = dec(s.ToInt());
        }
        delete m;
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

#pragma warn .8057
