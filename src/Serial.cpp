#include <vcl.h>
#pragma hdrstop

#include "Serial.h"

#pragma package(smart_init)

#define SERIAL_ENC_STR_CONFIG_SERIAL_INI "d1dqa>d-h,B8d9_0jpC"
#define SERIAL_ENC_STR_NAME              "h2l1"
#define SERIAL_ENC_STR_MKEY              "T:b2"
#define SERIAL_ENC_STR_SKEY              "T:b,"
#define SERIAL_ENC_STR_PUB_DVF001_EVF    "g)hq@oA9W;B=X/Bq"
#define SERIAL_ENC_STR_NOT_LICENCED      "i:j1h<d3 +^1"

Serial::Serial()
{
    counter = 10;
    ini_file = new TStringList;

    String enc1 = SERIAL_ENC_STR_CONFIG_SERIAL_INI;
    String dec1 = DecodeString(this, enc1);
    SetIniPath(this, dec1);

    String enc2 = SERIAL_ENC_STR_NAME;
    key_base = ReadKey(this, DecodeString(this, enc2), key_base);

    String enc3 = SERIAL_ENC_STR_MKEY;
    unlock_code = ReadKey(this, DecodeString(this, enc3), unlock_code);

    String enc4 = SERIAL_ENC_STR_SKEY;
    serial_code = ReadKey(this, DecodeString(this, enc4), serial_code);

    ReloadIni(this);
    Validate(this);

    String enc5 = SERIAL_ENC_STR_PUB_DVF001_EVF;
    String dec5 = DecodeString(this, enc5);
    SetIniPath(this, dec5);

    if (ini_file->Text.Length() > 11)
        reg_name += ini_file->Text.SubString(1, 12);

    ReloadIni(this);
}

int Serial::GetCounter(Serial *s)
{
    return s->counter;
}

void Serial::SetCounter(Serial *s, int value)
{
    s->counter = value;
}

bool Serial::IsValid(Serial *s)
{
    return s->valid;
}

String Serial::GetKeyBaseCopy(Serial *s)
{
    return s->key_base_copy;
}

String Serial::GetUnlockCode(Serial *s)
{
    return s->unlock_code;
}

String Serial::GetRegName(Serial *s)
{
    return s->reg_name;
}

void Serial::SetIniPath(Serial *s, String path)
{
    s->ini_file->Clear();

    try
    {
    String exe = Application->ExeName;
    int last_slash_pos = 0;
    if (exe.Length() >= 1)
    {
        for (int i = exe.Length(); i >= 1; i--)
        {
            if (exe[i] == '\\')
            {
                last_slash_pos = i;
                break;
            }
        }
    }

    String dir = "";
    if (exe.Length() >= 1 && last_slash_pos >= 1)
    {
        for (int p = 1; p < last_slash_pos; p++)
            dir = dir + exe[p];
    }

    exe = dir;

    String tail;
    if (exe[exe.Length()] != '\\')
        tail = "\\" + path;
    else
        tail = path;

    s->ini_file->LoadFromFile(exe + tail);
    }
    catch (...)
    {
    }
}

void Serial::ReloadIni(Serial *s)
{
    s->ini_file->Clear();
}

String Serial::ReadKey(Serial *s, String key, String def)
{
    String result = def;
    if (s->ini_file->Count >= 1)
    {
    try
    {
        for (int i = 0; i < s->ini_file->Count; i++)
        {
            String line = s->ini_file->Strings[i];
            bool before_eq = true;
            bool after_eq = true;
            String ini_name = "";
            String value = "";
            if (line.Length() >= 1)
            {
                for (int j = 1; j <= line.Length(); j++)
                {
                    if (line[j] == '=')
                        before_eq = false;
                    if (before_eq)
                    {
                        if (line[j] != ' ' && line[j] != '=')
                            ini_name = ini_name + line[j];
                    }
                    else if (after_eq)
                    {
                        if (line[j] != ' ' && line[j] != '=')
                        {
                            after_eq = false;
                            value = value + line[j];
                        }
                    }
                    else
                    {
                        value = value + line[j];
                    }
                }
                if (AnsiLowerCase(ini_name) == AnsiLowerCase(key))
                {
                    if (value.Length() >= 1)
                        result = value;
                    break;
                }
            }
        }
    }
    catch (...)
    {
    }
    }
    return result;
}

String Serial::DecodeString(Serial *s, String src)
{
    String rev = "";
    String result = "";
    int parity = (src.Length() + 1) % 2;
    for (int i = src.Length(); i >= 1; i--)
        rev = rev + src[i];
    for (int i = 1; i <= src.Length(); i++)
    {
        char ch = rev[i];
        unsigned char c = ch;
        int u = c;
        if (i % 2 == parity)
        {
            if (0x22 <= u && u <= 0x7d)
            {
                u = 0x7d - u + 0x22;
                c = u;
                ch = c;
                result = result + String(ch);
            }
            else
                result = result + String(rev[i]);
        }
        else
        {
            bool done = false;
            if (0x22 <= u && u <= 0x4f)
            {
                done = true;
                u = 0x4f - u + 0x22;
                c = u;
                ch = c;
                result = result + String(ch);
            }
            if (0x50 <= u && u <= 0x7d)
            {
                done = true;
                u = 0x7d - u + 0x50;
                c = u;
                ch = c;
                result = result + String(ch);
            }
            if (!done)
                result = result + String(rev[i]);
        }
    }
    return result;
}

void Serial::Validate(Serial *s)
{
    bool stage1_passed = false;
    s->valid = 0;
    int acc5 = 400;
    int acc4 = 0x19a;
    int acc3 = 0x19d;
    for (int i = 1; i <= s->key_base.Length(); i++)
    {
        acc5 += (unsigned char)s->key_base[i] % 0x11;
        acc4 += (unsigned char)s->key_base[i] / 0x16;
        acc3 += (unsigned char)s->key_base[i] % 0x16;
    }
    String c1 = IntToHex(acc5 * 2 % 100 << 5, 3);
    String c2 = IntToHex(acc4 * 0xb % 0x58 * 0x25, 3);
    String c3 = IntToHex(acc3 * 3 % 0x70 * 0x1f, 3);
    if (s->serial_code.Length() >= 0xf)
    {
        String p1 = s->serial_code.SubString(5, 3);
        String p2 = s->serial_code.SubString(9, 3);
        String p3 = s->serial_code.SubString(0xd, 3);
        if (c1 == p1 && c2 == p2 && c3 == p3)
            stage1_passed = true;
    }
    if (stage1_passed)
    {
        DWORD csize = 0x10;
        DWORD volserial;
        DWORD maxcomp;
        DWORD flags;
        char computer_name[16];
        char volume_name[1000];
        char filesystem_name[1000];

        computer_name[0] = '\0';
        GetComputerNameA(computer_name, &csize);
        String cn = computer_name;

        (void)filesystem_name;
        GetVolumeInformationA("c:\\", volume_name, 1000, &volserial, &maxcomp, &flags, filesystem_name, 1000);
        String vs = volserial;
        if (vs.Length() >= 0x21)
            vs += vs.SubString(1, 0x20);
        unsigned int char_sum = 0;
        for (int i = 1; i <= vs.Length(); i++)
            char_sum += (unsigned char)vs[i];
        for (int i = 1; i <= s->serial_code.Length(); i++)
            char_sum += (unsigned char)s->serial_code[i];
        char_sum = vs.Length() * char_sum;
        char_sum = char_sum * 0x87;
        s->key_base_copy += s->key_base;
        s->key_base += IntToStr((int)char_sum);

        acc5 = 0x199;
        acc4 = 0x19b;
        acc3 = 0x19c;
        for (int i = 1; i <= s->key_base.Length(); i++)
        {
            acc5 += (unsigned char)s->key_base[i] % 0x16;
            acc4 += (unsigned char)s->key_base[i] / 0x11;
            acc3 += (unsigned char)s->key_base[i] % 0x12;
        }
        c1 += IntToHex(acc5 * 2 % 100 << 5, 3);
        c2 += IntToHex(acc4 * 0xb % 0x58 * 0x25, 3);
        c3 += IntToHex(acc3 * 3 % 0x70 * 0x1f, 3);
        if (s->serial_code.Length() >= 0xf)
        {
            String u1 = s->unlock_code.SubString(5, 3);
            String u2 = s->unlock_code.SubString(9, 3);
            String u3 = s->unlock_code.SubString(0xd, 3);
            if (c1 == u1 && c2 == u2 && c3 == u3)
                s->valid = 1;
        }
    }
}

String Serial::GetDisplayCode(Serial *s)
{
    String result = DecodeString(s, SERIAL_ENC_STR_NOT_LICENCED);
    if (s->serial_code.Length() >= 1 && s->valid != 0)
        result = s->serial_code;
    return result;
}
