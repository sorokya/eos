#include <vcl.h>
#pragma hdrstop

#include "Banned.h"
#include "Mysqlcontrols.h"
#include "Protocol.h"

#pragma package(smart_init)

// Sentinel octet for a wildcarded (`*`) component of an IP pattern.
#define IP_OCTET_WILDCARD 0x100

Banned::Banned(mySQLdb *db_handle)
{
    ban_list = new TList;
    this->db_handle = db_handle;
    mySQLdb::Query(this->db_handle,
                   "SELECT bandate, permanent, ipaddress, serial_h FROM endl_banlist");
    if (mySQLdb::GetResultCount(this->db_handle) > 0)
    {
        while (!mySQLdb::ResultAtEnd(this->db_handle))
        {
            if ((unsigned)mySQLdb::Db_GetInt(this->db_handle, "permanent") > 0)
                AddBan(this,
                       mySQLdb::Db_GetString(this->db_handle, "ipaddress"),
                       mySQLdb::Db_GetString(this->db_handle, "serial_h"),
                       true,
                       0x2ee);
            else
                AddBan(this,
                       mySQLdb::Db_GetString(this->db_handle, "ipaddress"),
                       mySQLdb::Db_GetString(this->db_handle, "serial_h"),
                       false,
                       0x2ee);
            mySQLdb::NextResultRecord(this->db_handle);
        }
    }
}

Banned::~Banned()
{
}

void Banned::AddBan(
    Banned *self, String ip, String serial, bool permanent, int duration_secs)
{
    try
    {
        int p1 = ip.Pos(".");
        ip[p1] = 'x';
        int p2 = ip.Pos(".");
        ip[p2] = 'x';
        int p3 = ip.Pos(".");
        ip[p3] = 'x';
        short octet1 = IP_OCTET_WILDCARD;
        short octet2 = IP_OCTET_WILDCARD;
        short octet3 = IP_OCTET_WILDCARD;
        short octet4 = IP_OCTET_WILDCARD;
        if (ip.SubString(1, p1 - 1) != "*")
            octet1 = StrToInt(ip.SubString(1, p1 - 1));
        if (ip.SubString(p1 + 1, (p2 - 1) - p1) != "*")
            octet2 = StrToInt(ip.SubString(p1 + 1, (p2 - 1) - p1));
        if (ip.SubString(p2 + 1, (p3 - 1) - p2) != "*")
            octet3 = StrToInt(ip.SubString(p2 + 1, (p3 - 1) - p2));
        if (ip.SubString(p3 + 1, 3) != "*")
            octet4 = StrToInt(ip.SubString(p3 + 1, 3));
        Asocketban *entry;
        entry = new Asocketban;
        entry->hdid = serial;
        entry->octet1 = octet1;
        entry->octet2 = octet2;
        entry->octet3 = octet3;
        entry->octet4 = octet4;
        entry->ban_date = DateTimeToTimeStamp(Now());
        entry->ban_type = permanent;
        entry->duration_secs = duration_secs;
        self->ban_list->Add(entry);
    }
    catch (...)
    {
    }
}

void Banned::AddBan(
    Banned *self, String ip, String hdid, char ban_type, int duration_secs)
{
    try
    {
        int p1 = ip.Pos(".");
        ip[p1] = 'x';
        int p2 = ip.Pos(".");
        ip[p2] = 'x';
        int p3 = ip.Pos(".");
        ip[p3] = 'x';
        short octet1 = IP_OCTET_WILDCARD;
        short octet2 = IP_OCTET_WILDCARD;
        short octet3 = IP_OCTET_WILDCARD;
        short octet4 = IP_OCTET_WILDCARD;
        if (ip.SubString(1, p1 - 1) != "*")
            octet1 = StrToInt(ip.SubString(1, p1 - 1));
        if (ip.SubString(p1 + 1, (p2 - 1) - p1) != "*")
            octet2 = StrToInt(ip.SubString(p1 + 1, (p2 - 1) - p1));
        if (ip.SubString(p2 + 1, (p3 - 1) - p2) != "*")
            octet3 = StrToInt(ip.SubString(p2 + 1, (p3 - 1) - p2));
        Asocketban *entry;
        entry = new Asocketban;
        entry->hdid = hdid;
        entry->octet1 = octet1;
        entry->octet2 = octet2;
        entry->octet3 = octet3;
        entry->octet4 = octet4;
        entry->ban_date = DateTimeToTimeStamp(Now());
        entry->ban_type = ban_type;
        entry->duration_secs = duration_secs;
        self->ban_list->Add(entry);
    }
    catch (...)
    {
    }
}

bool Banned::IsBanned(Banned *self, String ip, String hdid)
{
    bool banned = false;
    try
    {
        int p1 = ip.Pos(".");
        ip[p1] = 'x';
        int p2 = ip.Pos(".");
        ip[p2] = 'x';
        int p3 = ip.Pos(".");
        ip[p3] = 'x';
        short octet1 = IP_OCTET_WILDCARD;
        short octet2 = IP_OCTET_WILDCARD;
        short octet3 = IP_OCTET_WILDCARD;
        short octet4 = IP_OCTET_WILDCARD;
        if (ip.SubString(1, p1 - 1) != "*")
            octet1 = StrToInt(ip.SubString(1, p1 - 1));
        if (ip.SubString(p1 + 1, (p2 - 1) - p1) != "*")
            octet2 = StrToInt(ip.SubString(p1 + 1, (p2 - 1) - p1));
        if (ip.SubString(p2 + 1, (p3 - 1) - p2) != "*")
            octet3 = StrToInt(ip.SubString(p2 + 1, (p3 - 1) - p2));
        if (ip.SubString(p3 + 1, 3) != "*")
            octet4 = StrToInt(ip.SubString(p3 + 1, 3));
        for (int i = self->ban_list->Count - 1; i >= 0; i--)
        {
            bool match = false;
            Asocketban *entry = (Asocketban *)self->ban_list->Items[i];
            TTimeStamp now = DateTimeToTimeStamp(Now());
            int days = now.Date - entry->ban_date.Date;
            int ms = now.Time - entry->ban_date.Time;
            int elapsed = ms / MS_PER_SECOND + days * SECONDS_PER_DAY;
            if (entry->octet1 == octet1 || entry->octet1 > 0xff)
            {
                if (hdid == entry->hdid)
                    match = true;
                if ((entry->octet2 == octet2 || entry->octet2 > 0xff) &&
                    (entry->octet3 == octet3 || entry->octet3 > 0xff) &&
                    (entry->octet4 == octet4 || entry->octet4 > 0xff))
                    match = true;
                if (match && entry->duration_secs > elapsed)
                {
                    self->minutes_remaining = (entry->duration_secs - elapsed) / 60 + 1;
                    self->ban_type = entry->ban_type;
                    banned = true;
                }
                if (entry->ban_type == 0 && entry->duration_secs <= elapsed)
                {
                    self->ban_list->Delete(i);
                    delete entry;
                }
            }
        }
    }
    catch (...)
    {
        banned = false;
    }
    return banned;
}

int Banned::GetBanType(Banned *self)
{
    return self->ban_type;
}

int Banned::GetBanTime(Banned *self)
{
    return self->minutes_remaining;
}
