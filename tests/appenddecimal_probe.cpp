// Probe: which source form lowers a decimal value append to the RTL helper
// AnsiString_AppendDecimal (0x51ef68) inline (empty ctor + AppendDecimal)
// rather than to a real IntToStr (0x403010) call.
//
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/appenddecimal_probe.asm tests/appenddecimal_probe.cpp
#include <vcl.h>
#pragma hdrstop

#pragma warn - 8057

// A: IntToStr both sides (current Packets source).
String A(int character_id, int account)
{
    return "DELETE FROM endl_characters WHERE ident = " + IntToStr(character_id) +
           " AND ident_account = " + IntToStr(account);
}

// B: explicit String(...) on the first value.
String B(int character_id, int account)
{
    return "DELETE FROM endl_characters WHERE ident = " + String(character_id) +
           " AND ident_account = " + IntToStr(account);
}

// C: build via += on named accumulator.
String C(int character_id, int account)
{
    String s = "DELETE FROM endl_characters WHERE ident = ";
    s += character_id;
    s += " AND ident_account = ";
    s += IntToStr(account);
    return s;
}

// D: AnsiString local assigned from a plain int.
String D(int character_id, int account)
{
    return "DELETE FROM endl_characters WHERE ident = " +
           AnsiString(character_id) + " AND ident_account = " +
           IntToStr(account);
}

// E: append decimal directly via the int ctor as an operand to +=.
String E(int character_id, int account)
{
    String s = "DELETE FROM endl_characters WHERE ident = ";
    s += AnsiString(character_id);
    s += " AND ident_account = ";
    s += IntToStr(account);
    return s;
}
