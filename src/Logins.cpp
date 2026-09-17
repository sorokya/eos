#include <vcl.h>
#pragma hdrstop

#include "Logins.h"

#pragma package(smart_init)

Logins::Logins(Mysqlcontrols *mysql)
{
    mysql_controls = mysql;
    list_a = new TList;
    reserved_names = new TList;
    AddReservedName(this, "vult-r");
    AddReservedName(this, "aengie");
    AddReservedName(this, "angel");
}

Logins::~Logins()
{
    delete list_a;
    delete reserved_names;
}

void Logins::AddReservedName(Logins *self, String name)
{
    ReservedName *entry = new ReservedName;
    entry->name = name;
    entry->value = "new";
    self->reserved_names->Add(entry);
}

void Logins::AddLogin(Logins *self, String address)
{
    LoginEntry *entry = new LoginEntry;
    entry->address = address;
    entry->count = 0x1e;
    self->list_a->Add(entry);
}

void Logins::SetReservedName(Logins *self, String name, String value)
{
    for (int i = 0; i < self->reserved_names->Count; i++)
    {
        ReservedName *entry = (ReservedName *)self->reserved_names->Items[i];
        if (entry->name == name)
        {
            entry->value = value;
            break;
        }
    }
}

void Logins::Tick(Logins *self)
{
    for (int i = self->list_a->Count - 1; i >= 0; i--)
    {
        LoginEntry *entry = (LoginEntry *)self->list_a->Items[i];
        entry->count--;
        if (entry->count >= 1)
            continue;
        self->list_a->Delete(i);
        delete entry;
    }
}

bool Logins::HandleAddress(Logins *self, String address)
{
    if (address == "127.0.0.1")
        return true;

    bool is_new = true;

    for (int i = 0; i < self->list_a->Count; i++)
    {
        LoginEntry *entry = (LoginEntry *)self->list_a->Items[i];
        if (entry->address == address)
        {
            is_new = false;
            break;
        }
    }

    if (!is_new)
    {
        for (int j = 0; j < self->reserved_names->Count; j++)
        {
            ReservedName *entry = (ReservedName *)self->reserved_names->Items[j];
            if (entry->value == address)
            {
                is_new = true;
                break;
            }
        }
    }

    if (is_new)
    {
        LoginEntry *entry = new LoginEntry;
        entry->address = address;
        entry->count = 0xc;
        self->list_a->Add(entry);
    }

    return is_new;
}

bool Logins::ConnectionLog_CheckIP(String ip)
{
    for (int i = 0; i < reserved_names->Count; i++)
    {
        ReservedName *entry = (ReservedName *)reserved_names->Items[i];
        if (entry->value == ip)
            return true;
    }
    return false;
}
