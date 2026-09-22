#ifndef PacketsH
#define PacketsH

#include <Classes.hpp>
#include <ScktComp.hpp>
#include <vector.h>
#include <deque>
#include <stack>
#include <queue>

#include "Protocol.h"
#include "Map.h"
#include "Mapcontrol.h"

#include "Server.h"

// Cross-unit operations this unit defines as free functions; the controllers
// and Mainform reference these exact mangled names.
ChestItem *Mapcontrol_GetByIndex(MapContainer *map_control, int index);
void Game_Tick(Packets *server);
void Server_ClientRead(Packets *server, TCustomWinSocket *socket, String data);
void Server_Shutdown(Packets *server);
void Server_RemovePlayer(Packets *server, TCustomWinSocket *socket);
String Server_FormatSentTraffic(Packets *server);
String Server_FormatReceivedTraffic(Packets *server);
bool Player_HandlePacket(Packets *server, Player *player, String data);
void Player_CalculateStats(Packets *server, Player *player);
String Player_SerializeAvatar(Packets *server, Player *player, int arg);
String Player_SerializePaperdoll(Packets *server, Player *player);
void Client_SendRaw(Packets *server, Player *client, String data, int break_byte);
void Client_SendEncoded(Packets *server,
                        Player *player,
                        unsigned char action,
                        unsigned char family,
                        String data);
bool Walk_Execute(Packets *server, Player *player, int action, String *data);
bool Attack_Execute(Packets *server, Player *caster, int action, String *data);
bool Spell_Execute(Packets *server, Player *caster, int action, String *data);
bool Face_Execute(Packets *server, Player *player, int action, String *data);
bool Chair_Execute(Packets *server, Player *player, int action, String *data);
bool Player_CheckIdleWarp(Packets *server, Player *player, int x, int y);
void Player_Respawn(Packets *server, Player *player);
void Player_Warp(Packets *server,
                 Player *player,
                 int target_map,
                 MapCoord coords,
                 int warp_effect,
                 bool do_leave);
void Server_BroadcastToPartyExceptSelf(Packets *server,
                                       Player *player,
                                       unsigned char action,
                                       unsigned char family,
                                       String data);
void Server_BroadcastToParty(Packets *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data);
void Guild_BroadcastToAll(Packets *server,
                          Player *player,
                          unsigned char action,
                          unsigned char family,
                          String data);
void Server_BroadcastAdjacent(Packets *server,
                              Player *player,
                              MapCoord coords,
                              unsigned char action,
                              unsigned char family,
                              String data);
void Server_BroadcastNearTile(Packets *server,
                              int skip_id,
                              int map_id,
                              MapCoord coord,
                              unsigned char action,
                              unsigned char family,
                              String data);
void Admin_BroadcastToAll(Packets *server,
                          unsigned char action,
                          unsigned char family,
                          String data);
void Admin_ReportToGMs(Packets *server,
                       Player *player,
                       unsigned char action,
                       unsigned char family,
                       String data);
void Admin_BroadcastToAdmins(Packets *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data);
void Server_BroadcastNearby(Packets *server,
                            Player *player,
                            unsigned char action,
                            unsigned char family,
                            String data);
void Server_BroadcastToMap(
    Packets *server, int map_id, unsigned char action, unsigned char family, String data);

int Math_Abs(int value);

String EO_EncodeNumber(Packets *server, unsigned int value, int width);
String EO_Encode_Interleave(Packets *server, int multiple, char *begin, char *end);
String EO_Decode_Deinterleave(Packets *server, int multiple, char *begin, char *end);

struct EOEncodedObj
{
    struct
    {
        int value;
    };
    EOEncodedObj()
    {
    }
};

int FUN_0044f73c(void *range);
int FUN_0044f710(void *range);
int EO_DecodeNumber(Packets *self, String data);
int EO_DecodeByte(Packets *self, char value);
char EO_GetBreakByte(Packets *self, int value);
unsigned int Server_DecodePacketLength(Packets *self, String data);
bool Login_CheckConnectionThreshold(Packets *server);
void Connection_Ping(Packets *server);
void PacketReader_Init(Packets *reader, String data, unsigned char break_byte);
String PacketReader_GetBreakString(Packets *reader);
String
PacketReader_GetBreakStringAt(Packets *reader, int end, String break_str, char append);
bool CharName_CheckUnique(Packets *server, String name);

bool Coords_IsAdjacent(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InViewRange(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InViewRing(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InViewRangeReverse(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InItemViewRing(Packets *self, int x1, int y1, int x2, int y2);

void Server_AddReceivedBytes(Packets *server, int value);
void Server_AddSentBytes(Packets *server, int value);
bool Coords_IsWithinTwo(Packets *self, int x1, int y1, int x2, int y2);
String NpcRange_Lookup(Packets *server, Player *player, unsigned int npc_index);
void Player_FireQuestTriggers(Packets *server,
                              Player *player,
                              int state_index,
                              int value);
void Server_BroadcastToMapAndAdmins(
    Packets *server, int map_id, unsigned char action, unsigned char family, String data);
void Admin_BroadcastToOtherAdmins(Packets *server,
                                  Player *player,
                                  unsigned char action,
                                  unsigned char family,
                                  String data);
void Server_BroadcastToAll(Packets *server,
                           unsigned char action,
                           unsigned char family,
                           String data);
void Server_SyncMapHazardFlags(Packets *server, int map_id);
void Server_AppendChatLog(Packets *server, String message);
void Talk_PlayerWhisper(Packets *server, int map_id, String message, int break_byte);
String Server_BuildOnlineNames(Packets *server);
String Server_BuildOnlineList(Packets *server);
String Refresh_BuildReply(Packets *server, Player *player);
String Party_EncodeMemberList(Packets *server, Player *player);
bool Server_TickOncePerFiveSeconds(Packets *server);
String Message_BuildServerStatus(Packets *server);
String Paperdoll_BuildReply(Packets *server, Player *player);
void Player_ApplyQuestActions(Packets *server,
                              Player *player,
                              PlayerQuest *tracker,
                              bool repeat);

#endif
