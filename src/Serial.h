#ifndef SerialH
#define SerialH

#include <Classes.hpp>

// Layout recovered from the reference (offsets in bytes):
//   0  TStringList *ini_file
//   4  AnsiString   key_base
//   8  AnsiString   key_base_copy
//  12  AnsiString   serial_code
//  16  AnsiString   unlock_code
//  20  AnsiString   reg_name
//  24  bool         valid
//  28  int          counter
struct Serial
{
    TStringList *ini_file;
    String key_base;
    String key_base_copy;
    String serial_code;
    String unlock_code;
    String reg_name;
    bool valid;
    int counter;

    Serial();

    static int GetCounter(Serial *s);
    static void SetCounter(Serial *s, int value);
    static bool IsValid(Serial *s);
    static String GetKeyBaseCopy(Serial *s);
    static String GetUnlockCode(Serial *s);
    static String GetRegName(Serial *s);
    static String GetDisplayCode(Serial *s);
    static void SetIniPath(Serial *s, String path);
    static void ReloadIni(Serial *s);
    static String ReadKey(Serial *s, String key, String def);
    static String DecodeString(Serial *s, String src);
    static void Validate(Serial *s);
};

#endif
