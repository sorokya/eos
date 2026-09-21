#ifndef BannedH
#define BannedH

#include <Classes.hpp>
#include <SysUtils.hpp>
#include "Mysqlcontrols.h"


// Ban record stored in the Banned manager's list. RTTI type name is
// `Asocketban` (reference type table at 0x52ca98, sizeof 0x1c, deleting
// destructor 0x52cae8 which clears the String at +0). Layout recovered from the
// record construction in Banned::AddBan (0x52bb60) and the scan in
// Banned::IsBanned (0x52c45c).
class Asocketban
{
  public:
    String hdid;            // +0x00
    short octet1;           // +0x04
    short octet2;           // +0x06
    short octet3;           // +0x08
    short octet4;           // +0x0a
    unsigned char ban_type; // +0x0c
    char pad_0xd[3];        // +0x0d
    int duration;           // +0x10
    TTimeStamp ban_date;    // +0x14
};

// IP/serial ban list manager. RTTI type name `Banned` (reference type table at
// 0x473e74, sizeof 0x10, deleting destructor 0x52bb44). Layout from the
// constructor 0x52b8dc: db handle at +0, TList of Asocketban* at +4; +8/+0xc
// hold the last matched ban's remaining minutes and type, read back through
// GetBanTime/GetBanType.
class Banned
{
  public:
    Mysqlcontrols *db_handle; // +0x00
    TList *ban_list;          // +0x04
    int field_0x8;            // +0x08
    unsigned char field_0xc;  // +0x0c
    char pad_0xd[3];          // +0x0d

    Banned(Mysqlcontrols *db_handle);
    ~Banned();

    static void
    AddBan(Banned *self, String ip, String serial, bool permanent, int duration);
    static void AddBan(Banned *self, String ip, String hdid, char ban_type, int duration);
    static bool IsBanned(Banned *self, String ip, String hdid);
    static int GetBanType(Banned *self);
    static int GetBanTime(Banned *self);
};

#endif
