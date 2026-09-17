#include <vcl.h>
#pragma hdrstop

#include "Mysqlcontrols.h"
#include "Mainform.h"

#pragma package(smart_init)

// Cross-unit helpers whose units are reconstructed separately.
extern void FUN_0053d070(Filecache *cache);
extern void FUN_0053d2a8(int cache);
extern void FUN_0053d53c(int cache);
extern void FUN_005340f4(int thread, int value);

Mysqlcontrols::Mysqlcontrols()
{
    file_cache = new Filecache;
    thread_queue = new Mysqlthread;
    worker_thread = Mysqlthread::Spawn(
        NULL, true, GUI->mysession, false, thread_queue, GUI->myquery, GUI->mysql);
    field_18 = 0;
    field_1c = 0;
    connected_time = DateTimeToTimeStamp(Now());
    last_query_time = DateTimeToTimeStamp(Now());
}

void Mysqlcontrols::Free(Mysqlcontrols *self, unsigned char free_flags)
{
    if (self != 0 && (free_flags & 1))
        delete self;
}

bool Mysqlcontrols::TestConnection(Mysqlcontrols *self)
{
    GUI->mysql->Params->Clear();
    GUI->mysql->Params->Add(FUN_00477a00(self, "xxz_hhvowmv=vnzmivhf"));
    GUI->mysql->Params->Add(FUN_00477a00(self, "5pz0agrc=wildhhzk"));
    GUI->mysql->Connected = true;
    return GUI->mysql->Connected;
}

void Mysqlcontrols::Connect(Mysqlcontrols *self,
                            int version_patch,
                            int version_minor,
                            int version_major)
{
    FUN_0053d070(self->file_cache);
    if (self->file_cache->dirty == false)
        ExecDrop(self, "UPDATE endl_characters SET online = 0 WHERE online = 1");
    ExecDrop(self, "DELETE FROM endl_server");

    String connect_string = "INSERT INTO endl_server (version, characters, connections, "
                            "players, uptime, upload, download) VALUES (";
    connect_string = connect_string + IntToStr(version_major) + "." +
                     IntToStr(version_minor) + "." + IntToStr(version_patch);
    connect_string = connect_string + "',0,0,0,0,'no data','no data')";
    ExecDrop(self, connect_string);

    Query(self, "SHOW TABLE STATUS FROM endless_db");
    while (GUI->myquery->Eof == false)
    {
        if (GUI->myquery->Fields->Fields[0]->AsString == "endl_accounts")
        {
            if (GUI->myquery->Fields->Fields[3]->AsString.LowerCase() == "fixed")
                self->file_cache->accounts_count =
                    GUI->myquery->Fields->Fields[4]->AsInteger;
        }
        if (GUI->myquery->Fields->Fields[0]->AsString == "endl_characters")
        {
            if (GUI->myquery->Fields->Fields[3]->AsString.LowerCase() == "fixed")
                self->file_cache->characters_count =
                    GUI->myquery->Fields->Fields[4]->AsInteger;
        }
        if (GUI->myquery->Fields->Fields[0]->AsString == "endl_guilds")
        {
            if (GUI->myquery->Fields->Fields[3]->AsString.LowerCase() == "fixed")
                self->file_cache->guilds_count =
                    GUI->myquery->Fields->Fields[4]->AsInteger;
        }
        GUI->myquery->Next();
    }

    if (self->file_cache->dirty == false)
    {
        Query(self,
              "SELECT privilege, name, title, level, experience, gender FROM "
              "endl_characters "
              "WHERE privilege = 0 ORDER BY experience desc LIMIT 100");
        FUN_004752d4(self);
    }
    else
    {
        FUN_0053d2a8((int)self->file_cache);
    }

    if (self->file_cache->dirty == false)
    {
        Query(self,
              "SELECT ident_guild, guild, SUM(level) as exptotal, MAX(level) as exphigh, "
              "COUNT(ident_guild) as members FROM endl_characters WHERE "
              "LENGTH(ident_guild) > 1 "
              "AND privilege = 0 GROUP BY ident_guild desc ORDER BY exptotal desc LIMIT "
              "100");
        FUN_00475b20(self);
    }
    else
    {
        FUN_0053d53c((int)self->file_cache);
    }
}

void Mysqlcontrols::FUN_004752d4(Mysqlcontrols *self)
{
    while (GUI->myquery->Eof == false)
    {
        FilecacheEntry *entry = new FilecacheEntry;
        entry->field_0 = GUI->myquery->Fields->FieldByName("privilege")->AsInteger;
        entry->field_4 = GUI->myquery->Fields->FieldByName("name")->AsString;
        entry->field_8 = GUI->myquery->Fields->FieldByName("title")->AsString;
        entry->field_c = GUI->myquery->Fields->FieldByName("level")->AsInteger;
        entry->field_10 = GUI->myquery->Fields->FieldByName("experience")->AsInteger;
        entry->field_14 = GUI->myquery->Fields->FieldByName("gender")->AsInteger;
        self->file_cache->pending_player_writes.insert(
            self->file_cache->pending_player_writes.end(), entry);
        GUI->myquery->Next();
    }
}

void Mysqlcontrols::FUN_00475b20(Mysqlcontrols *self)
{
    while (GUI->myquery->Eof == false)
    {
        FilecacheEntryB *entry = new FilecacheEntryB;
        entry->field_0 = GUI->myquery->Fields->FieldByName("ident_guild")->AsString;
        entry->field_4 = GUI->myquery->Fields->FieldByName("guild")->AsString;
        entry->field_8 = GUI->myquery->Fields->FieldByName("exptotal")->AsInteger;
        entry->field_c = GUI->myquery->Fields->FieldByName("exphigh")->AsInteger;
        entry->field_10 = GUI->myquery->Fields->FieldByName("members")->AsInteger;
        self->file_cache->pending_guild_writes.insert(
            self->file_cache->pending_guild_writes.end(), entry);
        GUI->myquery->Next();
    }
}

void Mysqlcontrols::FUN_004762c8(
    Mysqlcontrols *self, int refresh, int conn, int idle, int stat, String a, String b)
{
    TTimeStamp now_stamp = DateTimeToTimeStamp(Now());
    int elapsed = (now_stamp.Time - self->last_query_time.Time) / 1000 +
                  (now_stamp.Date - self->last_query_time.Date) * 0x15180;
    if (refresh <= elapsed)
    {
        self->connected_time = DateTimeToTimeStamp(Now());
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int uptime = (stamp.Time - self->connected_time.Time) / 1800000 +
                     (stamp.Date - self->connected_time.Date) * 0x30;
        String s = "UPDATE endl_server SET ";
        s = s + "uptime = " + IntToStr(uptime);
        s = s + ", connections = " + IntToStr(conn);
        s = s + ", players = " + IntToStr(idle);
        s = s + ", most = " + IntToStr(stat);
        s = s + ", upload = '" + a;
        s = s + "', download = '" + b;
        s = s + "'";
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

void Mysqlcontrols::FUN_00476bfc(Mysqlcontrols *self)
{
    GUI->myquery->Next();
}

bool Mysqlcontrols::FUN_00476c14(Mysqlcontrols *self)
{
    return GUI->myquery->Eof;
}

int Mysqlcontrols::GetResultCount(Mysqlcontrols *self)
{
    return GUI->myquery->RecordCount;
}

bool Mysqlcontrols::Mysql_SubmitQuery(Mysqlcontrols *db,
                                      int query_id,
                                      int player_id,
                                      int expected_query_id,
                                      String p5,
                                      String p6)
{
    Mysqltask *task = new Mysqltask(query_id, player_id, expected_query_id, p5, p6);
    db->thread_queue->thread->Acquire();
    Mysqlthread::EnqueueTask(db->thread_queue, task);
    db->worker_thread->Resume();
    db->thread_queue->thread->Release();
    return 1;
}

bool Mysqlcontrols::Mysql_SubmitQuery_FromCallback(Mysqlcontrols *db,
                                                   int query_id,
                                                   int player_id,
                                                   int expected_query_id,
                                                   String p5,
                                                   String p6)
{
    Mysqltask *task = new Mysqltask(query_id, player_id, expected_query_id, p5, p6);
    Mysqlthread::EnqueueTask(db->thread_queue, task);
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
        self->field_18 = self->field_18 + 1;
    }
    return result;
}

bool Mysqlcontrols::Mysql_ExecDirect(Mysqlcontrols *db, int unk, String query)
{
    Mysqltask *task = new Mysqltask(1, 0, unk, "", query);
    db->thread_queue->thread->Acquire();
    Mysqlthread::EnqueueTask(db->thread_queue, task);
    db->worker_thread->Resume();
    db->thread_queue->thread->Release();
    return 1;
}

bool Mysqlcontrols::Mysql_ExecDirect_FromCallback(Mysqlcontrols *db,
                                                  int unk,
                                                  String query)
{
    Mysqltask *task = new Mysqltask(1, 0, unk, "", query);
    Mysqlthread::EnqueueTask(db->thread_queue, task);
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
        self->field_1c = self->field_1c + 1;
    }
    return result;
}

int Mysqlcontrols::Db_GetActiveConnectionCount(Mysqlcontrols *db)
{
    return db->thread_queue->job_queue.size();
}

bool Mysqlcontrols::Database_CanReconnect(Mysqlcontrols *db)
{
    if (db->thread_queue->job_queue.size() > 0)
        return false;
    if (db->thread_queue->last_player_id > 0)
        return false;
    if (db->worker_thread->Suspended)
    {
        db->worker_thread->Terminate();
        return true;
    }
    return false;
}

void Mysqlcontrols::FUN_004772a0(Mysqlcontrols *self, int value)
{
    FUN_005340f4((int)self->thread_queue, value);
}

bool Mysqlcontrols::FUN_004772b8(Mysqlcontrols *self, String value)
{
    for (int i = 1; i <= value.Length(); i++)
    {
        if ((unsigned char)value[i] > 'z')
            return 0;
    }
    return 1;
}

bool Mysqlcontrols::SpamGuard_CheckCooldown(Mysqlcontrols *self, String value)
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
Mysqlcontrols::Mysql_SanitizeString(Mysqlcontrols *db, String value, bool uppercase)
{
    if (value.Length() >= 1)
    {
        for (int i = value.Length(); i >= 1; i--)
        {
            if ((unsigned char)value[i] != ' ')
            {
                if ((unsigned char)value[i] < '[' || (unsigned char)value[i] > '`')
                {
                    if ((unsigned char)value[i] <= '@' || (unsigned char)value[i] >= '{')
                        value.Delete(i, 1);
                }
                else
                {
                    value.Delete(i, 1);
                }
            }
        }
    }
    if (uppercase)
        return value.UpperCase();
    return value.LowerCase();
}

String Mysqlcontrols::Db_SanitizeString(Mysqlcontrols *db, String in)
{
    if (in.Length() >= 1)
    {
        for (int i = in.Length(); i >= 1; i--)
        {
            if (String(in[i]) == "'" || in[i] == '%' || in[i] == '[' || in[i] == '*' ||
                in[i] == ']' || in[i] == '(' || in[i] == ',' || in[i] == ')' ||
                in[i] == '&' || in[i] == '{' || in[i] == '#' || in[i] == '}' ||
                in[i] == '"' || in[i] == ':' || in[i] == ';' || in[i] == (char)0xc8 ||
                in[i] == '=' || in[i] == '/' || in[i] == '\\' || in[i] == (char)0xff)
                in.Delete(i, 1);
        }
    }
    return in.LowerCase();
}

void Mysqlcontrols::Chat_HasBadWords(Mysqlcontrols *ctx, String &message)
{
    if (message.Length() < 0x3c)
        return;
    if (message.Length() > 0x80)
        message = message.SubString(1, 0x80);
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
            if (other > 0x14)
                message.Delete(i, 1);
        }
    }
    if (caps > 0x28)
        message = message.LowerCase();
}

TTimeStamp Mysqlcontrols::Server_GetUptime(Mysqlcontrols *db)
{
    return db->connected_time;
}

String Mysqlcontrols::FUN_00477a00(Mysqlcontrols *db, String value)
{
    String reversed = "";
    for (int i = value.Length(); i >= 1; i--)
        reversed = reversed + String(value[i]);
    String result = "";
    for (int j = 1; j <= value.Length(); j++)
    {
        char c = reversed[j];
        if (c > '/' && c < ':')
            result.Insert(String((char)('i' - c)), result.Length() + 1);
        else if (c > '`' && c < '{')
            result.Insert(String((char)(0xdb - c)), result.Length() + 1);
        else
            result.Insert(String(c), result.Length() + 1);
    }
    return result;
}
