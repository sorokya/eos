#ifndef PacketsH
#define PacketsH

#include <Classes.hpp>
#include <ScktComp.hpp>
#include <vector>
#include <deque>
#include <stack>

#include "Map.h"
#include "Mapcontrol.h"

class Player;
class Players;
class Questengine;
class Settings;
class Mysqlcontrols;
class Logins;
class Banned;
class KillCounters;
class QuestCounters;
struct WeaponmapEntry;

// The application core. Layout recovered from the reference server constructor
// (Packets unit, 0x41670c) and the per-tick loop. sizeof is 0xc8, pinned by the
// `operator new(0xc8)` in Mainform's FormCreate.
class Server
{
  public:
    int state_0x00;                // +0x00
    int state_0x04;                // +0x04
    TStringList *wordfilter;       // +0x08
    WeaponmapEntry *weapon_map;    // +0x0c
    Players *players;              // +0x10
    Mysqlcontrols *mysql_controls; // +0x14
    Logins *logins;                // +0x18
    Banned *banned;                // +0x1c
    Questengine *quest_engine;     // +0x20
    Mapcontrol *map_control;       // +0x24
    Settings *settings;            // +0x28
    KillCounters *kill_counters;   // +0x2c
    QuestCounters *quest_counters; // +0x30
    int padding_0x34;              // +0x34
    TDateTime start_time;          // +0x38
    char *encode_buffer;           // +0x40
    char *packet_buffer;           // +0x44
    int version_patch;             // +0x48
    int version_minor;             // +0x4c
    int version_major;             // +0x50
    int sent_bytes;                // +0x54
    int sent_kilobytes;            // +0x58
    int sent_megabytes;            // +0x5c
    int received_bytes;            // +0x60
    int received_kilobytes;        // +0x64
    int received_megabytes;        // +0x68
    String reader_data;            // +0x6c
    int reader_pos;                // +0x70
    int reader_len;                // +0x74
    char reader_break_byte;        // +0x78
    int online_names_ttl;          // +0x7c
    String online_names_cache;     // +0x80
    int online_list_ttl;           // +0x84
    String online_list_cache;      // +0x88
    String field_0x8c[7];          // +0x8c
    int ping_history[3];           // +0xa8
    int ticks;                     // +0xb4
    char hangup_gate;              // +0xb8
    char kill_counters_cleared;    // +0xb9
    char shutting_down;            // +0xba
    char pad_bb[1];                // +0xbb
    int cheat_offset_x;            // +0xbc
    int cheat_offset_y;            // +0xc0
    int padding_0xc4;              // +0xc4

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
void Server_Shutdown(Server *server);
void Client_SendRaw(Server *server, Player *client, String data, int break_byte);
void Client_SendEncoded(Server *server,
                        Player *player,
                        unsigned char action,
                        unsigned char family,
                        String data);
bool Face_Execute(Server *server, Player *player, int action, String *data);
bool Chair_Execute(Server *server, Player *player, int action, String *data);
bool Player_CheckIdleWarp(Server *server, Player *player, int x, int y);
void Player_Respawn(Server *server, Player *player);
void Player_Warp(Server *server,
                 Player *player,
                 int target_map,
                 MapCoord coords,
                 int warp_anim,
                 bool do_leave);
void Server_BroadcastToPartyExceptSelf(Server *server,
                                       Player *player,
                                       unsigned char action,
                                       unsigned char family,
                                       String data);
void Server_BroadcastToParty(Server *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data);
void Guild_BroadcastToAll(Server *server,
                          Player *player,
                          unsigned char action,
                          unsigned char family,
                          String data);
void Server_BroadcastAdjacent(Server *server,
                              Player *player,
                              int x,
                              int y,
                              unsigned char action,
                              unsigned char family,
                              String data);
void Server_BroadcastNearTile(Server *server,
                              int skip_id,
                              int map_id,
                              int x,
                              int y,
                              unsigned char action,
                              unsigned char family,
                              String data);
void Admin_BroadcastToAll(Server *server,
                          unsigned char action,
                          unsigned char family,
                          String data);
void Admin_ReportToGMs(Server *server,
                       Player *player,
                       unsigned char action,
                       unsigned char family,
                       String data);
void Admin_BroadcastToAdmins(Server *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data);
void Server_BroadcastNearby(Server *server,
                            Player *player,
                            unsigned char action,
                            unsigned char family,
                            String data);
void Server_BroadcastToMap(
    Server *server, int map_id, unsigned char action, unsigned char family, String data);

int Math_Abs(int value);

MapContainer *Mapcontrol_Iter_Front(Mapcontrol *map_control);
void *Map_NpcIter_Begin(void *npc_list);
void *Map_NpcIter_End(void *npc_list);
void *GroundItemPtrVector_Begin(void *list);
void *PtrVector_GetEnd(void *list);
int GroundItemPtrVector_Count(void *list);

String EO_EncodeNumber(Server *server, unsigned int value, int width);
String EO_Encode_Interleave(Server *server, int multiple, char *begin, char *end);
int EO_DecodeNumber(void *self, String data);
int EO_DecodeByte(void *self, char value);
char EO_GetBreakByte(void *self, int value);
unsigned int Server_DecodePacketLength(void *self, String data);
bool Login_CheckConnectionThreshold(Server *server);
void Connection_Ping(Server *server);
void PacketReader_Init(Server *reader, String data, unsigned char break_byte);
String PacketReader_GetBreakString(Server *reader);
String
PacketReader_GetBreakStringAt(void *reader, int end, String break_str, char append);
bool CharName_CheckUnique(Server *server, String name);

bool Coords_IsAdjacent(void *self, int x1, int y1, int x2, int y2);
bool Server_InViewRange(void *self, int x1, int y1, int x2, int y2);
bool Server_InViewRing(void *self, int x1, int y1, int x2, int y2);
bool Server_InViewRangeReverse(void *self, int x1, int y1, int x2, int y2);
bool Server_InItemViewRing(void *self, int x1, int y1, int x2, int y2);

#endif
