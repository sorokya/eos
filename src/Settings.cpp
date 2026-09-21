#include <vcl.h>
#pragma hdrstop

#include "Settings.h"

#pragma package(smart_init)

Settings::Settings()
{
    ini_file = new TStringList;
    welcome_message = "";
    server_name = "endl serv";
    mysql_host = "localhost";
    mysql_port = 3306;
    port = 8642;
    login_protection = 6;
    max_players = 100;
    max_connections = 100;
    max_clones = 2;
    max_maps = 250;
    max_quests = 250;
    start_map = 1;
    start_x = 1;
    start_y = 1;
    rescue_map = 1;
    rescue_x = 1;
    rescue_y = 1;
    jail_map = 1;
    jail_x = 1;
    jail_y = 1;
    max_kills = 0;
    spy_and_light_guide_flood_rate = 10;
    guardian_flood_rate = 10;
    game_master_flood_rate = 10;
    high_game_master_flood_rate = 10;
    world_communication = 1;
    chat_log = 0;
    account_lock = 0;
    access_lock = 0;
    server_status = 0;
    memory_map = 0;
    sql_queue = 1;
    mm_timer = 0;
    refresh_time = 30;
    popup_style = 0;
    LoadConfig(this);
}

Settings::~Settings()
{
}

String Settings::GetJoinMessage(Settings *self)
{
    return self->join_message;
}

String Settings::GetServerName(Settings *self)
{
    return self->server_name;
}

int Settings::GetPort(Settings *self)
{
    return self->port;
}

int Settings::GetStartMap(Settings *self)
{
    return self->start_map;
}

int Settings::GetStartX(Settings *self)
{
    return self->start_x;
}

int Settings::GetStartY(Settings *self)
{
    return self->start_y;
}

int Settings::GetJailMap(Settings *self)
{
    return self->jail_map;
}

int Settings::GetJailX(Settings *self)
{
    return self->jail_x;
}

int Settings::GetJailY(Settings *self)
{
    return self->jail_y;
}

int Settings::GetRescueMap(Settings *self)
{
    return self->rescue_map;
}

int Settings::GetRescueX(Settings *self)
{
    return self->rescue_x;
}

int Settings::GetRescueY(Settings *self)
{
    return self->rescue_y;
}

int Settings::GetMaxPlayers(Settings *self)
{
    return self->max_players;
}

int Settings::GetMaxConnections(Settings *self)
{
    return self->max_connections;
}

int Settings::GetMaxClones(Settings *self)
{
    return self->max_clones;
}

int Settings::GetMaxMaps(Settings *self)
{
    return self->max_maps;
}

int Settings::GetMaxQuests(Settings *self)
{
    return self->max_quests;
}

char Settings::GetAccountLock(Settings *self)
{
    return self->account_lock;
}

char Settings::GetAccessLock(Settings *self)
{
    return self->access_lock;
}

int Settings::GetMaxKills(Settings *self)
{
    return self->max_kills;
}

char Settings::GetMemoryMap(Settings *self)
{
    return self->memory_map;
}

char Settings::GetSqlSmart(Settings *self)
{
    return self->sql_smart;
}

char Settings::GetWorldCommunication(Settings *self)
{
    return self->world_communication;
}

void Settings::SetWorldCommunication(Settings *self, bool value)
{
    self->world_communication = value;
}

int Settings::GetRefreshSeconds(Settings *self)
{
    return self->refresh_time * 60;
}

int Settings::GetGroupMax(Settings *self)
{
    return self->group_max;
}

int Settings::GetSpyAndLightGuideFloodRate(Settings *self)
{
    return self->spy_and_light_guide_flood_rate;
}

int Settings::GetGuardianFloodRate(Settings *self)
{
    return self->guardian_flood_rate;
}

int Settings::GetGameMasterFloodRate(Settings *self)
{
    return self->game_master_flood_rate;
}

int Settings::GetHighGameMasterFloodRate(Settings *self)
{
    return self->high_game_master_flood_rate;
}

char Settings::GetChatLog(Settings *self)
{
    return self->chat_log;
}

void Settings::LoadConfig(Settings *self)
{
    SetIniPath(self, "./config/server.ini");
    self->server_name = ReadIniString(self, "servername", self->server_name);
    self->mysql_host = ReadIniString(self, "mysqlhost", self->mysql_host);
    self->mysql_port = ReadIniInt(self, "mysqlport", self->mysql_port);
    self->welcome_message = ReadIniString(self, "welcomemessage", self->welcome_message);
    self->join_message = ReadIniString(self, "joinmessage", self->join_message);
    self->port = ReadIniInt(self, "port", self->port);
    self->max_players = ReadIniInt(self, "maxplayers", self->max_players);
    self->max_connections = ReadIniInt(self, "maxconnections", self->max_connections);
    self->max_clones = ReadIniInt(self, "maxclones", self->max_clones);
    self->max_maps = ReadIniInt(self, "maxmaps", self->max_maps);
    self->max_quests = ReadIniInt(self, "maxquests", self->max_maps);
    self->start_map = ReadIniInt(self, "startmap", self->start_map);
    self->start_x = ReadIniInt(self, "startx", self->start_x);
    self->start_y = ReadIniInt(self, "starty", self->start_y);
    self->rescue_map = ReadIniInt(self, "rescuemap", self->rescue_map);
    self->rescue_x = ReadIniInt(self, "rescuex", self->rescue_x);
    self->rescue_y = ReadIniInt(self, "rescuey", self->rescue_y);
    self->jail_map = ReadIniInt(self, "jailmap", self->jail_map);
    self->jail_x = ReadIniInt(self, "jailx", self->jail_x);
    self->jail_y = ReadIniInt(self, "jaily", self->jail_y);
    self->spy_and_light_guide_flood_rate =
        ReadIniInt(self, "adminlevel1", self->spy_and_light_guide_flood_rate);
    self->guardian_flood_rate =
        ReadIniInt(self, "adminlevel2", self->guardian_flood_rate);
    self->game_master_flood_rate =
        ReadIniInt(self, "adminlevel3", self->game_master_flood_rate);
    self->high_game_master_flood_rate =
        ReadIniInt(self, "adminlevel4", self->high_game_master_flood_rate);
    self->account_lock = ReadIniBool(self, "accountlock", self->account_lock);
    self->access_lock = ReadIniBool(self, "accesslock", self->access_lock);
    self->max_kills = ReadIniInt(self, "maxkills", self->max_kills);
    self->server_status = ReadIniBool(self, "serverstatus", self->server_status);
    self->memory_map = ReadIniBool(self, "memorymap", self->memory_map);
    self->sql_queue = ReadIniBool(self, "sqlqueque", self->sql_queue);
    self->sql_smart = ReadIniBool(self, "sqlsmart", self->sql_smart);
    self->db_shutdown = ReadIniBool(self, "dbshutdown", self->db_shutdown);
    self->mm_timer = ReadIniBool(self, "mmtimer", self->mm_timer);
    self->refresh_time = ReadIniInt(self, "refreshtime", self->refresh_time);
    self->popup_style = ReadIniInt(self, "popupstyle", self->popup_style);
    self->group_max = ReadIniInt(self, "groupmax", self->group_max);
    self->group_balance = ReadIniInt(self, "groupbalance", self->group_balance);
    self->chat_log = ReadIniBool(self, "chatlog", self->chat_log);

    String protection = ReadIniString(self, "loginprotection", self->join_message);
    if (AnsiLowerCase(protection) == "low")
        self->login_protection = 8;
    if (AnsiLowerCase(protection) == "high")
        self->login_protection = 4;

    if (self->group_max > 10)
        self->group_max = 10;
    if (self->refresh_time > 30)
        self->refresh_time = 30;
    if (self->refresh_time < 1)
        self->refresh_time = 1;
    if (self->rescue_map < 1)
        self->rescue_map = 1;
    if (self->start_map < 1)
        self->start_map = 1;
    if (self->jail_map < 1)
        self->jail_map = 1;
    if (self->popup_style < 0)
        self->popup_style = 0;
    if (self->popup_style > 1)
        self->popup_style = 1;
    if (self->max_kills > 10000)
        self->max_kills = 10000;
    if (self->welcome_message.Length() > 250)
        self->welcome_message = self->welcome_message.SubString(1, 250);

    CloseIni(self);
}

void Settings::SetIniPath(Settings *self, String path)
{
    self->ini_file->Clear();

    try
    {
        String exe = Application->ExeName;
        int last_slash_pos = 0;
        if (exe.Length() >= 1)
        {
            for (int i = exe.Length(); i >= 1; i--)
            {
                if (exe[i] == '\\')
                {
                    last_slash_pos = i;
                    break;
                }
            }
        }

        String dir = "";
        if (exe.Length() >= 1 && last_slash_pos >= 1)
        {
            for (int p = 1; p < last_slash_pos; p++)
                dir = dir + exe[p];
        }

        exe = dir;

        String tail;
        if (exe[exe.Length()] != '\\')
            tail = "\\" + path;
        else
            tail = path;

        self->ini_file->LoadFromFile(exe + tail);
    }
    catch (...)
    {
    }
}

void Settings::CloseIni(Settings *self)
{
    self->ini_file->Clear();
}

int Settings::ReadIniInt(Settings *self, String key, int default_value)
{
    int result = default_value;
    if (self->ini_file->Count >= 1)
    {
        try
        {
            for (int i = 0; i < self->ini_file->Count; i++)
            {
                String line = self->ini_file->Strings[i];
                bool before_eq = true;
                String ini_name = "";
                String value = "";
                if (line.Length() >= 1)
                {
                    for (int j = 1; j <= line.Length(); j++)
                    {
                        if (line[j] == '=')
                            before_eq = false;
                        if (line[j] != ' ' && line[j] != '=')
                        {
                            if (before_eq)
                                ini_name = ini_name + line[j];
                            else
                                value = value + line[j];
                        }
                    }
                    if (AnsiLowerCase(ini_name) == AnsiLowerCase(key))
                    {
                        if (value.Length() >= 1)
                            result = StrToInt(value);
                        break;
                    }
                }
            }
        }
        catch (...)
        {
        }
    }
    return result;
}

String Settings::ReadIniString(Settings *self, String key, String default_value)
{
    String result = default_value;
    if (self->ini_file->Count >= 1)
    {
        try
        {
            for (int i = 0; i < self->ini_file->Count; i++)
            {
                String line = self->ini_file->Strings[i];
                bool before_eq = true;
                bool after_eq = true;
                String ini_name = "";
                String value = "";
                if (line.Length() >= 1)
                {
                    for (int j = 1; j <= line.Length(); j++)
                    {
                        if (line[j] == '=')
                            before_eq = false;
                        if (before_eq)
                        {
                            if (line[j] != ' ' && line[j] != '=')
                                ini_name = ini_name + line[j];
                        }
                        else if (after_eq)
                        {
                            if (line[j] != ' ' && line[j] != '=')
                            {
                                after_eq = false;
                                value = value + line[j];
                            }
                        }
                        else
                        {
                            value = value + line[j];
                        }
                    }
                    if (AnsiLowerCase(ini_name) == AnsiLowerCase(key))
                    {
                        if (value.Length() >= 1)
                            result = value;
                        break;
                    }
                }
            }
        }
        catch (...)
        {
        }
    }
    return result;
}

char Settings::ReadIniBool(Settings *self, String key, char default_value)
{
    char result = default_value;
    if (self->ini_file->Count >= 1)
    {
        try
        {
            for (int i = 0; i < self->ini_file->Count; i++)
            {
                String line = self->ini_file->Strings[i];
                bool before_eq = true;
                bool after_eq = true;
                String ini_name = "";
                String value = "";
                if (line.Length() >= 1)
                {
                    for (int j = 1; j <= line.Length(); j++)
                    {
                        if (line[j] == '=')
                            before_eq = false;
                        if (before_eq)
                        {
                            if (line[j] != ' ' && line[j] != '=')
                                ini_name = ini_name + line[j];
                        }
                        else if (after_eq)
                        {
                            if (line[j] != ' ' && line[j] != '=')
                            {
                                after_eq = false;
                                value = value + line[j];
                            }
                        }
                        else
                        {
                            value = value + line[j];
                        }
                    }
                    if (AnsiLowerCase(ini_name) == AnsiLowerCase(key))
                    {
                        if (value.Length() >= 1)
                        {
                            if (AnsiLowerCase(value) == "on" ||
                                AnsiLowerCase(value) == "1")
                                result = true;
                            else
                                result = false;
                        }
                        break;
                    }
                }
            }
        }
        catch (...)
        {
        }
    }
    return result;
}

