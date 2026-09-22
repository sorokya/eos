#ifndef LoginsH
#define LoginsH

#include <Classes.hpp>
#include "Mysqlcontrols.h"

// The two record types the `TList`s hold. The names and the namespace scope are
// the reference's own: bcc32 writes the spelled type name into the RTTI
// descriptor it emits for each, and the Logins module carries `Asocketvip` /
// `Asocketvip *` and `Asocketblock` / `Asocketblock *` (no `Logins::` prefix).
// Reconstructing them as nested `Logins::ReservedName` / `Logins::LoginEntry`
// made the descriptors 32 bytes too long, which was the whole of that module's
// size difference. The field-count words in the descriptors (3 and 2) pair
// `Asocketvip` with the two-String record and `Asocketblock` with the
// String+int one.
struct Asocketvip
{
    String name;  // +0x00
    String value; // +0x04
};

struct Asocketblock
{
    String address; // +0x00
    int count;      // +0x04
};

class Logins
{
  public:
    mySQLdb *mysql_controls; // +0x00
    TList *login_list;       // +0x04
    TList *reserved_names;   // +0x08
    String field_0xc;        // +0x0c

    Logins(mySQLdb *mysql);
    ~Logins();

    static void Tick(Logins *self);
    static bool HandleAddress(Logins *self, String address);
    static void AddReservedName(Logins *self, String name);
    static void AddLogin(Logins *self, String address);
    static void SetReservedName(Logins *self, String name, String ip);
    bool ConnectionLog_CheckIP(String ip);
};

#endif
