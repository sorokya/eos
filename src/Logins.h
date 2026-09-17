#ifndef LoginsH
#define LoginsH

#include <Classes.hpp>

class Mysqlcontrols;

class Logins
{
  public:
    Mysqlcontrols *mysql_controls; // +0x00
    TList *list_a;                 // +0x04
    TList *reserved_names;         // +0x08
    String field_c;                // +0x0c

    struct ReservedName
    {
        String name;  // +0x00
        String value; // +0x04
    };

    struct LoginEntry
    {
        String address; // +0x00
        int count;      // +0x04
    };

    Logins(Mysqlcontrols *mysql);
    ~Logins();

    static void Tick(Logins *self);
    static bool HandleAddress(Logins *self, String address);
    static void AddReservedName(Logins *self, String name);
    static void AddLogin(Logins *self, String address);
    static void SetReservedName(Logins *self, String name, String value);
    bool ConnectionLog_CheckIP(String ip);
};

#endif
