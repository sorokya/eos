#ifndef PacketsH
#define PacketsH

#include <Classes.hpp>
#include <ScktComp.hpp>
#include <vector>

#include "Map.h"
#include "Mapcontrol.h"

class Player;
class Players;
class Questengine;
class Settings;
class Mysqlcontrols;
class Logins;
class Banned;
class Killcounters;
class Questcounterlist;

// The application core. Layout recovered from the reference server constructor
// (Packets unit, 0x41670c) and the per-tick loop. sizeof is 0xc8, pinned by the
// `operator new(0xc8)` in Mainform's FormCreate.
class Server
{
  public:
    int state_0x00;                       // +0x00
    int state_0x04;                       // +0x04
    TStringList *wordfilter;              // +0x08
    void *field_0x0c;                     // +0x0c
    Players *players;                     // +0x10
    Mysqlcontrols *mysql_controls;        // +0x14
    Logins *logins;                       // +0x18
    Banned *banned;                       // +0x1c
    Questengine *quest_engine;            // +0x20
    Mapcontrol *map_control;              // +0x24
    Settings *settings;                   // +0x28
    Killcounters *kill_counters;          // +0x2c
    Questcounterlist *quest_counter_list; // +0x30
    int field_0x34;                       // +0x34
    double start_time;                    // +0x38
    char *encode_buffer;                  // +0x40
    char *packet_buffer;                  // +0x44
    int version_patch;                    // +0x48
    int version_minor;                    // +0x4c
    int version_major;                    // +0x50
    int field_0x54;                       // +0x54
    int field_0x58;                       // +0x58
    int field_0x5c;                       // +0x5c
    int field_0x60;                       // +0x60
    int field_0x64;                       // +0x64
    int field_0x68;                       // +0x68
    String field_0x6c;                    // +0x6c
    int field_0x70;                       // +0x70
    int field_0x74;                       // +0x74
    int field_0x78;                       // +0x78
    int online_names_ttl;                 // +0x7c
    String online_names_cache;            // +0x80
    int online_list_ttl;                  // +0x84
    String online_list_cache;             // +0x88
    String field_0x8c[7];                 // +0x8c
    int field_0xa8;                       // +0xa8
    int field_0xac;                       // +0xac
    int field_0xb0;                       // +0xb0
    int ticks;                            // +0xb4
    char field_0xb8;                      // +0xb8
    char flag_0xb9;                       // +0xb9
    char flag_0xba;                       // +0xba
    char pad_bb[1];                       // +0xbb
    int cheat_offset_x;                   // +0xbc
    int cheat_offset_y;                   // +0xc0
    int field_0xc4;                       // +0xc4

    Server(Mapcontrol *map_control,
           Questengine *quest_engine,
           Players *players,
           Settings *settings,
           Mysqlcontrols *mysql_controls,
           Logins *logins,
           int version_patch,
           int version_minor,
           int version_major);
    ~Server();
};

// Cross-unit operations this unit defines as free functions; the controllers
// and Mainform reference these exact mangled names.
MapContainer *MapVector_Begin(Mapcontrol *map_control);
MapContainer *MapVector_End(Mapcontrol *map_control);
int Mapcontrol_GetCount(Mapcontrol *map_control);
MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index);
void Game_Tick(Server *server);
void Server_ClientRead(Server *server, TCustomWinSocket *socket, String data);
void Client_SendEncoded(
    Server *server, Player *player, int action, int family, String data);
void Server_BroadcastToMap(
    Server *server, int map_id, int action, int family, String data);
void Server_BroadcastNearby(
    Server *server, Player *player, int action, int family, String data);

int Math_Abs(int value);

#endif
