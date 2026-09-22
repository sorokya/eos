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
struct SerialKey
{
    TStringList *ini_file;
    String key_base;
    String key_base_copy;
    String serial_code;
    String unlock_code;
    String reg_name;
    bool valid;
    int counter;

    SerialKey();
    ~SerialKey();

    static int GetCounter(SerialKey *self);
    static void SetCounter(SerialKey *self, int value);
    static bool IsValid(SerialKey *self);
    static String GetKeyBaseCopy(SerialKey *self);
    static String GetUnlockCode(SerialKey *self);
    static String GetRegName(SerialKey *self);
    static String GetDisplayCode(SerialKey *self);
    static void SetIniPath(SerialKey *self, String path);
    static void ReloadIni(SerialKey *self);
    static String ReadKey(SerialKey *self, String key, String default_value);
    static String DecodeString(SerialKey *self, String src);
    static void Validate(SerialKey *self);
};

#endif
