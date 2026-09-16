#include <System.hpp>
#include <SysUtils.hpp>
#pragma hdrstop

int string_ops(String a, String b, int i, char c)
{
    String s;
    s = "literal";
    s = a;
    s += b;
    String t = a + b;
    int n = t.Length();
    char ch = t[i];
    t[i] = c;
    String sub = t.SubString(i, 3);
    String lc = t.LowerCase();
    String tr = t.Trim();
    bool eq = (t == s);
    s = c;
    s = "";
    t.Delete(i, 2);
    t.Insert(a, i);
    int p = t.Pos(a);
    t.SetLength(5);
    String num = IntToStr(i);
    return n + p + eq + (int)ch + sub.Length() + lc.Length() + tr.Length() + num.Length();
}

int ansistring_ops(AnsiString a, AnsiString b, int i, char c)
{
    AnsiString s;
    s = "literal";
    s = a;
    s += b;
    AnsiString t = a + b;
    int n = t.Length();
    char ch = t[i];
    t[i] = c;
    AnsiString sub = t.SubString(i, 3);
    AnsiString lc = t.LowerCase();
    AnsiString tr = t.Trim();
    bool eq = (t == s);
    s = c;
    s = "";
    t.Delete(i, 2);
    t.Insert(a, i);
    int p = t.Pos(a);
    t.SetLength(5);
    AnsiString num = IntToStr(i);
    return n + p + eq + (int)ch + sub.Length() + lc.Length() + tr.Length() + num.Length();
}

int string_ints(String a, int i, char c)
{
    String fromint = i;
    String fromchar = c;
    a += i;
    a = a + i;
    a += c;
    a = a + c;
    int back = a.ToInt();
    String hex = IntToHex(i, 4);
    return back + fromint.Length() + fromchar.Length() + hex.Length();
}
