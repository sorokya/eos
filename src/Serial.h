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
    ~Serial();

    static int GetCounter(Serial *self);
    static void SetCounter(Serial *self, int value);
    static bool IsValid(Serial *self);
    static String GetKeyBaseCopy(Serial *self);
    static String GetUnlockCode(Serial *self);
    static String GetRegName(Serial *self);
    static String GetDisplayCode(Serial *self);
    static void SetIniPath(Serial *self, String path);
    static void ReloadIni(Serial *self);
    static String ReadKey(Serial *self, String key, String default_value);
    static String DecodeString(Serial *self, String src);
    static void Validate(Serial *self);
};

#endif
