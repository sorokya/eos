// Probe: which try/block structure produces the loader's EH arming stream?
//
// FUN_00482834 arms cleanup scopes (the `mov word ptr [ebp-N],imm` marker
// stream) as 8, 0x14, 0x20, 0x14, 0x2c, 0x38 ... — i.e. it re-arms the 0x14
// clause scope after `local_c` is constructed, before the "0"/maps/.emf temps.
// Find the source shape that reproduces it.
//
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/prologue_probe.asm tests/prologue_probe.cpp
#include <vcl.h>
#pragma hdrstop

#pragma warn - 8057

int A(int id)
{
    String map_buf;
    try
    {
        String local_c;
        String t_itoa = IntToStr(id);
        map_buf = t_itoa;
        String t_zero = "0";
        map_buf.Insert(t_zero, 0);
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

int B(int id)
{
    String map_buf;
    String local_c;
    try
    {
        String t_itoa = IntToStr(id);
        map_buf = t_itoa;
        String t_zero = "0";
        map_buf.Insert(t_zero, 0);
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

int C(int id)
{
    String map_buf;
    try
    {
        String local_c;
        {
            String t_itoa = IntToStr(id);
            map_buf = t_itoa;
            String t_zero = "0";
            map_buf.Insert(t_zero, 0);
        }
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

#pragma warn .8057
