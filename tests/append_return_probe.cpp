// Probe: the "arm/save/destroy/restore" EH fingerprint for a returned append.
//
// Three Packets functions end by appending a local `String` to an output and
// returning it. The reference emits an EH-clause sequence our current sources do
// not:
//
//   arm S                       ; enter the result's clause scope
//   <append>(dst, names)        ; e.g. $badd / $brplu
//   mov eax,[dst]               ; materialise the value being returned
//   arm S+0xc                   ; enter the temporary's nested clause
//   push eax                    ; save it across the destructor
//   <destroy names>
//   pop eax                     ; restore
//   arm S                       ; back to the result's clause
//   inc [scope]
//
// `A`/`B`/`C` return an `AnsiString` BY VALUE (the returned string is the append
// destination or a result built with `+=`); they reproduce the whole sequence.
// `D`/`E` (void / pointer return) emit no arms, and `F` shows the wrong guess
// (`try`/`catch`): it gives the save/restore but no arms and adds a flags-3
// cleanup-table entry the reference does not have.
//
// The numeric scope ids are byte offsets into the function's cleanup table, so
// they differ by function; the fingerprint is the sequence, not the value.
//
// Compile with the reference flags:
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/append_return_probe.asm tests/append_return_probe.cpp
#include <vcl.h>
#pragma hdrstop

#pragma warn - 8057

AnsiString enc(int v, int w)
{
    AnsiString r = "";
    return r;
}

// A: return `*out += names` by value. The append destination is the hidden
//    return slot, so the result pointer is saved across `names`'s destructor.
String A(String *out, int v)
{
    String names = enc(v, 2);
    return *out += names;
}

// B: build the result by `+=` and return it by value.
String B(const String &prefix, int v)
{
    String names = enc(v, 2);
    String result = prefix;
    result += names;
    return result;
}

// C: the result is a local initialised from a parameter, appended, returned.
String C(String out, int v)
{
    String names = enc(v, 2);
    out += names;
    return out;
}

// D: same statements, but the function returns void -> no arms.
void D(String *out, int v)
{
    String names = enc(v, 2);
    *out += names;
}

// E: same statements, but the function returns a pointer -> no arms.
String *E(String *out, int v)
{
    String names = enc(v, 2);
    *out += names;
    return out;
}

// F: the try/catch guess -> save/restore but no arms, plus a flags-3 ECT entry.
String *F(String *out, int v)
{
    String names = enc(v, 2);
    try
    {
        *out += names;
    }
    catch (...)
    {
    }
    return out;
}

#pragma warn .8057
