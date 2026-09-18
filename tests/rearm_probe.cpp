// Probe: which construct makes bcc re-arm the enclosing cleanup scope after a
// decode statement inside a loop inside a try?
//
// Target shape (FUN_00482834 inner loop): arm(0x20c) spec-temp; arm(0x224)
// code-temp; then a no-op re-arm 0x218 before the following if. We emit only
// the first two. Read the arming stream (`mov word ptr [ebp-N],V`) and the
// ECTXPB descriptor for each variant.
//
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/rearm_probe.asm tests/rearm_probe.cpp
#include <vcl.h>
#pragma hdrstop

#pragma warn - 8057

int dec(AnsiString s)
{
    return s.Length();
}

// A: decodes as assignments to function-scope locals, plain block body.
int A(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            String t2 = buf.SubString(1, 2);
            code = dec(t2);
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// B: the code decode alone wrapped in a nested block.
int B(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            {
                String t2 = buf.SubString(1, 2);
                code = dec(t2);
            }
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// C: if/else-if chain following.
int C(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            String t2 = buf.SubString(1, 2);
            code = dec(t2);
            if (code == 0)
                n++;
            else if (code == 1)
                n--;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// D: a discarded by-value call intervenes.
AnsiString side(AnsiString s)
{
    return s;
}

int D(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            String t2 = buf.SubString(1, 2);
            code = dec(t2);
            side("x");
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// E: continue as the loop's last statement.
int E(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            String t2 = buf.SubString(1, 2);
            code = dec(t2);
            if (code == 0)
            {
                n++;
                continue;
            }
            n--;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// F: the second decode declared in the for-init via a declaration without init.
int F(AnsiString buf, int n)
{
    int spec = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            int code;
            String t2 = buf.SubString(1, 2);
            code = dec(t2);
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}


// G: spec/code declared with block scope inside the loop (one statement each).
int G(AnsiString buf, int n)
{
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            int spec = dec(t1);
            String t2 = buf.SubString(1, 2);
            int code = dec(t2);
            if (code == 0)
                n++;
            if (spec == 0)
                n--;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// H: one reused named temp for both decodes.
int H(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t = buf.SubString(1, 1);
            spec = dec(t);
            t = buf.SubString(1, 2);
            code = dec(t);
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// I: body's last statement is a String method call (like Delete).
int I(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String t1 = buf.SubString(1, 1);
            spec = dec(t1);
            String t2 = buf.SubString(1, 2);
            code = dec(t2);
            if (code == 0)
                n++;
            buf.Delete(1, 2);
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}


// J: two consecutive statements that each BIND a String temporary, then scalar.
int J(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            String spec_str = buf.SubString(1, 1);
            spec = dec(spec_str);
            String code_str = buf.SubString(1, 2);
            code = dec(code_str);
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// K: the decode called with an inline SubString directly (anonymous temp).
int K(AnsiString buf, int n)
{
    int spec = 0;
    int code = 0;
    try
    {
        for (int k = 0; k < n; k++)
        {
            spec = dec(buf.SubString(1, 1));
            code = dec(buf.SubString(1, 2));
            if (code == 0)
                n++;
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

#pragma warn .8057
