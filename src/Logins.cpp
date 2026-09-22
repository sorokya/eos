#include <vcl.h>
#pragma hdrstop

#include "Logins.h"

#pragma package(smart_init)

Logins::Logins(mySQLdb *mysql)
{
    mysql_controls = mysql;
    login_list = new TList;
    reserved_names = new TList;
    AddReservedName(this, "vult-r");
    AddReservedName(this, "aengie");
    AddReservedName(this, "angel");
}

Logins::~Logins()
{
    delete login_list;
    delete reserved_names;
}

void Logins::AddReservedName(Logins *self, String name)
{
    Asocketvip *entry = new Asocketvip;
    entry->name = name;
    entry->value = "new";
    self->reserved_names->Add(entry);
}

void Logins::AddLogin(Logins *self, String address)
{
    Asocketblock *entry = new Asocketblock;
    entry->address = address;
    entry->count = 0x1e;
    self->login_list->Add(entry);
}

void Logins::SetReservedName(Logins *self, String name, String ip)
{
    for (int i = 0; i < self->reserved_names->Count; i++)
    {
        Asocketvip *entry = (Asocketvip *)self->reserved_names->Items[i];
        if (entry->name == name)
        {
            entry->value = ip;
            break;
        }
    }
}

void Logins::Tick(Logins *self)
{
    for (int i = self->login_list->Count - 1; i >= 0; i--)
    {
        Asocketblock *entry = (Asocketblock *)self->login_list->Items[i];
        entry->count--;
        if (entry->count >= 1)
            continue;
        self->login_list->Delete(i);
        delete entry;
    }
}

bool Logins::HandleAddress(Logins *self, String address)
{
    if (address == "127.0.0.1")
        return true;

    bool is_new = true;

    for (int i = 0; i < self->login_list->Count; i++)
    {
        Asocketblock *entry = (Asocketblock *)self->login_list->Items[i];
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
            Asocketvip *entry = (Asocketvip *)self->reserved_names->Items[j];
            if (entry->value == address)
            {
                is_new = true;
                break;
            }
        }
    }

    if (is_new)
    {
        Asocketblock *entry = new Asocketblock;
        entry->address = address;
        entry->count = 0xc;
        self->login_list->Add(entry);
    }

    return is_new;
}

bool Logins::ConnectionLog_CheckIP(String ip)
{
    for (int i = 0; i < reserved_names->Count; i++)
    {
        Asocketvip *entry = (Asocketvip *)reserved_names->Items[i];
        if (entry->value == ip)
            return true;
    }
    return false;
}
