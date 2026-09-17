#include <vcl.h>
#pragma hdrstop

#include "Mysqlcontrols.h"
#include "Mainform.h"

#pragma package(smart_init)

// Cross-unit helpers whose units are reconstructed separately.

// Connection parameters are stored obfuscated; DecodeString reverses them and
// maps digits/letters back (see Serial::DecodeString). Decoded:
//   username=endless_acc
//   password=xitz9ak4
#define MYSQL_ENC_STR_USERNAME "xxz_hhvowmv=vnzmivhf"
#define MYSQL_ENC_STR_PASSWORD "5pz0agrc=wildhhzk"

#define MS_PER_SECOND 1000
#define MS_PER_HALF_HOUR 1800000
#define SECONDS_PER_DAY 0x15180
#define HALF_HOURS_PER_DAY 0x30

#define TEXT_MIN_LENGTH 0x3c
#define TEXT_MAX_LENGTH 0x80
#define TEXT_SYMBOL_LIMIT 0x14
#define TEXT_CAPS_LIMIT 0x28

Mysqlcontrols::~Mysqlcontrols()
{
}

Mysqlcontrols::Mysqlcontrols()
{
    file_cache = new FileCache;
    thread_queue = new mySQLbuffer;
    worker_thread =
        new MySQLthread(GUI->mysession, GUI->mysql, GUI->myquery, thread_queue, false);
    query_error_count = 0;
    exec_error_count = 0;
    connected_time = DateTimeToTimeStamp(Now());
    last_query_time = DateTimeToTimeStamp(Now());
}

void Mysqlcontrols::Free(Mysqlcontrols *self, unsigned char free_flags)
{
    if (self != 0 && (free_flags & 1))
        ::operator delete(self);
}

bool Mysqlcontrols::TestConnection(Mysqlcontrols *self)
{
    try
    {
        GUI->mysql->Params->Clear();
        GUI->mysql->Params->Add(DecodeString(self, MYSQL_ENC_STR_USERNAME));
        GUI->mysql->Params->Add(DecodeString(self, MYSQL_ENC_STR_PASSWORD));
        GUI->mysql->Connected = true;
    }
    catch (...)
    {
    }
    return GUI->mysql->Connected;
}

void Mysqlcontrols::Connect(Mysqlcontrols *self,
                            int version_major,
                            int version_minor,
                            int version_patch)
{
    FileCache::CheckCacheFile(self->file_cache);
    if (self->file_cache->dirty == false)
        ExecDrop(self, "UPDATE endl_characters SET online = 0 WHERE online = 1");
    ExecDrop(self, "DELETE FROM endl_server");

    String connect_string = "INSERT INTO endl_server (version, characters, connections, "
                            "players, uptime, upload, download) VALUES (";
    connect_string = connect_string + "'" + IntToStr(version_major) + "." +
                     IntToStr(version_minor) + "." + IntToStr(version_patch) +
                     "',0,0,0,0,'no data','no data')";
    ExecDrop(self, connect_string);

    Query(self, "SHOW TABLE STATUS FROM endless_db");
    while (GUI->myquery->Eof == false)
    {
        try
        {
            if (GUI->myquery->Fields->Fields[0]->AsString == "endl_accounts")
            {
                if (AnsiLowerCase(GUI->myquery->Fields->Fields[3]->AsString) == "fixed")
                    self->file_cache->accounts_count =
                        GUI->myquery->Fields->Fields[4]->AsInteger;
            }
        }
        catch (...)
        {
        }
        try
        {
            if (GUI->myquery->Fields->Fields[0]->AsString == "endl_characters")
            {
                if (AnsiLowerCase(GUI->myquery->Fields->Fields[3]->AsString) == "fixed")
                    self->file_cache->characters_count =
                        GUI->myquery->Fields->Fields[4]->AsInteger;
            }
        }
        catch (...)
        {
        }
        try
        {
            if (GUI->myquery->Fields->Fields[0]->AsString == "endl_guilds")
            {
                if (AnsiLowerCase(GUI->myquery->Fields->Fields[3]->AsString) == "fixed")
                    self->file_cache->guilds_count =
                        GUI->myquery->Fields->Fields[4]->AsInteger;
            }
        }
        catch (...)
        {
        }
        GUI->myquery->Next();
    }

    if (self->file_cache->dirty == false)
    {
        Query(self,
              "SELECT privilege, name, title, level, experience, gender FROM "
              "endl_characters "
              "WHERE privilege = 0 ORDER BY experience desc LIMIT 100");
        LoadCachedPlayers(self);
    }
    else
    {
        FileCache::LoadPlayerCache(self->file_cache);
    }

    if (self->file_cache->dirty == false)
    {
        Query(self,
              "SELECT ident_guild, guild, SUM(level) as exptotal, MAX(level) as exphigh, "
              "COUNT(ident_guild) as members FROM endl_characters WHERE "
              "LENGTH(ident_guild) > 1 "
              "AND privilege = 0 GROUP BY ident_guild desc ORDER BY exptotal desc LIMIT "
              "100");
        LoadCachedGuilds(self);
    }
    else
    {
        FileCache::LoadGuildCache(self->file_cache);
    }
}

void Mysqlcontrols::LoadCachedPlayers(Mysqlcontrols *self)
{
    while (GUI->myquery->Eof == false)
    {
        FilecacheEntry *entry = new FilecacheEntry;
        entry->privilege = GUI->myquery->Fields->FieldByName("privilege")->AsInteger;
        entry->name = GUI->myquery->Fields->FieldByName("name")->AsString;
        entry->title = GUI->myquery->Fields->FieldByName("title")->AsString;
        entry->level = GUI->myquery->Fields->FieldByName("level")->AsInteger;
        entry->experience = GUI->myquery->Fields->FieldByName("experience")->AsInteger;
        entry->gender = GUI->myquery->Fields->FieldByName("gender")->AsInteger;
        self->file_cache->pending_player_writes.insert(
            self->file_cache->pending_player_writes.end(), entry);
        GUI->myquery->Next();
    }
}

void Mysqlcontrols::LoadCachedGuilds(Mysqlcontrols *self)
{
    while (GUI->myquery->Eof == false)
    {
        FilecacheEntryB *entry = new FilecacheEntryB;
        entry->ident_guild = GUI->myquery->Fields->FieldByName("ident_guild")->AsString;
        entry->guild = GUI->myquery->Fields->FieldByName("guild")->AsString;
        entry->exptotal = GUI->myquery->Fields->FieldByName("exptotal")->AsInteger;
        entry->exphigh = GUI->myquery->Fields->FieldByName("exphigh")->AsInteger;
        entry->members = GUI->myquery->Fields->FieldByName("members")->AsInteger;
        self->file_cache->pending_guild_writes.insert(
            self->file_cache->pending_guild_writes.end(), entry);
        GUI->myquery->Next();
    }
}

void Mysqlcontrols::UpdateServerStatus(Mysqlcontrols *self,
                                       int refresh_seconds,
                                       int connections,
                                       int players,
                                       int most,
                                       String upload,
                                       String download)
{
    TTimeStamp now_stamp = DateTimeToTimeStamp(Now());
    int date_diff = now_stamp.Date - self->last_query_time.Date;
    int time_diff = now_stamp.Time - self->last_query_time.Time;
    int elapsed = time_diff / MS_PER_SECOND + date_diff * SECONDS_PER_DAY;
    if (elapsed >= refresh_seconds)
    {
        self->last_query_time = DateTimeToTimeStamp(Now());
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int up_date_diff = stamp.Date - self->connected_time.Date;
        int up_time_diff = stamp.Time - self->connected_time.Time;
        int uptime = up_time_diff / MS_PER_HALF_HOUR + up_date_diff * HALF_HOURS_PER_DAY;
        String s = "UPDATE endl_server SET ";
        s = s + "uptime = " + IntToStr(uptime) + ",";
        s = s + "connections = " + IntToStr(connections) + ",";
        s = s + "players = " + IntToStr(players) + ",";
        s = s + "most = " + IntToStr(most) + ",";
        s = s + "upload = '" + upload + "',";
        s = s + "download = '" + download + "'";
        Mysql_ExecDirect(self, 0, s);
    }
}

String Mysqlcontrols::Db_GetString(Mysqlcontrols *self, String label)
{
    return GUI->myquery->Fields->FieldByName(label)->AsString;
}

int Mysqlcontrols::Db_GetInt(Mysqlcontrols *self, String label)
{
    return GUI->myquery->Fields->FieldByName(label)->AsInteger;
}

void Mysqlcontrols::NextResultRecord(Mysqlcontrols *self)
{
    GUI->myquery->Next();
}

bool Mysqlcontrols::ResultAtEnd(Mysqlcontrols *self)
{
    return GUI->myquery->Eof;
}

int Mysqlcontrols::GetResultCount(Mysqlcontrols *self)
{
    return GUI->myquery->RecordCount;
}

bool Mysqlcontrols::Mysql_SubmitQuery(Mysqlcontrols *self,
                                      int query_id,
                                      int player_id,
                                      int expected_query_id,
                                      String data,
                                      String param2)
{
    mySQLtask *task = new mySQLtask(query_id, player_id, expected_query_id, data, param2);
    self->thread_queue->thread->Acquire();
    self->thread_queue->EnqueueTask(task);
    self->worker_thread->Resume();
    self->thread_queue->thread->Release();
    return 1;
}

bool Mysqlcontrols::Mysql_SubmitQuery_FromCallback(Mysqlcontrols *self,
                                                   int query_id,
                                                   int player_id,
                                                   int expected_query_id,
                                                   String data,
                                                   String param2)
{
    mySQLtask *task = new mySQLtask(query_id, player_id, expected_query_id, data, param2);
    self->thread_queue->EnqueueTask(task);
    return 1;
}

bool Mysqlcontrols::Query(Mysqlcontrols *self, String query)
{
    bool result = true;
    try
    {
        if (GUI->myquery->Active)
            GUI->myquery->Active = false;
        GUI->myquery->SQL->Clear();
        GUI->myquery->SQL->Add(query);
        GUI->myquery->Active = true;
        if (GUI->myquery->RecordCount > 0)
        {
            if (GUI->myquery->Active)
                GUI->myquery->Open();
        }
    }
    catch (...)
    {
        result = false;
        self->query_error_count = self->query_error_count + 1;
    }
    return result;
}

bool Mysqlcontrols::Mysql_ExecDirect(Mysqlcontrols *self,
                                     int expected_query_id,
                                     String query)
{
    mySQLtask *task = new mySQLtask(1, 0, expected_query_id, "", query);
    self->thread_queue->thread->Acquire();
    self->thread_queue->EnqueueTask(task);
    self->worker_thread->Resume();
    self->thread_queue->thread->Release();
    return 1;
}

bool Mysqlcontrols::Mysql_ExecDirect_FromCallback(Mysqlcontrols *self,
                                                  int expected_query_id,
                                                  String query)
{
    mySQLtask *task = new mySQLtask(1, 0, expected_query_id, "", query);
    self->thread_queue->EnqueueTask(task);
    return 1;
}

bool Mysqlcontrols::ExecDrop(Mysqlcontrols *self, String query)
{
    bool result = true;
    try
    {
        if (GUI->myquery->Active)
            GUI->myquery->Active = false;
        GUI->myquery->SQL->Clear();
        GUI->myquery->SQL->Add(query);
        GUI->myquery->ExecSQL();
    }
    catch (...)
    {
        result = false;
        self->exec_error_count = self->exec_error_count + 1;
    }
    return result;
}

int Mysqlcontrols::Db_GetActiveConnectionCount(Mysqlcontrols *self)
{
    return self->thread_queue->job_queue.size();
}

bool Mysqlcontrols::Database_CanReconnect(Mysqlcontrols *self)
{
    if (self->thread_queue->job_queue.size() > 0)
        return false;
    if (self->thread_queue->last_player_id > 0)
        return false;
    if (self->worker_thread->Suspended)
    {
        self->worker_thread->Terminate();
        return true;
    }
}

bool Mysqlcontrols::IsTaskPending(Mysqlcontrols *self, int player_id)
{
    return self->thread_queue->HasPendingTask(player_id);
}

bool Mysqlcontrols::IsAsciiText(Mysqlcontrols *self, String value)
{
    for (int i = 1; i <= value.Length(); i++)
    {
        if ((unsigned char)value[i] > 'z')
            return 0;
    }
    return 1;
}

bool Mysqlcontrols::IsAlphabeticText(Mysqlcontrols *self, String value)
{
    bool result = true;
    if (value.Length() >= 1)
    {
        for (int i = value.Length(); i >= 1; i--)
        {
            if ((unsigned char)value[i] < 'A' || (unsigned char)value[i] > 'z')
            {
                result = false;
                break;
            }
            if ((unsigned char)value[i] > 'Z' && (unsigned char)value[i] < 'a')
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

String
Mysqlcontrols::Mysql_SanitizeString(Mysqlcontrols *self, String value, bool uppercase)
{
    if (value.Length() >= 1)
    {
        for (int i = value.Length(); i >= 1; i--)
        {
            if ((unsigned char)value[i] != ' ')
            {
                if ((unsigned char)value[i] >= '[' && (unsigned char)value[i] <= '`')
                    value.Delete(i, 1);
                else if ((unsigned char)value[i] <= '@' || (unsigned char)value[i] >= '{')
                    value.Delete(i, 1);
            }
        }
    }
    if (!uppercase)
        return AnsiLowerCase(value);
    else
        return AnsiUpperCase(value);
}

String Mysqlcontrols::Db_SanitizeString(Mysqlcontrols *self, String value)
{
    if (value.Length() >= 1)
    {
        for (int i = value.Length(); i >= 1; i--)
        {
            if (String(value[i]) == "'" || value[i] == '%' || value[i] == '[' ||
                value[i] == '*' || value[i] == ']' || value[i] == '(' ||
                value[i] == ',' || value[i] == ')' || value[i] == '&' ||
                value[i] == '{' || value[i] == '#' || value[i] == '}' ||
                value[i] == '"' || value[i] == ':' || value[i] == ';' ||
                value[i] == (char)0xc8 || value[i] == '=' || value[i] == '/' ||
                value[i] == '\\' || value[i] == (char)0xff)
                value.Delete(i, 1);
        }
    }
    return AnsiLowerCase(value);
}

// Normalizes an incoming player text field for several packet handlers (it runs
// before the '#'-command parser). Short text is left alone; long text is capped
// at 0x80, non-letter (or 'W') characters are dropped once more than 0x14 have
// been seen, and the whole field is lowercased when it holds more than 0x28
// uppercase letters.
void Mysqlcontrols::NormalizePlayerText(Mysqlcontrols *self, String &message)
{
    if (message.Length() < TEXT_MIN_LENGTH)
        return;
    if (message.Length() > TEXT_MAX_LENGTH)
        message = message.SubString(1, TEXT_MAX_LENGTH);
    int caps = 0;
    int other = 0;
    for (int i = message.Length(); i >= 1; i--)
    {
        if ((unsigned char)message[i] >= 'A' && (unsigned char)message[i] <= 'Z')
            caps++;
        if ((unsigned char)message[i] < 'A' || (unsigned char)message[i] > 'z' ||
            (unsigned char)message[i] == 'W')
        {
            other++;
            if (other > TEXT_SYMBOL_LIMIT)
                message.Delete(i, 1);
        }
    }
    if (caps > TEXT_CAPS_LIMIT)
        message = AnsiLowerCase(message);
}

TTimeStamp Mysqlcontrols::Server_GetUptime(Mysqlcontrols *self)
{
    return self->connected_time;
}

String Mysqlcontrols::DecodeString(Mysqlcontrols *self, String value)
{
    String reversed = "";
    String result = "";
    try
    {
        for (int i = value.Length(); i >= 1; i--)
            reversed = reversed + value[i];
        for (int j = 1; j <= value.Length(); j++)
        {
            char a = reversed[j];
            unsigned char b = a;
            int u = b;
            bool done = false;
            if (!done && u >= '0' && u <= '9')
            {
                u = '9' - u + '0';
                b = u;
                a = b;
                result.Insert(String(a), result.Length() + 1);
                done = true;
            }
            if (!done && u >= 'a' && u <= 'z')
            {
                u = 'z' - u + 'a';
                b = u;
                a = b;
                result.Insert(String(a), result.Length() + 1);
                done = true;
            }
            if (!done)
                result.Insert(String(a), result.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return result;
}
