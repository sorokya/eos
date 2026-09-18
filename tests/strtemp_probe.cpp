#include <vcl.h>
#pragma hdrstop

#include <SysUtils.hpp>

int bb();

void v1(AnsiString msg, int break_byte)
{
    msg.Insert("Too large uncoded packet dropped: " + IntToStr(break_byte),
               msg.Length() + 1);
}

void v2(AnsiString msg, int break_byte)
{
    msg.Insert(AnsiString("Too large uncoded packet dropped: ") + IntToStr(break_byte),
               msg.Length() + 1);
}

void v3(AnsiString msg, int break_byte)
{
    AnsiString s = "Too large uncoded packet dropped: ";
    s += IntToStr(break_byte);
    msg.Insert(s, msg.Length() + 1);
}

void v4(AnsiString msg, int break_byte)
{
    AnsiString s = "Too large uncoded packet dropped: " + IntToStr(break_byte);
    msg.Insert(s, msg.Length() + 1);
}
