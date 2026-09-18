#ifndef SettingsH
#define SettingsH

#include <Classes.hpp>

// The server configuration record (./config/server.ini). Layout pinned by the
// reference constructor (0x413b6c) stores and the ReadIni* key mapping in
// LoadConfig (0x413ff4); sizeof is 0x8c. Field names follow the INI keys the
// parser matches (server/protocol keys are observable strings), the runtime
// message "World communication changed to: " for +0x78, and the observed
// value ranges:
//   +0x00 AnsiString  welcome_message        "welcomemessage"
//   +0x04 AnsiString  join_message           "joinmessage"  (unset default)
//   +0x08 AnsiString  server_name            "servername"   default "endl serv"
//   +0x0c int         port                   "port"         default 8610
//   +0x10 AnsiString  mysql_host             "mysqlhost"    default "localhost"
//   +0x14 int         mysql_port             "mysqlport"    default 3306
//   +0x18 int         login_protection       "loginprotection" low=8 high=4 else 6
//   +0x1c int         max_players            "maxplayers"
//   +0x20 int         max_connections        "maxconnections"
//   +0x24 int         max_clones             "maxclones"
//   +0x28 int         max_maps               "maxmaps"
//   +0x2c int         max_quests             "maxquests"
//   +0x30 int         start_map / start_x / start_y
//   +0x3c int         rescue_map / rescue_x / rescue_y
//   +0x48 int         jail_map / jail_x / jail_y
//   +0x54 int         spy_and_light_guide_flood_rate   "adminlevel1"
//   +0x58 int         guardian_flood_rate               "adminlevel2"
//   +0x5c int         game_master_flood_rate            "adminlevel3"
//   +0x60 int         high_game_master_flood_rate       "adminlevel4"
//   +0x64 char        account_lock           "accountlock"
//   +0x65 char        access_lock            "accesslock"
//   +0x68 int         max_kills              "maxkills"     clamp 10000
//   +0x6c char        server_status          "serverstatus"
//   +0x6d char        memory_map             "memorymap"
//   +0x6e char        sql_queue              "sqlqueque"
//   +0x6f char        sql_smart              "sqlsmart"
//   +0x70 char        db_shutdown            "dbshutdown"
//   +0x71 char        mm_timer               "mmtimer"
//   +0x74 int         refresh_time           "refreshtime"  minutes, clamp 1..30
//   +0x78 char        world_communication    toggled by Player_HandlePacket
//   +0x79 char        chat_log               "chatlog"
//   +0x7c int         group_max              "groupmax"     clamp 10
//   +0x80 int         group_balance          "groupbalance"
//   +0x84 int         popup_style            "popupstyle"   clamp 0..1
//   +0x88 TStringList *ini_file
//
// The class has no virtual functions (the ctor never writes a vptr). The
// reference carries an explicit destructor (0x413da8) whose body scope marker
// is observable, so an empty user destructor is declared; the compiler appends
// the four AnsiString member destructions and does not free ini_file.
class Settings
{
  public:
    String welcome_message;
    String join_message;
    String server_name;
    int port;
    String mysql_host;
    int mysql_port;
    int login_protection;
    int max_players;
    int max_connections;
    int max_clones;
    int max_maps;
    int max_quests;
    int start_map;
    int start_x;
    int start_y;
    int rescue_map;
    int rescue_x;
    int rescue_y;
    int jail_map;
    int jail_x;
    int jail_y;
    int spy_and_light_guide_flood_rate;
    int guardian_flood_rate;
    int game_master_flood_rate;
    int high_game_master_flood_rate;
    char account_lock;
    char access_lock;
    char pad_66[2];
    int max_kills;
    char server_status;
    char memory_map;
    char sql_queue;
    char sql_smart;
    char db_shutdown;
    char mm_timer;
    char pad_72[2];
    int refresh_time;
    char world_communication;
    char chat_log;
    char pad_7a[2];
    int group_max;
    int group_balance;
    int popup_style;
    TStringList *ini_file;

    Settings();
    ~Settings();

    static String GetJoinMessage(Settings *self);
    static String GetServerName(Settings *self);
    static int GetPort(Settings *self);
    static int GetStartMap(Settings *self);
    static int GetStartX(Settings *self);
    static int GetStartY(Settings *self);
    static int GetJailMap(Settings *self);
    static int GetJailX(Settings *self);
    static int GetJailY(Settings *self);
    static int GetRescueMap(Settings *self);
    static int GetRescueX(Settings *self);
    static int GetRescueY(Settings *self);
    static int GetMaxPlayers(Settings *self);
    static int GetMaxConnections(Settings *self);
    static int GetMaxClones(Settings *self);
    static int GetMaxMaps(Settings *self);
    static int GetMaxQuests(Settings *self);
    static char GetAccountLock(Settings *self);
    static char GetAccessLock(Settings *self);
    static int GetMaxKills(Settings *self);
    static char GetMemoryMap(Settings *self);
    static char GetSqlSmart(Settings *self);
    static char GetWorldCommunication(Settings *self);
    static void SetWorldCommunication(Settings *self, bool value);
    static int GetRefreshSeconds(Settings *self);
    static int GetGroupMax(Settings *self);
    static int GetSpyAndLightGuideFloodRate(Settings *self);
    static int GetGuardianFloodRate(Settings *self);
    static int GetGameMasterFloodRate(Settings *self);
    static int GetHighGameMasterFloodRate(Settings *self);
    static char GetChatLog(Settings *self);

    static void LoadConfig(Settings *self);
    static void SetIniPath(Settings *self, String path);
    static void CloseIni(Settings *self);
    static int ReadIniInt(Settings *self, String key, int default_value);
    static String ReadIniString(Settings *self, String key, String default_value);
    static char ReadIniBool(Settings *self, String key, char default_value);
};

#endif
