#include <vcl.h>
#include <stdio.h>
#pragma hdrstop

#include "Packets.h"
#include "Player.h"
#include "Players.h"
#include "Weaponmap.h"
#include "Banned.h"
#include "Killcounters.h"
#include "Questcounters.h"
#include "Mysqlcontrols.h"
#include "Filecache.h"
#include "Settings.h"
#include "Logins.h"
#include "Mainform.h"
#include "Gamecontrol.h"
#include "Innvalues.h"
#include "Shopvalues.h"
#include "Npcvalues.h"
#include "Mapcontrol.h"
#include "Mapobject.h"
#include "Npc.h"
#include "Itemground.h"
#include "Itemvalues.h"
#include "Questengine.h"
#include "Playerquest.h"
#include "Skillvalues.h"
#include "Classvalues.h"
#include "Learnvalues.h"
#include "Weddings.h"
#include "Jukeboxcontrol.h"
#include "Msgboardcontrol.h"
#include "Newscontrol.h"
#include "Protocol.h"

#pragma package(smart_init)

void Game_Tick(Packets *server);
void Server_Shutdown(Packets *server);
void Server_RemovePlayer(Packets *server, TCustomWinSocket *socket);
int FUN_0044f97c(MapContainer *map_control);
int FUN_0044f9bc(MapContainer *map_control);
MapItem *Mapcontrol_GetByIndex(MapContainer *map_control, int index);
void Server_ClientRead(Packets *server, TCustomWinSocket *socket, String data);
bool Player_HandlePacket(Packets *server, Player *player, String data);
void MysqlCallback_Dispatch(Packets *server, mySQLtask *query_result);
void Player_FireQuestTriggers(Packets *server,
                              Player *player,
                              int state_index,
                              int value);
void Player_EvaluateQuestRules(Packets *server,
                               Player *player,
                               PlayerQuest *tracker,
                               QuestState *state,
                               int event,
                               int arg);
void Player_ApplyQuestActions(Packets *server,
                              Player *player,
                              PlayerQuest *tracker,
                              bool repeat);
void Login_SendCharacterList(Packets *server,
                             Player *player,
                             PacketAction action,
                             PacketFamily family,
                             String data);
String Party_EncodeMemberList(Packets *server, Player *player);
String Walk_BuildReply(Packets *server, Player *player);
String Refresh_BuildReply(Packets *server, Player *player);
String Player_SerializeAvatar(Packets *server, Player *player, int arg);
String Player_SerializePaperdoll(Packets *server, Player *player);
String Paperdoll_BuildReply(Packets *server, Player *player);
String Server_BuildOnlineNames(Packets *server);
String Message_BuildServerStatus(Packets *server);
String Server_BuildOnlineList(Packets *server);
String NpcRange_Lookup(Packets *server, Player *player, unsigned int npc_index);
bool FUN_00462374(Packets *server, Player *player, String data);
String Server_BuildInitOkReply(Packets *server, Player *player);
String Server_BuildInitVersionReply(Packets *server);
String Server_BuildInitBanReply(Packets *server);
void Client_SendRaw(Packets *server, Player *client, String data, int break_byte);
void Talk_PlayerWhisper(Packets *server, int map_id, String message, int break_byte);
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
void Server_BroadcastToPartyOnMap(Packets *server,
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
void Server_BroadcastToMapAndAdmins(
    Packets *server, int map_id, unsigned char action, unsigned char family, String data);
void Server_BroadcastToMap(
    Packets *server, int map_id, unsigned char action, unsigned char family, String data);
void Server_BroadcastNearTile(Packets *server,
                              int skip_id,
                              int map_id,
                              MapCoord coord,
                              unsigned char action,
                              unsigned char family,
                              String data);
void Admin_BroadcastToOtherAdmins(Packets *server,
                                  Player *player,
                                  unsigned char action,
                                  unsigned char family,
                                  String data);
void Admin_BroadcastToAll(Packets *server,
                          unsigned char action,
                          unsigned char family,
                          String data);
void Server_BroadcastToAll(Packets *server,
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
void Client_SendEncoded(Packets *server,
                        Player *player,
                        unsigned char action,
                        unsigned char family,
                        String data);
void Player_Respawn(Packets *server, Player *player);
void Player_Warp(Packets *server,
                 Player *player,
                 int target_map,
                 MapCoord coords,
                 int warp_effect,
                 bool do_leave);
bool Player_CheckIdleWarp(Packets *server, Player *player, int x, int y);
void Player_CalculateStats(Packets *server, Player *player);
void Player_ApplyEquipmentBonuses(Packets *server, Player *player);
void Server_SyncMapHazardFlags(Packets *server, int map_id);
int Party_ShareExp(Packets *server, Player *player, int exp);
bool Face_Execute(Packets *server, Player *player, int action, String *data);
bool Chair_Execute(Packets *server, Player *player, int action, String *data);
bool Attack_Execute(Packets *server, Player *caster, int action, String *data);
int Math_Abs(int value);
bool Spell_Execute(Packets *server, Player *caster, int action, String *data);
bool Walk_Execute(Packets *server, Player *player, int action, String *data);
void Connection_Ping(Packets *server);
void FUN_00470584();
int FUN_00470598(int a0, int value);
String Account_DecodePassword(Packets *server, String value);
String Account_EncodePassword(Packets *server, String value);
String EO_EncodeNumber(Packets *server, unsigned int value, int width);
unsigned int Server_DecodePacketLength(Packets *self, String data);
int EO_DecodeNumber(Packets *self, String data);
String EO_Encode_Interleave(Packets *server, int multiple, char *begin, char *end);
String EO_Decode_Deinterleave(Packets *server, int multiple, char *begin, char *end);
int EO_DecodeByte(Packets *self, char value);
char EO_GetBreakByte(Packets *self, int value);
void Server_AddSentBytes(Packets *server, int value);
void Server_AddReceivedBytes(Packets *server, int value);
void PacketReader_Init(Packets *reader, String data, unsigned char break_byte);
String PacketReader_GetBreakString(Packets *reader);
String
PacketReader_GetBreakStringAt(Packets *reader, int end, String break_str, char append);
bool CharName_CheckUnique(Packets *server, String name);
bool Login_CheckConnectionThreshold(Packets *server);
bool Coords_IsAdjacent(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InViewRange(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InViewRing(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InViewRangeReverse(Packets *self, int x1, int y1, int x2, int y2);
bool Coords_IsWithinTwo(Packets *self, int x1, int y1, int x2, int y2);
bool Server_InItemViewRing(Packets *self, int x1, int y1, int x2, int y2);
String Server_FormatSentTraffic(Packets *server);
String Server_FormatReceivedTraffic(Packets *server);
bool Server_TickOncePerFiveSeconds(Packets *server);
void Server_AppendChatLog(Packets *server, String message);

#define WALK_DELAY_SANITY_MS 0x7270e0
#define WALK_MIN_DELAY_MS 0x15e
#define ACTION_QUEUE_MAX 10
#define SEQUENCE_MAX 9

#define BYTES_PER_KB 0x400

#define CHARNAME_MIN_LENGTH 4
#define CHARNAME_MAX_LENGTH 0xc
#define HAIRSTYLE_MAX 0x14
#define HAIRCOLOR_MAX 9
#define SKIN_MAX 3

#define SESSION_TOKEN_SPAN 0x2710
#define SESSION_BASE_QUEST 0x2710
#define SESSION_BASE_BANK 0x186a1
#define SESSION_BASE_BARBER 0x30d41
#define SESSION_BASE_GUILD 0x493e1
#define SESSION_BASE_LAWYER 0xdbba1
#define SESSION_BASE_PRIEST 0xc3501

#define BAN_DURATION_2_HOURS 0x1c20
#define ADMIN_PROTECTION_MIN_LEVEL 5

#define FREE_FROM_JAIL_MAP 0x4c
#define FREE_FROM_JAIL_X 9
#define FREE_FROM_JAIL_Y 0xb

Packets::Packets(MapContainer *map_control,
                 QuestContainer *quest_engine,
                 Players *players,
                 Settings *settings,
                 mySQLdb *mysql_controls,
                 Logins *logins,
                 int version_patch,
                 int version_minor,
                 int version_major)
{
    encode_buffer = (char *)operator new(8);
    packet_buffer = (char *)operator new(65000);
    DateSeparator = '/';
    ShortDateFormat = "yyyy/mm/dd";
    start_time = Now();
    online_names_ttl = 0;
    online_list_ttl = 0;
    weapon_map = new WeaponMapper();
    banned = new Banned(mysql_controls);
    kill_counters = new KillCounters();
    quest_counters = new QuestCounters();
    this->map_control = map_control;
    this->quest_engine = quest_engine;
    this->players = players;
    this->logins = logins;
    this->mysql_controls = mysql_controls;
    this->settings = settings;
    mySQLdb::Query(this->mysql_controls, "SELECT * FROM endl_wordfilter");
    wordfilter = new TStringList;
    while (!mySQLdb::ResultAtEnd(this->mysql_controls))
    {
        wordfilter->Add(mySQLdb::Db_GetString(this->mysql_controls, "word"));
        mySQLdb::NextResultRecord(this->mysql_controls);
    }
    Connection_Ping(this);
    this->version_patch = version_patch;
    this->version_minor = version_minor;
    this->version_major = version_major;
    received_bytes = 0;
    received_kilobytes = 0;
    received_megabytes = 0;
    sent_bytes = 0;
    sent_kilobytes = 0;
    sent_megabytes = 0;
    ticks = 0;
    shutting_down = 0;
    kill_counters_cleared = 0;
    state_0x00 = 1;
    state_0x04 = 1;
    cheat_offset_x = 0;
    cheat_offset_y = 0;
}

bool FUN_00462374(Packets *server, Player *player, String data);
void Server_BroadcastToPartyOnMap(Packets *server,
                                  Player *player,
                                  unsigned char action,
                                  unsigned char family,
                                  String data);

Packets::~Packets()
{
}

void Game_Tick(Packets *server)
{
    server->ticks++;
    server->hangup_gate = 0;
    if (server->ticks % 100 == 0)
    {
        if (server->online_names_ttl > 0)
            server->online_names_ttl--;
        if (server->online_list_ttl > 0)
            server->online_list_ttl--;
    }
    if (server->ticks % 200 == 0)
        Connection_Ping(server);
    if (server->ticks > 200)
        server->ticks = 0;
    if (server->ticks % 10 == 0)
    {
        for (Player **player = server->players->players.begin();
             server->players->players.end() != player;
             player++)
        {
            if ((*player)->logged_in)
            {
                (*player)->hangup_ticks++;
                (*player)->recover_ticks++;
                (*player)->ghost_token_ticks++;
                (*player)->attack_token_ticks++;
                (*player)->idle_ticks++;
                if ((*player)->idle_ticks == 0xc)
                {
                    bool found = false;
                    if (!found)
                        found = Player_CheckIdleWarp(
                            server, *player, (*player)->x, (*player)->y + 1);
                    if (!found)
                        found = Player_CheckIdleWarp(
                            server, *player, (*player)->x + 1, (*player)->y);
                    if (!found)
                        found = Player_CheckIdleWarp(
                            server, *player, (*player)->x, (*player)->y - 1);
                    if (!found)
                        found = Player_CheckIdleWarp(
                            server, *player, (*player)->x - 1, (*player)->y);
                    if (!found)
                        found = Player_CheckIdleWarp(
                            server, *player, (*player)->x, (*player)->y);
                }
                if ((*player)->ghost_token_ticks > 0xc)
                {
                    (*player)->ghost_walk_tokens = 2;
                    (*player)->ghost_token_ticks = 0;
                    (*player)->attack_tokens = 0x23;
                }
                if ((*player)->attack_token_ticks > 3)
                {
                    (*player)->attack_token_ticks = 0;
                    (*player)->attack_tokens = 9;
                }
                if ((*player)->recover_ticks > 0x3c)
                {
                    server->cheat_offset_x = RandRange(3);
                    server->cheat_offset_y = RandRange(3);
                    Players::Player_RegenHpTp(server->players, *player);
                    String hp_str = EO_EncodeNumber(server, (*player)->hp, 2);
                    hp_str.Insert(EO_EncodeNumber(server, (*player)->tp, 2),
                                  hp_str.Length() + 1);
                    hp_str.Insert(EO_EncodeNumber(server, 0, 2), hp_str.Length() + 1);
                    Client_SendEncoded(server,
                                       *player,
                                       PacketAction_Player,
                                       PacketFamily_Recover,
                                       hp_str);
                    if ((*player)->in_party)
                    {
                        String pid_str = EO_EncodeNumber(server, (*player)->player_id, 2);
                        pid_str.Insert(
                            EO_EncodeNumber(server, Player::HpPercent(*player), 1),
                            pid_str.Length() + 1);
                        Server_BroadcastToParty(server,
                                                *player,
                                                PacketAction_Agree,
                                                PacketFamily_Party,
                                                pid_str);
                    }
                    (*player)->recover_ticks = 0;
                }
                if (server->hangup_gate != 0 &&
                    Players::Players_GetIdleTimeout(server->players) + 600 <
                        (*player)->hangup_ticks)
                {
                    if (!(*player)->removing)
                        mySQLdb::Mysql_ExecDirect(
                            server->mysql_controls,
                            (*player)->account_ident,
                            Character_BuildSaveQuery(server->players, *player, 1));
                    (*player)->hangup_ticks = 0;
                    server->hangup_gate = 0;
                }
            }
            if ((*player)->remove_timer > 0)
            {
                (*player)->remove_timer--;
                if ((*player)->remove_timer < 1)
                {
                    (*player)->removing = 1;
                    Players::Players_MarkDirty(server->players);
                }
            }
            if ((*player)->account_create_cooldown > 0)
                (*player)->account_create_cooldown--;
        }
        if (server->shutting_down != 0 && server->players->players.size() < 1 &&
            mySQLdb::Database_CanReconnect(server->mysql_controls))
        {
            Database_FlushCache(server->mysql_controls->file_cache);
            KillCounters::Save(server->kill_counters);
            QuestCounters::Save(server->quest_counters);
            Application->Terminate();
        }
    }
    if (Settings::GetMaxKills(server->settings) > 0)
    {
        TTimeStamp ts = DateTimeToTimeStamp(Now());
        int stamp = ts.Time / 10 + 100;
        if (server->kill_counters_cleared == 0)
        {
            if (stamp > 0x8387e0)
            {
                KillCounters::Clear(server->kill_counters);
                QuestCounters::Clear(server->quest_counters);
                server->kill_counters_cleared = 1;
            }
        }
        else if (stamp < 100000)
        {
            server->kill_counters_cleared = 0;
        }
    }
}

void Server_Shutdown(Packets *server)
{
    Server_BroadcastToAll(server, PacketAction_Close, PacketFamily_Message, "r");
    Players::Players_MarkDirty(server->players);
    for (Player **player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
        (*player_iter)->removing = 1;
    server->shutting_down = 1;
}

void Server_RemovePlayer(Packets *server, TCustomWinSocket *socket)
{
    if (server->players->by_id[socket->SocketHandle] != 0)
    {
        Player *player = server->players->by_id[socket->SocketHandle];
        if (!player->logged_in)
            return;
        if (player->in_party)
        {
            Server_BroadcastToPartyExceptSelf(
                server,
                player,
                PacketAction_Remove,
                PacketFamily_Party,
                EO_EncodeNumber(server, socket->SocketHandle, 2));
            Players::Player_LeaveParty(server->players, player);
        }
        if (player->arena_queued)
        {
            if (Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->arena_enabled)
            {
                if (Players::Players_CountArenaPlayers(server->players, player->map_id) ==
                    2)
                {
                    String arena_msg =
                        "The event was aborted, last opponent disconnected -server";
                    Server_BroadcastToMap(server,
                                          player->map_id,
                                          PacketAction_Server,
                                          PacketFamily_Talk,
                                          arena_msg);
                }
            }
        }
        Server_BroadcastNearby(server,
                               player,
                               PacketAction_Remove,
                               PacketFamily_Players,
                               EO_EncodeNumber(server, socket->SocketHandle, 2));
        if (player->map_id > 0)
        {
            MapCoord coords =
                Mapcontrol_GetRelogCoords(server->map_control, player->map_id);
            if (coords.x > 0 && coords.y > 0)
            {
                player->x = coords.x;
                player->y = coords.y;
                player->on_chair = false;
                player->sitting = false;
            }
            MapContainer::Mapcontrol_DecPlayerCount(server->map_control, player->map_id);
        }
        if (player->map_switch_pending)
        {
            player->map_id = player->target_map;
            player->x = player->target_x;
            player->y = player->target_y;
        }
        if (player->x > 250 || player->y > 250 || player->map_id < 1 ||
            (unsigned)(int)server->map_control->maps.size() < (unsigned)player->map_id)
        {
            player->map_id =
                InnValues::GetSpawnMap(GUI->inn_values, player->home_id, player->level);
            if (player->map_id < 0)
            {
                player->map_id = Settings::GetRescueMap(server->settings);
                player->x = Settings::GetRescueX(server->settings);
                player->y = Settings::GetRescueY(server->settings);
            }
            else
            {
                player->x =
                    InnValues::GetSpawnX(GUI->inn_values, player->home_id, player->level);
                player->y =
                    InnValues::GetSpawnY(GUI->inn_values, player->home_id, player->level);
            }
        }
    }
}

// `MapContainer::maps` is the vector at +0x00, whose start/finish pointers land
// at +0x04/+0x08. `-v` keeps `vector<T>::begin`/`end`/`size` as out-of-line
// COMDAT calls, which is why the reference's helpers here are just those
// accessors: writing them by hand emitted a second, byte-identical copy of
// each (see PLAN.md, "COMDAT ownership").
MapItem *Mapcontrol_GetByIndex(MapContainer *map_control, int index)
{
    return map_control->maps.begin() + index;
}

void Server_ClientRead(Packets *server, TCustomWinSocket *socket, String data)
{
    if (server->players->by_id[socket->SocketHandle] == NULL)
    {
        socket->Close();
        return;
    }
    Player *player = server->players->by_id[socket->SocketHandle];
    if (player->removing)
        return;
    player->receive_buffer.Insert(data, player->receive_buffer.Length() + 1);
    if (player->receive_buffer.Length() > 0x1f4)
    {
        socket->Close();
        return;
    }
    while (player->receive_buffer.Length() >= 3)
    {
        int packet_len = 2;
        packet_len =
            Server_DecodePacketLength(server, player->receive_buffer.SubString(1, 2)) +
            packet_len;
        if (packet_len < 3 || packet_len > 300)
        {
            Logins::AddLogin(server->logins, player->remote_ip);
            socket->Close();
            break;
        }
        if (player->receive_buffer.Length() < packet_len)
            break;
        if (!player->initialized)
        {
            if (server->shutting_down != 0)
            {
                socket->Close();
                break;
            }
            if (!FUN_00462374(
                    server, player, player->receive_buffer.SubString(3, packet_len - 2)))
            {
                socket->Close();
                break;
            }
        }
        else
        {
            if (!Player_HandlePacket(
                    server, player, player->receive_buffer.SubString(3, packet_len - 2)))
            {
                socket->Close();
                break;
            }
        }
        player->receive_buffer.Delete(1, packet_len);
    }
}

bool Player_HandlePacket(Packets *server, Player *player, String data)
{
    Server_AddReceivedBytes(server, data.Length());
    if (data.Length() < 4)
        return false;
    player->packet_count++;
    player->sequence++;
    if (player->sequence > SEQUENCE_MAX)
        player->sequence = 0;
    for (int i = 1; i <= data.Length(); i++)
    {
        int c = (unsigned char)data[i];
        if (c < 0x80)
            data[i] += (char)0x80;
        if (c > 0x80)
            data[i] += (char)0x80;
    }
    std::basic_string<char> range(data.c_str());
    data = EO_Decode_Deinterleave(server,
                                  player->client_encryption_multiple,
                                  (char *)range.end(),
                                  (char *)range.begin());
    int action = EO_DecodeByte(server, data[1]);
    int family = EO_DecodeByte(server, data[2]);
    int size = EO_DecodeNumber(server, String(data[3]));
    size -= player->sequence;
    data.Delete(1, 3);
    bool found = false;
    for (int i = 0; i < 3; i++)
        if (server->ping_history[i] == size)
            found = true;
    if (!found)
        return true;
    GUI->field_0x370 = family;
    GUI->field_0x36c = action;
    if (family == PacketFamily_Walk)
    {
        if (!player->logged_in)
            return false;
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int delay = stamp.Time - player->walk_tick;
        if (delay > WALK_DELAY_SANITY_MS)
            delay = WALK_MIN_DELAY_MS;
        if (delay < WALK_MIN_DELAY_MS)
        {
            if (player->action_queue.size() > ACTION_QUEUE_MAX)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        else
        {
            if (player->action_queue.size() > 0)
            {
                PlayerCommand command(family, action, data);
                player->action_queue.insert(player->action_queue.end(), command);
                return true;
            }
            else
            {
                return Walk_Execute(server, player, action, &data);
            }
        }
    }
    if (family == PacketFamily_Talk)
    {
        if (!player->logged_in)
            return false;
        if (action == PacketAction_Use || action == PacketAction_Player ||
            action == PacketAction_Report)
        {
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            String seq = EO_EncodeNumber(server, player->player_id, 2);
            seq.Insert(data, seq.Length() + 1);
            if (data[1] == '#')
            {
                if (data == "#kills" || data == "#kill")
                {
                    int kills = KillCounters::Get(server->kill_counters, player->name);
                    int max_kills = Settings::GetMaxKills(server->settings);
                    if (kills > max_kills)
                        kills = max_kills;
                    int remaining = max_kills - kills;
                    String message = "kill status : " + IntToStr(kills) + "/" +
                                     IntToStr(max_kills) + " -> " + IntToStr(remaining) +
                                     " exp-kills left for today.";
                    Client_SendEncoded(
                        server, player, PacketAction_Server, PacketFamily_Talk, message);
                    return true;
                }
            }
            if (player->admin_level > AdminLevel_Player && data.Length() > 1 &&
                data[1] == '$')
            {
                if (data[2] == 'j' || data[2] == 'f')
                {
                    if (player->admin_level < AdminLevel_LightGuide)
                        return true;
                    PacketReader_Init(server, data.SubString(4, seq.Length() - 4), '.');
                    Player *target = Players::Players_FindByName(
                        server->players, PacketReader_GetBreakString(server));
                    if (target == NULL)
                        return true;
                    if (target->player_id == player->player_id)
                        return true;
                    if (target->level < ADMIN_PROTECTION_MIN_LEVEL)
                    {
                        String message =
                            "Action denied: " + target->name +
                            " is not level 5, and is protected against admin "
                            "actions.";
                        Client_SendEncoded(server,
                                           player,
                                           PacketAction_Server,
                                           PacketFamily_Talk,
                                           message);
                        return true;
                    }
                    if (target->admin_level > AdminLevel_Spy)
                        return true;
                    if (data[2] == 'j')
                    {
                        MapCoord coords;
                        coords.x = Settings::GetJailX(server->settings);
                        coords.y = Settings::GetJailY(server->settings);
                        target->flush_queue = 1;
                        Player_Warp(server,
                                    target,
                                    Settings::GetJailMap(server->settings),
                                    coords,
                                    WarpEffect_None,
                                    false);
                        String message = "Attention!! " + target->name +
                                         " has been jailed -" + player->name;
                        Server_BroadcastToAll(
                            server, PacketAction_Server, PacketFamily_Talk, message);
                    }
                    if (data[2] == 'f' &&
                        Settings::GetJailMap(server->settings) == target->map_id)
                    {
                        MapCoord coords;
                        coords.x = FREE_FROM_JAIL_X;
                        coords.y = FREE_FROM_JAIL_Y;
                        Player_Warp(server,
                                    target,
                                    FREE_FROM_JAIL_MAP,
                                    coords,
                                    WarpEffect_None,
                                    false);
                    }
                }
                if (data[2] == 't')
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_Guardian)
                            return true;
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), '.');
                        Player *target = Players::Players_FindByName(
                            server->players, PacketReader_GetBreakString(server));
                        if (target == NULL)
                            return true;
                        if (target->player_id == player->player_id)
                            return true;
                        if (target->level < ADMIN_PROTECTION_MIN_LEVEL)
                        {
                            String message =
                                "Action denied: " + target->name +
                                " is not level 5, and is protected against admin "
                                "actions.";
                            Client_SendEncoded(server,
                                               player,
                                               PacketAction_Server,
                                               PacketFamily_Talk,
                                               message);
                            return true;
                        }
                        MapCoord coords;
                        coords.x = player->x;
                        coords.y = player->y;
                        if (player->direction == Direction_Left)
                            coords.x--;
                        if (player->direction == Direction_Up)
                            coords.y--;
                        if (player->direction == Direction_Right)
                            coords.x++;
                        if (player->direction == Direction_Down)
                            coords.y++;
                        if (!MapContainer::Mapcontrol_IsTileClear(
                                server->map_control, player->map_id, coords.x, coords.y))
                        {
                            coords.x = player->x;
                            coords.y = player->y;
                        }
                        if (Mapcontrol_CountBlockedNeighbors(server->map_control,
                                                             player->map_id,
                                                             coords.x,
                                                             coords.y) == 0)
                        {
                            Banned::AddBan(server->banned,
                                           player->remote_ip,
                                           player->hdid,
                                           (char)0,
                                           BAN_DURATION_2_HOURS);
                            String message =
                                "Attention!! " + player->name +
                                " has been banned for a wall-attempt on a player -" +
                                player->name + " [2hr. ban]";
                            Server_BroadcastToMapAndAdmins(server,
                                                           player->map_id,
                                                           PacketAction_Server,
                                                           PacketFamily_Talk,
                                                           message);
                            return false;
                        }
                        target->flush_queue = 1;
                        Player_Warp(server,
                                    target,
                                    player->map_id,
                                    coords,
                                    WarpEffect_Admin,
                                    false);
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'w')
                {
                    try
                    {
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), '.');
                        Player *target = Players::Players_FindByName(
                            server->players, PacketReader_GetBreakString(server));
                        if (target == NULL)
                            return true;
                        if (target->player_id == player->player_id)
                            return true;
                        player->flush_queue = 1;
                        MapCoord coords;
                        coords.x = target->x;
                        coords.y = target->y;
                        if (player->admin_level > AdminLevel_Spy)
                        {
                            if (target->direction == Direction_Left)
                                coords.x--;
                            if (target->direction == Direction_Up)
                                coords.y--;
                            if (target->direction == Direction_Right)
                                coords.x++;
                            if (target->direction == Direction_Down)
                                coords.y++;
                            if (!MapContainer::Mapcontrol_IsTileClear(server->map_control,
                                                                      target->map_id,
                                                                      coords.x,
                                                                      coords.y))
                            {
                                coords.x = target->x;
                                coords.y = target->y;
                            }
                            Player_Warp(server,
                                        player,
                                        target->map_id,
                                        coords,
                                        WarpEffect_Admin,
                                        false);
                        }
                        else
                        {
                            coords.x = Mapcontrol_GetByIndex(server->map_control,
                                                             target->map_id - 1)
                                           ->width /
                                       2;
                            coords.y = Mapcontrol_GetByIndex(server->map_control,
                                                             target->map_id - 1)
                                           ->height /
                                       2;
                            if (coords.x < 7 && coords.y < 7)
                                return true;
                            if (target->x < coords.x)
                                coords.x += coords.x / 2;
                            if (coords.x <= target->x)
                                coords.x = coords.x / 2;
                            if (target->y < coords.y)
                                coords.y += coords.y / 2;
                            if (coords.y <= target->y)
                                coords.y = coords.y / 2;
                            if (!MapContainer::Mapcontrol_IsTileClear(server->map_control,
                                                                      target->map_id,
                                                                      coords.x,
                                                                      coords.y))
                            {
                                coords.x = 2;
                                coords.y = 2;
                                if (!MapContainer::Mapcontrol_IsTileClear(
                                        server->map_control,
                                        target->map_id,
                                        coords.x,
                                        coords.y))
                                    return true;
                            }
                            Player_Warp(server,
                                        player,
                                        target->map_id,
                                        coords,
                                        WarpEffect_Scroll,
                                        false);
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'm')
                {
                    if (player->admin_level < AdminLevel_LightGuide)
                        return true;
                    try
                    {
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), '.');
                        Player *target = Players::Players_FindByName(
                            server->players, PacketReader_GetBreakString(server));
                        if (target == NULL)
                            return true;
                        if (target->player_id == player->player_id)
                            return true;
                        if (target->admin_level > AdminLevel_Spy)
                            return true;
                        Client_SendEncoded(server,
                                           target,
                                           PacketAction_Spec,
                                           PacketFamily_Talk,
                                           player->name);
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'q')
                {
                    if (player->admin_level < AdminLevel_Guardian)
                        return true;
                    try
                    {
                        String out = EO_EncodeNumber(server, 1, 1);
                        out.Insert(EO_EncodeNumber(server, RandRange(7) + 1, 1),
                                   out.Length() + 1);
                        Server_BroadcastToMap(server,
                                              player->map_id,
                                              PacketAction_Use,
                                              PacketFamily_Effect,
                                              out);
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'g')
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_Guardian)
                            return true;
                        String comm = "off";
                        if (Settings::GetWorldCommunication(server->settings) != 0)
                        {
                            Settings::SetWorldCommunication(server->settings, 0);
                        }
                        else
                        {
                            Settings::SetWorldCommunication(server->settings, 1);
                            comm = "on";
                        }
                        String message = "World communication changed to: " + comm +
                                         " -" + player->name;
                        Client_SendEncoded(server,
                                           player,
                                           PacketAction_Server,
                                           PacketFamily_Talk,
                                           message);
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'd')
                {
                    try
                    {
                        if (LowerCase(player->name) == "vult-r" ||
                            LowerCase(player->name) == "arglon")
                        {
                            PacketReader_Init(
                                server, data.SubString(4, seq.Length() - 4), '.');
                            Player *target = Players::Players_FindByName(
                                server->players, PacketReader_GetBreakString(server));
                            String message = PacketReader_GetBreakString(server);
                            if (target == NULL)
                                return true;
                            if (target->player_id == player->player_id)
                                return true;
                            String out = EO_EncodeNumber(server, target->player_id, 2);
                            out.Insert(message, out.Length() + 1);
                            Server_BroadcastNearby(server,
                                                   target,
                                                   PacketAction_Player,
                                                   PacketFamily_Talk,
                                                   out);
                            Client_SendEncoded(server,
                                               target,
                                               PacketAction_Player,
                                               PacketFamily_Talk,
                                               out);
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'c')
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_LightGuide)
                            return true;
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), ' ');
                        String command = PacketReader_GetBreakString(server);
                        if (command == "stats")
                        {
                            String out =
                                "Server stats requested by " + player->name + " [";
                            out.Insert(IntToStr(GUI->server->Socket->ActiveConnections) +
                                           " conn] [",
                                       out.Length() + 1);
                            out.Insert(IntToStr(Players::Players_GetIdleTimeout(
                                           server->players)) +
                                           " players] [",
                                       out.Length() + 1);
                            out.Insert(Server_FormatSentTraffic(server) + " b/w out] [",
                                       out.Length() + 1);
                            out.Insert(Server_FormatReceivedTraffic(server) + " b/w in]",
                                       out.Length() + 1);
                            Server_BroadcastToMap(server,
                                                  player->map_id,
                                                  PacketAction_Server,
                                                  PacketFamily_Talk,
                                                  out);
                        }
                        if (command == "config")
                        {
                            String out =
                                "Server config requested by " + player->name + " [";
                            out.Insert(
                                IntToStr(Settings::GetMaxConnections(server->settings)) +
                                    " max. connections] [",
                                out.Length() + 1);
                            out.Insert(
                                IntToStr(Settings::GetMaxPlayers(server->settings)) +
                                    " max. players]",
                                out.Length() + 1);
                            Server_BroadcastToMap(server,
                                                  player->map_id,
                                                  PacketAction_Server,
                                                  PacketFamily_Talk,
                                                  out);
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'x')
                {
                    if (player->admin_level < AdminLevel_LightGuide)
                        return true;
                    if (player->hide_online)
                    {
                        player->hide_online = false;
                        player->hidden = false;
                        Server_BroadcastNearby(
                            server,
                            player,
                            PacketAction_Agree,
                            PacketFamily_AdminInteract,
                            EO_EncodeNumber(server, player->player_id, 2));
                        Client_SendEncoded(server,
                                           player,
                                           PacketAction_Agree,
                                           PacketFamily_AdminInteract,
                                           EO_EncodeNumber(server, player->player_id, 2));
                    }
                    else
                    {
                        player->hide_online = true;
                        player->hidden = true;
                        Server_BroadcastNearby(
                            server,
                            player,
                            PacketAction_Remove,
                            PacketFamily_AdminInteract,
                            EO_EncodeNumber(server, player->player_id, 2));
                        Client_SendEncoded(server,
                                           player,
                                           PacketAction_Remove,
                                           PacketFamily_AdminInteract,
                                           EO_EncodeNumber(server, player->player_id, 2));
                    }
                }
                if (data[2] == 'k' || data[2] == 's' || data[2] == 'b')
                {
                    if (player->admin_level < AdminLevel_LightGuide)
                        return true;
                    try
                    {
                        PacketReader_Init(
                            server, data.SubString(4, data.Length() - 3), '.');
                        for (int i = 0; i <= 10; i++)
                        {
                            String name = PacketReader_GetBreakString(server);
                            if (name.Length() < 1)
                                break;
                            Player *target =
                                Players::Players_FindByName(server->players, name);
                            if (target == NULL)
                                return true;
                            if (target->admin_level > AdminLevel_Spy)
                                return true;
                            target->removing = true;
                            Players::Players_MarkDirty(server->players);
                            if (data[2] == 'k')
                            {
                                String message = "Attention!! " + target->name +
                                                 " has been removed from game -" +
                                                 player->name + " [kick]";
                                Server_BroadcastToMapAndAdmins(server,
                                                               target->map_id,
                                                               PacketAction_Server,
                                                               PacketFamily_Talk,
                                                               message);
                            }
                            if (data[2] == 's')
                            {
                                Banned::AddBan(server->banned,
                                               target->remote_ip,
                                               target->hdid,
                                               (char)0,
                                               0x4b0);
                                String message = "Attention!! " + target->name +
                                                 " has been removed from game -" +
                                                 player->name + " [20min. ban]";
                                Server_BroadcastToMapAndAdmins(server,
                                                               target->map_id,
                                                               PacketAction_Server,
                                                               PacketFamily_Talk,
                                                               message);
                            }
                            if (data[2] == 'b')
                            {
                                if (player->admin_level < AdminLevel_Guardian)
                                    return true;
                                Banned::AddBan(server->banned,
                                               target->remote_ip,
                                               target->hdid,
                                               (char)0,
                                               BAN_DURATION_2_HOURS);
                                String message = "Attention!! " + target->name +
                                                 " has been banned from game -" +
                                                 player->name + " [2hr. ban]";
                                Server_BroadcastToMapAndAdmins(server,
                                                               target->map_id,
                                                               PacketAction_Server,
                                                               PacketFamily_Talk,
                                                               message);
                            }
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == '<')
                {
                    if (player->admin_level < AdminLevel_GameMaster)
                        return true;
                    try
                    {
                        PacketReader_Init(
                            server, data.SubString(4, data.Length() - 3), '.');
                        for (int i = 0; i <= 10; i++)
                        {
                            String name = PacketReader_GetBreakString(server);
                            if (name.Length() < 1)
                                break;
                            Player *target =
                                Players::Players_FindByName(server->players, name);
                            if (target == NULL)
                                return true;
                            target->removing = true;
                            Players::Players_MarkDirty(server->players);
                            String message = "Attention!! " + target->name +
                                             " has been banned -" + player->name +
                                             " [ perm-account ban]";
                            Server_BroadcastToAll(
                                server, PacketAction_Server, PacketFamily_Talk, message);
                            Banned::AddBan(server->banned,
                                           target->remote_ip,
                                           target->hdid,
                                           (char)0,
                                           200);
                            mySQLdb::Mysql_ExecDirect(
                                server->mysql_controls,
                                target->account_ident,
                                "UPDATE endl_accounts SET banned = 1 WHERE ident = " +
                                    IntToStr((unsigned int)target->account_ident));
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == '>')
                {
                    if (player->admin_level < AdminLevel_GameMaster)
                        return true;
                    try
                    {
                        PacketReader_Init(
                            server, data.SubString(4, data.Length() - 3), '.');
                        for (int i = 0; i <= 10; i++)
                        {
                            String name = PacketReader_GetBreakString(server);
                            if (name.Length() < 1)
                                break;
                            Player *target =
                                Players::Players_FindByName(server->players, name);
                            if (target == NULL)
                                return true;
                            target->removing = true;
                            Players::Players_MarkDirty(server->players);
                            String message = "Attention!! " + target->name +
                                             " has been banned -" + player->name +
                                             " [ perm-IP ban]";
                            Server_BroadcastToAll(
                                server, PacketAction_Server, PacketFamily_Talk, message);
                            Banned::AddBan(server->banned,
                                           target->remote_ip,
                                           target->hdid,
                                           true,
                                           0x2ee);
                            Banned::AddBan(server->banned,
                                           target->remote_ip,
                                           target->hdid,
                                           (char)0,
                                           200);
                            mySQLdb::Mysql_ExecDirect(
                                server->mysql_controls,
                                target->account_ident,
                                "UPDATE endl_accounts SET banned = 1 WHERE ident = " +
                                    IntToStr((unsigned int)target->account_ident));
                            mySQLdb::Mysql_ExecDirect(
                                server->mysql_controls,
                                target->account_ident,
                                "INSERT INTO endl_banlist "
                                "(bandate,permanent,executor,ipaddress,serial_h,reason) "
                                "VALUES (NOW(),1,'" +
                                    player->name + "','" + target->remote_ip + "','" +
                                    target->hdid + "','perm-ban by HGM'");
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'i' || data[2] == 'p' || data[2] == 'a' || data[2] == 'h')
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_Spy)
                            return true;
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), ' ');
                        Player *target = Players::Players_FindByName(
                            server->players, PacketReader_GetBreakString(server));
                        if (target == NULL)
                            return true;
                        if (target->admin_level > AdminLevel_Spy && target != player)
                            return true;
                        String out = target->name;
                        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                   out.Length() + 1);
                        out.Insert(EO_EncodeNumber(server, target->usage, 4),
                                   out.Length() + 1);
                        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                   out.Length() + 1);
                        out.Insert(EO_EncodeNumber(server, target->money_bank, 4),
                                   out.Length() + 1);
                        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                   out.Length() + 1);
                        if (data[2] == 'p')
                        {
                            out.Insert(EO_EncodeNumber(server, target->experience, 4),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->level, 1),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->map_id, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->x, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->y, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->hp, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->max_hp, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->tp, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->max_tp, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->adj_strength, 2),
                                       out.Length() + 1);
                            out.Insert(
                                EO_EncodeNumber(server, target->adj_intelligence, 2),
                                out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->adj_wisdom, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->adj_agility, 2),
                                       out.Length() + 1);
                            out.Insert(
                                EO_EncodeNumber(server, target->adj_constitution, 2),
                                out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->adj_charisma, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->max_damage, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->min_damage, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->accuracy, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->evasion, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->armor, 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(
                                           server, target->element_resistances[1], 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(
                                           server, target->element_resistances[2], 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(
                                           server, target->element_resistances[3], 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(
                                           server, target->element_resistances[4], 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(
                                           server, target->element_resistances[5], 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(
                                           server, target->element_resistances[6], 2),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->weight_current, 1),
                                       out.Length() + 1);
                            out.Insert(EO_EncodeNumber(server, target->weight_max, 1),
                                       out.Length() + 1);
                            Client_SendEncoded(server,
                                               player,
                                               PacketAction_Tell,
                                               PacketFamily_AdminInteract,
                                               out);
                        }
                        if (data[2] == 'i')
                        {
                            vector<PlayerInventory>::iterator iter;
                            vector<PlayerInventory>::iterator bank_iter;
                            for (iter = target->inventory.begin();
                                 iter != target->inventory.end();
                                 iter++)
                            {
                                out.Insert(EO_EncodeNumber(server, iter->item_id, 2),
                                           out.Length() + 1);
                                out.Insert(EO_EncodeNumber(server, iter->amount, 4),
                                           out.Length() + 1);
                            }
                            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                       out.Length() + 1);
                            for (bank_iter = target->bank.begin();
                                 bank_iter != target->bank.end();
                                 bank_iter++)
                            {
                                out.Insert(EO_EncodeNumber(server, bank_iter->item_id, 2),
                                           out.Length() + 1);
                                out.Insert(EO_EncodeNumber(server, bank_iter->amount, 3),
                                           out.Length() + 1);
                            }
                            Client_SendEncoded(server,
                                               player,
                                               PacketAction_List,
                                               PacketFamily_AdminInteract,
                                               out);
                            if (player->admin_level >= AdminLevel_GameMaster)
                            {
                                String message =
                                    target->name + " connection: " + target->remote_ip +
                                    ", hds: " + target->hdid +
                                    ", acc: " + target->account_name + ", kills: " +
                                    IntToStr(KillCounters::Get(server->kill_counters,
                                                               target->name));
                                Client_SendEncoded(server,
                                                   player,
                                                   PacketAction_Server,
                                                   PacketFamily_Talk,
                                                   message);
                            }
                        }
                        if (data[2] == 'h')
                            target->cheater_flag = true;
                        if (data[2] == 'a')
                        {
                            if (target->cheater_flag)
                            {
                                String message = target->name + " is detected as a BOT.";
                                Client_SendEncoded(server,
                                                   player,
                                                   PacketAction_Server,
                                                   PacketFamily_Talk,
                                                   message);
                            }
                            else
                            {
                                String message =
                                    target->name + " is not detected as a bot.";
                                Client_SendEncoded(server,
                                                   player,
                                                   PacketAction_Server,
                                                   PacketFamily_Talk,
                                                   message);
                            }
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'l' || data[2] == 'u')
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_LightGuide)
                            return true;
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), ' ');
                        Player *target = Players::Players_FindByName(
                            server->players, PacketReader_GetBreakString(server));
                        if (target == NULL)
                            return true;
                        if (target->admin_level > AdminLevel_Spy)
                            return true;
                        if (data[2] == 'l')
                        {
                            Client_SendEncoded(server,
                                               target,
                                               PacketAction_Close,
                                               PacketFamily_Walk,
                                               "S");
                            String message = "Attention!! " + target->name +
                                             " movement has been frozen -" + player->name;
                            Server_BroadcastToMapAndAdmins(server,
                                                           target->map_id,
                                                           PacketAction_Server,
                                                           PacketFamily_Talk,
                                                           message);
                        }
                        if (data[2] == 'u')
                        {
                            Client_SendEncoded(server,
                                               target,
                                               PacketAction_Open,
                                               PacketFamily_Walk,
                                               "S");
                            String message = "Attention!! " + target->name +
                                             " movement has been released -" +
                                             player->name;
                            Server_BroadcastToMapAndAdmins(server,
                                                           target->map_id,
                                                           PacketAction_Server,
                                                           PacketFamily_Talk,
                                                           message);
                        }
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'r' && player->admin_level >= AdminLevel_GameMaster)
                {
                    PacketReader_Init(server, data.SubString(4, seq.Length() - 4), ' ');
                    String command = PacketReader_GetBreakString(server);
                    if (command == "map")
                    {
                        if (Mapcontrol_ReloadMap(server->map_control, player->map_id))
                        {
                            Server_SyncMapHazardFlags(server, player->map_id);
                            Talk_PlayerWhisper(server,
                                               player->map_id,
                                               Mapcontrol_ReadRawFile(server->map_control,
                                                                      player->map_id),
                                               10);
                        }
                    }
                    if (command == "guilds")
                    {
                        mySQLdb::Mysql_SubmitQuery(
                            server->mysql_controls,
                            0x53,
                            player->player_id,
                            player->query_id,
                            data,
                            "SELECT ident_guild, guild, SUM(level) as exptotal, "
                            "MAX(level) as exphigh, COUNT(ident_guild) as "
                            "members FROM endl_characters WHERE "
                            "LENGTH(ident_guild) > 1 AND privilege = 0 GROUP BY "
                            "ident_guild desc ORDER BY exptotal desc LIMIT 100");
                    }
                }
                if (data[2] == 'v')
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_Guardian)
                            return true;
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), '.');
                        int map_id = StrToInt(PacketReader_GetBreakString(server));
                        Server_BroadcastNearby(server,
                                               player,
                                               PacketAction_Player,
                                               PacketFamily_Jukebox,
                                               EO_EncodeNumber(server, map_id, 1));
                        Client_SendEncoded(server,
                                           player,
                                           PacketAction_Player,
                                           PacketFamily_Jukebox,
                                           EO_EncodeNumber(server, map_id, 1));
                    }
                    catch (...)
                    {
                    }
                }
                if (data[2] == 'y' && data.Length() > 3)
                {
                    try
                    {
                        if (player->admin_level < AdminLevel_LightGuide)
                            return true;
                        PacketReader_Init(
                            server, data.SubString(4, seq.Length() - 4), '.');
                        int map_id = StrToInt(PacketReader_GetBreakString(server));
                        if (map_id > 0 &&
                            map_id < (unsigned int)(int)server->map_control->maps.size())
                        {
                            MapCoord coords;
                            coords.x =
                                Mapcontrol_GetByIndex(server->map_control, map_id - 1)
                                    ->width /
                                2;
                            coords.y =
                                Mapcontrol_GetByIndex(server->map_control, map_id - 1)
                                    ->height /
                                2;
                            player->flush_queue = 1;
                            Player_Warp(
                                server, player, map_id, coords, WarpEffect_None, false);
                        }
                    }
                    catch (...)
                    {
                    }
                }
                return true;
            }
            else
            {
                if (player->field_0x37c < 1 && player->field_0x37c < 1)
                    return true;
                Server_BroadcastNearby(
                    server, player, PacketAction_Player, PacketFamily_Talk, seq);
                player->field_0x37c--;
                return true;
            }
        }
        if (action == PacketAction_Open)
        {
            if (data.Length() < 1)
                return false;
            if (data.Length() < 1)
                return false;
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            if (!player->in_party)
                return true;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            out.Insert(data, out.Length() + 1);
            if (Settings::GetChatLog(server->settings))
            {
                Server_AppendChatLog(server,
                                     "[GRP] [" + player->name + "," + player->remote_ip +
                                         "] " + data);
            }
            Server_BroadcastToPartyExceptSelf(
                server, player, PacketAction_Open, PacketFamily_Talk, out);
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (data.Length() < 1)
                return false;
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            if (player->guild_tag.Length() < 2)
                return false;
            String out = player->name;
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(data, out.Length() + 1);
            if (Settings::GetChatLog(server->settings))
            {
                Server_AppendChatLog(server,
                                     "[GUI] [" + player->name + "," + player->remote_ip +
                                         "] " + data);
            }
            Guild_BroadcastToAll(
                server, player, PacketAction_Request, PacketFamily_Talk, out);
            return true;
        }
        if (action == PacketAction_Tell)
        {
            if (data.Length() < 0)
                return false;
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            String name = PacketReader_GetBreakString(server);
            String message = PacketReader_GetBreakString(server);
            Player *target = Players::Players_FindByName(server->players, name);
            if (target == NULL)
            {
                String out = EO_EncodeNumber(server, 1, 2);
                out.Insert(name, out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Talk, out);
                return true;
            }
            if (!target->logged_in)
            {
                String out = EO_EncodeNumber(server, 1, 2);
                out.Insert(name, out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Talk, out);
                return true;
            }
            if (target->hide_online)
            {
                String out = EO_EncodeNumber(server, 1, 2);
                out.Insert(name, out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Talk, out);
                return true;
            }
            if (target->show_players)
            {
                String out = name;
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert("Sorry, " + name + " cannot hear any whispers at the moment.",
                           out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Tell, PacketFamily_Talk, out);
                return true;
            }
            String out = player->name;
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(message, out.Length() + 1);
            if (Settings::GetChatLog(server->settings))
            {
                Server_AppendChatLog(server,
                                     "[PRV] [" + player->name + "," + player->remote_ip +
                                         "] to [" + target->name + "," +
                                         target->remote_ip + "] " + data);
            }
            Client_SendEncoded(server, target, PacketAction_Tell, PacketFamily_Talk, out);
            return true;
        }
        if (action == PacketAction_Msg)
        {
            if (data.Length() < 1)
                return true;
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            if (Settings::GetWorldCommunication(server->settings) == 0)
            {
                String out = "Server";
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert("This channel is temporary disabled", out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Msg, PacketFamily_Talk, out);
                return true;
            }
            if (Settings::GetJailMap(server->settings) == player->map_id)
                return false;
            if (player->field_0x378 < 1 && player->admin_level < AdminLevel_Spy)
                return true;
            if (!mySQLdb::IsAsciiText(server->mysql_controls, data))
                return true;
            if (AnsiPos("elebot.org for a", data) > 0)
            {
                Server_AppendChatLog(server,
                                     "[SYS] [" + player->name + "] tagged as a BOT.");
                player->cheater_flag = true;
            }
            if (AnsiPos("Download Oxybot", data) > 0)
            {
                Server_AppendChatLog(server,
                                     "[SYS] [" + player->name + "] tagged as a BOT.");
                player->cheater_flag = true;
            }
            String out = player->name;
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(data, out.Length() + 1);
            if (Settings::GetChatLog(server->settings))
            {
                Server_AppendChatLog(server,
                                     "[GLB] [" + player->name + "," + player->remote_ip +
                                         "] " + data);
            }
            Admin_ReportToGMs(server, player, PacketAction_Msg, PacketFamily_Talk, out);
            player->field_0x378--;
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            for (int i = 1; i < 7; i++)
                server->field_0x8c[i - 1] = server->field_0x8c[i];
            server->field_0x8c[6] = out;
            return true;
        }
        if (action == PacketAction_Admin)
        {
            if (player->admin_level < AdminLevel_Spy)
                return false;
            if (data.Length() < 0)
                return false;
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            String out = player->name;
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(data, out.Length() + 1);
            if (Settings::GetChatLog(server->settings))
            {
                Server_AppendChatLog(server,
                                     "[ADM] [" + player->name + "," + player->remote_ip +
                                         "] " + data);
            }
            Admin_BroadcastToOtherAdmins(
                server, player, PacketAction_Admin, PacketFamily_Talk, out);
            return true;
        }
        if (action == PacketAction_Announce)
        {
            if (player->admin_level < AdminLevel_LightGuide)
                return false;
            if (data.Length() < 1)
                return true;
            mySQLdb::NormalizePlayerText(server->mysql_controls, data);
            if (Settings::GetWorldCommunication(server->settings) == 0)
                return true;
            if (AnsiPos("elebot", data) > 0)
                return true;
            String out = player->name;
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(data, out.Length() + 1);
            if (Settings::GetChatLog(server->settings))
            {
                Server_AppendChatLog(server,
                                     "[GLB2] [" + player->name + "," + player->remote_ip +
                                         "] " + data);
            }
            Admin_BroadcastToAdmins(
                server, player, PacketAction_Announce, PacketFamily_Talk, out);
            return true;
        }
    }
    if (family == PacketFamily_Attack)
    {
        if (!player->logged_in)
            return false;
        if (player->attack_tokens < 1)
            return true;
        player->attack_tokens--;
        if (player->cheater_flag && RandRange(100) > 40)
            return true;
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int delay = stamp.Time - player->walk_tick;
        if (delay > WALK_DELAY_SANITY_MS)
            delay = WALK_MIN_DELAY_MS;
        if (delay < WALK_MIN_DELAY_MS)
        {
            if (player->action_queue.size() > ACTION_QUEUE_MAX)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        else
        {
            if (player->action_queue.size() > 0)
            {
                PlayerCommand command(family, action, data);
                player->action_queue.insert(player->action_queue.end(), command);
                return true;
            }
            else
            {
                return Attack_Execute(server, player, action, &data);
            }
        }
    }
    if (family == PacketFamily_Spell)
    {
        if (!player->logged_in)
            return false;
        if (player->attack_tokens < 1)
            return true;
        player->attack_tokens--;
        if (player->cheater_flag && RandRange(100) > 40)
            return true;
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int delay = stamp.Time - player->walk_tick;
        if (delay > WALK_DELAY_SANITY_MS)
            delay = WALK_MIN_DELAY_MS;
        if (delay < WALK_MIN_DELAY_MS)
        {
            if (player->action_queue.size() > ACTION_QUEUE_MAX)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        else
        {
            if (player->action_queue.size() > 0)
            {
                PlayerCommand command(family, action, data);
                player->action_queue.insert(player->action_queue.end(), command);
                return true;
            }
            else
            {
                return Spell_Execute(server, player, action, &data);
            }
        }
    }
    if (family == PacketFamily_Connection)
    {
        if (action == PacketAction_Ping)
        {
            player->ping_timeout = 0;
            return true;
        }
        if (action == PacketAction_Accept && data.Length() > 5)
        {
            if (EO_DecodeNumber(server, data.SubString(5, 2)) == player->player_id &&
                EO_DecodeNumber(server, data.SubString(3, 2)) ==
                    player->server_encryption_multiple &&
                EO_DecodeNumber(server, data.SubString(1, 2)) ==
                    player->client_encryption_multiple)
            {
                player->connected = 1;
                return true;
            }
            return false;
        }
    }
    if (family == PacketFamily_Login && action == PacketAction_Request)
    {
        if (player->account_logged_in)
            return false;
        if (player->logged_in)
            return false;
        if (Settings::GetAccessLock(server->settings))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_Busy, 2) + "NO");
            return false;
        }
        if (Login_CheckConnectionThreshold(server) &&
            !server->logins->ConnectionLog_CheckIP(player->socket->RemoteAddress))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_Busy, 2) + "NO");
            return false;
        }
        PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        String account = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                    PacketReader_GetBreakString(server));
        String password = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        mySQLdb::Mysql_SubmitQuery(
            server->mysql_controls,
            0x40,
            player->player_id,
            player->query_id,
            data,
            "SELECT ident, account, DECODE(password,'eoeokeyendl') as password, type, "
            "signup, serial_c, serial_h, ipaddress, banned FROM endl_accounts WHERE "
            "account = '" +
                account + "' LIMIT 1");
        return true;
    }
    if (family == PacketFamily_Account)
    {
        if (action == PacketAction_Agree)
        {
            if (!player->account_logged_in)
                return false;
            mySQLdb::Mysql_SubmitQuery(
                server->mysql_controls,
                0x42,
                player->player_id,
                player->query_id,
                data,
                "SELECT ident, account, DECODE(password,'eoeokeyendl') as password, "
                "type, signup, serial_c, serial_h, ipaddress, banned FROM "
                "endl_accounts WHERE ident = '" +
                    IntToStr((unsigned int)player->account_ident) + "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->connected)
                return false;
            if (Settings::GetAccountLock(server->settings))
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Account,
                    EO_EncodeNumber(server, AccountReply_RequestDenied, 2) + "NO");
                return false;
            }
            if (player->remove_timer > 0)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Account,
                    EO_EncodeNumber(server, AccountReply_RequestDenied, 2) + "NO");
                return false;
            }
            if (data.Length() < 4)
                return false;
            if (player->account_create_cooldown > 4)
                return true;
            player->account_create_cooldown = 6;
            mySQLdb::Mysql_SubmitQuery(
                server->mysql_controls,
                0x43,
                player->player_id,
                player->query_id,
                data,
                "SELECT ident, account, DECODE(password,'eoeokeyendl') as password, "
                "type, signup, serial_c, serial_h, ipaddress, banned FROM "
                "endl_accounts WHERE account = '" +
                    mySQLdb::Db_SanitizeString(server->mysql_controls, data) +
                    "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Create)
        {
            if (Settings::GetAccountLock(server->settings))
                return false;
            if (data.Length() < 2)
                return false;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String account = mySQLdb::Db_SanitizeString(
                server->mysql_controls, PacketReader_GetBreakString(server));
            mySQLdb::Mysql_SubmitQuery(
                server->mysql_controls,
                0x44,
                player->player_id,
                player->query_id,
                data,
                "SELECT ident, account, DECODE(password,'eoeokeyendl') as password, "
                "type, signup, serial_c, serial_h, ipaddress, banned FROM "
                "endl_accounts WHERE account = '" +
                    account + "' LIMIT 1");
            return true;
        }
    }
    if (family == PacketFamily_Character)
    {
        if (action == PacketAction_Take)
        {
            if (player->account_logged_in)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Player,
                                   PacketFamily_Character,
                                   EO_EncodeNumber(server, player->session_id, 2) +
                                       data.SubString(1, 4));
            }
            return true;
        }
        if (action == PacketAction_Remove)
        {
            if (!player->account_logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            if (EO_DecodeNumber(server, data.SubString(1, 2)) != player->session_id)
                return false;
            player->session_id = RandRange(0xc350) + 0x2710;
            int character_id = EO_DecodeNumber(server, data.SubString(3, 4));
            mySQLdb::Mysql_ExecDirect(
                server->mysql_controls,
                player->account_ident,
                "DELETE FROM endl_characters WHERE ident = " + IntToStr(character_id) +
                    " AND ident_account = " +
                    IntToStr((unsigned int)player->account_ident));
            server->mysql_controls->file_cache->characters_count--;
            if (player->character_slot_0 != NULL &&
                player->character_slot_0->character_id == character_id)
            {
                Player *removed = player->character_slot_0;
                player->character_slot_0 = player->character_slot_1;
                player->character_slot_1 = player->character_slot_2;
                player->character_slot_2 = NULL;
                delete removed;
            }
            if (player->character_slot_1 != NULL &&
                player->character_slot_1->character_id == character_id)
            {
                Player *removed = player->character_slot_1;
                player->character_slot_1 = player->character_slot_2;
                player->character_slot_2 = NULL;
                delete removed;
            }
            if (player->character_slot_2 != NULL &&
                player->character_slot_2->character_id == character_id)
            {
                Player *removed = player->character_slot_2;
                player->character_slot_2 = NULL;
                delete removed;
            }
            int count = 0;
            if (player->character_slot_0 != NULL)
                count = 1;
            if (player->character_slot_1 != NULL)
                count = 2;
            String reply = EO_EncodeNumber(server, 6, 2);
            reply.Insert(EO_EncodeNumber(server, count, 1), reply.Length() + 1);
            reply.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), reply.Length() + 1);
            for (int i = 0; i < 2; i++)
            {
                Player *character = player->character_slots[i];
                if (character == NULL)
                    continue;
                reply.Insert(character->name, reply.Length() + 1);
                reply.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->character_id, 4),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->level, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->gender, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->hair_style, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->hair_color, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->skin, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->admin_level, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->boots_graphic_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->armor_graphic_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->hat_graphic_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->shield_graphic_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, character->weapon_graphic_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), reply.Length() + 1);
            }
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Character, reply);
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->account_logged_in)
                return false;
            if (player->character_slot_0 != NULL && player->character_slot_1 != NULL &&
                player->character_slot_2 != NULL)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Character,
                                   EO_EncodeNumber(server, CharacterReply_Full3, 2) +
                                       "NO");
                return true;
            }
            else
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Character,
                                   EO_EncodeNumber(server, player->session_id, 2) + "OK");
                return true;
            }
        }
        if (action == PacketAction_Create)
        {
            if (!player->account_logged_in)
                return false;
            if (data.Length() < 0xa)
                return false;
            if (EO_DecodeNumber(server, data.SubString(1, 2)) != player->session_id)
                return false;
            if (player->account_create_cooldown > 4)
                return true;
            player->account_create_cooldown = 6;
            int gender = EO_DecodeNumber(server, data.SubString(3, 2));
            int hair_style = EO_DecodeNumber(server, data.SubString(5, 2));
            int hair_color = EO_DecodeNumber(server, data.SubString(7, 2));
            int skin = EO_DecodeNumber(server, data.SubString(9, 2));
            String name = mySQLdb::Db_SanitizeString(
                server->mysql_controls,
                PacketReader_GetBreakStringAt(
                    server, 2, data, EO_GetBreakByte(server, EO_BREAK_BYTE)));
            if (Players::CharName_Validate(server->players, player, name))
                return false;
            if (name.Length() > CHARNAME_MAX_LENGTH)
                return false;
            if (gender < Gender_Female || gender > Gender_Male || hair_style < 1 ||
                hair_style > HAIRSTYLE_MAX || hair_color < 0 ||
                hair_color > HAIRCOLOR_MAX || skin < 0 || skin > SKIN_MAX ||
                name.Length() < CHARNAME_MIN_LENGTH)
                return false;
            if (player->character_slot_0 != NULL && player->character_slot_1 != NULL &&
                player->character_slot_2 != NULL)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Character,
                                   EO_EncodeNumber(server, CharacterReply_Full, 2) +
                                       "NO");
                return true;
            }
            if (!CharName_CheckUnique(server, name))
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Character,
                    EO_EncodeNumber(server, CharacterReply_NotApproved, 2) + "NO");
                return true;
            }
            if (!mySQLdb::IsAlphabeticText(server->mysql_controls, name))
            {
                Banned::AddBan(
                    server->banned, player->remote_ip, player->hdid, (bool)0, 0x3840);
                return false;
            }
            mySQLdb::Mysql_SubmitQuery(
                server->mysql_controls,
                0x45,
                player->player_id,
                player->query_id,
                data,
                "SELECT ident FROM endl_characters WHERE name = '" + name + "' LIMIT 1");
            return true;
        }
    }
    if (family == PacketFamily_Welcome)
    {
        if (action == PacketAction_Request)
        {
            if (!player->account_logged_in)
                return false;
            if (player->logged_in)
                return false;
            if (data.Length() != 4)
                return false;
            if (Players::Players_GetIdleTimeout(server->players) >
                Settings::GetMaxPlayers(server->settings))
            {
                if (!server->logins->ConnectionLog_CheckIP(player->socket->RemoteAddress))
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Welcome,
                        EO_EncodeNumber(server, WelcomeCode_ServerBusy, 2) + "NO");
                    return true;
                }
            }
            int selected_id = EO_DecodeNumber(server, data.SubString(1, 4));
            String out = EO_EncodeNumber(server, 1, 2);
            out.Insert(EO_EncodeNumber(server, player->session_id, 2), out.Length() + 1);
            out.Insert(data.SubString(1, 4), out.Length() + 1);
            for (int i = 0; i < 3; i++)
            {
                if (player->character_slots[i] == NULL)
                    continue;
                Player *slot = player->character_slots[i];
                if (slot->map_id < 1 ||
                    (unsigned int)(int)server->map_control->maps.size() <=
                        (unsigned int)slot->map_id)
                    return false;
                if (slot->character_id != selected_id)
                    continue;
                if (!Mapcontrol_CountBlockedNeighbors(
                        server->map_control, slot->map_id, slot->x, slot->y))
                {
                    slot->map_id = Settings::GetRescueMap(server->settings);
                    slot->x = Settings::GetRescueX(server->settings);
                    slot->y = Settings::GetRescueY(server->settings);
                }
                out.Insert(EO_EncodeNumber(server, slot->map_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server,
                                           (unsigned short)Mapcontrol_GetByIndex(
                                               server->map_control, slot->map_id - 1)
                                               ->rid1,
                                           2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server,
                                           (unsigned short)Mapcontrol_GetByIndex(
                                               server->map_control, slot->map_id - 1)
                                               ->rid2,
                                           2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server,
                                           (unsigned short)Mapcontrol_GetByIndex(
                                               server->map_control, slot->map_id - 1)
                                               ->filesize,
                                           3),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->item_values->rid_1, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->item_values->rid_2, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->item_values->num_records, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->npc_values->rid_1, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->npc_values->rid_2, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->npc_values->num_records, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->skill_values->rid_1, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->skill_values->rid_2, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->skill_values->num_records, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->class_values->rid_1, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->class_values->rid_2, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, GUI->class_values->num_records, 2),
                           out.Length() + 1);
                out.Insert(slot->name, out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(slot->title, out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(slot->guild_name, out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(slot->guild_rank_name, out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->class_id, 1), out.Length() + 1);
                String guild_tag = slot->guild_tag;
                if (guild_tag.Length() == 2)
                    guild_tag = guild_tag + " ";
                if (guild_tag.Length() == 1)
                    guild_tag = guild_tag + "  ";
                if (guild_tag.Length() == 0)
                    guild_tag = guild_tag + "   ";
                out.Insert(guild_tag, out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->admin_level, 1),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->level, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->experience, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->usage, 4), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->hp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->max_hp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->tp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->max_tp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->max_sp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->stat_points, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->skill_points, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->karma + 0x3e8, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->min_damage, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->max_damage, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->accuracy, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->evasion, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->armor, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->adj_strength, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->adj_wisdom, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->adj_intelligence, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->adj_agility, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->adj_constitution, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->adj_charisma, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->boots_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->gloves_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->accessory_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->armor_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->belt_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->necklace_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->hat_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->shield_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->weapon_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->ring1_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->ring2_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->armlet1_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->armlet2_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->bracer1_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->bracer2_item_id, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, slot->guild_rank_id, 1),
                           out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(server, Settings::GetJailMap(server->settings), 2),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(server, Settings::GetRescueMap(server->settings), 2),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(server, Settings::GetRescueX(server->settings), 1),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(server, Settings::GetRescueY(server->settings), 1),
                    out.Length() + 1);
                out.Insert(EO_EncodeNumber(
                               server,
                               Settings::GetSpyAndLightGuideFloodRate(server->settings),
                               2),
                           out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(
                        server, Settings::GetGuardianFloodRate(server->settings), 2),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(
                        server, Settings::GetGameMasterFloodRate(server->settings), 2),
                    out.Length() + 1);
                out.Insert(EO_EncodeNumber(
                               server,
                               Settings::GetHighGameMasterFloodRate(server->settings),
                               2),
                           out.Length() + 1);
                if (Settings::GetStartMap(server->settings) == slot->map_id)
                    out.Insert(EO_EncodeNumber(server, 0xfa, 1), out.Length() + 1);
                else
                    out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Welcome, out);
                return true;
            }
            return false;
        }
        if (action == PacketAction_Msg)
        {
            if (!player->account_logged_in)
                return false;
            if (player->logged_in)
                return false;
            if (data.Length() != 7)
                return false;
            if (EO_DecodeNumber(server, data.SubString(1, 3)) != player->session_id)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Welcome,
                                   EO_EncodeNumber(server, WelcomeCode_ServerBusy, 2) +
                                       "NO");
                return true;
            }
            int selected_id = EO_DecodeNumber(server, data.SubString(4, 4));
            bool found = false;
            for (int i = 0; i < 3; i++)
            {
                if (player->character_slots[i] == NULL)
                    continue;
                Player *slot = player->character_slots[i];
                if (slot->character_id != selected_id)
                    continue;
                found = true;
                player->character_id = slot->character_id;
                player->class_id = slot->class_id;
                player->guild_rank_id = slot->guild_rank_id;
                player->account_id = slot->account_id;
                player->name = slot->name;
                player->partner_name = mySQLdb::Mysql_SanitizeString(
                    server->mysql_controls, slot->partner_name, false);
                player->guild_tag = mySQLdb::Mysql_SanitizeString(
                    server->mysql_controls, slot->guild_tag, true);
                player->title = mySQLdb::Mysql_SanitizeString(
                    server->mysql_controls, slot->title, false);
                player->guild_name = mySQLdb::Mysql_SanitizeString(
                    server->mysql_controls, slot->guild_name, false);
                player->guild_rank_name = mySQLdb::Mysql_SanitizeString(
                    server->mysql_controls, slot->guild_rank_name, false);
                player->experience = slot->experience;
                player->level = slot->level;
                player->signup = slot->signup;
                player->gender = slot->gender;
                player->hair_style = slot->hair_style;
                player->hair_color = slot->hair_color;
                player->skin = slot->skin;
                player->map_id = slot->map_id;
                player->x = slot->x;
                player->y = slot->y;
                player->direction = slot->direction;
                player->base_hp = slot->base_hp;
                player->max_hp = slot->max_hp;
                player->hp = slot->hp;
                player->base_tp = slot->base_tp;
                player->max_tp = slot->max_tp;
                player->tp = slot->tp;
                player->base_sp = slot->base_sp;
                player->max_sp = slot->max_sp;
                player->usage = slot->usage;
                player->money_bank = slot->money_bank;
                player->locker_bank = slot->locker_bank;
                player->stat_points = slot->stat_points;
                player->skill_points = slot->skill_points;
                player->karma = slot->karma;
                player->base_strength = slot->base_strength;
                player->base_wisdom = slot->base_wisdom;
                player->base_intelligence = slot->base_intelligence;
                player->base_agility = slot->base_agility;
                player->base_constitution = slot->base_constitution;
                player->base_charisma = slot->base_charisma;
                player->adj_strength = slot->adj_strength;
                player->adj_wisdom = slot->adj_wisdom;
                player->adj_intelligence = slot->adj_intelligence;
                player->adj_agility = slot->adj_agility;
                player->adj_constitution = slot->adj_constitution;
                player->adj_charisma = slot->adj_charisma;
                player->boots_graphic_id = slot->boots_graphic_id;
                player->boots_item_id = slot->boots_item_id;
                player->accessory_graphic_id = slot->accessory_graphic_id;
                player->accessory_item_id = slot->accessory_item_id;
                player->gloves_graphic_id = slot->gloves_graphic_id;
                player->gloves_item_id = slot->gloves_item_id;
                player->armor_graphic_id = slot->armor_graphic_id;
                player->armor_item_id = slot->armor_item_id;
                player->belt_graphic_id = slot->belt_graphic_id;
                player->belt_item_id = slot->belt_item_id;
                player->necklace_graphic_id = slot->necklace_graphic_id;
                player->necklace_item_id = slot->necklace_item_id;
                player->hat_graphic_id = slot->hat_graphic_id;
                player->hat_item_id = slot->hat_item_id;
                player->shield_graphic_id = slot->shield_graphic_id;
                player->shield_item_id = slot->shield_item_id;
                player->weapon_graphic_id = slot->weapon_graphic_id;
                player->weapon_item_id = slot->weapon_item_id;
                player->ring1_graphic_id = slot->ring1_graphic_id;
                player->ring1_item_id = slot->ring1_item_id;
                player->ring2_graphic_id = slot->ring2_graphic_id;
                player->ring2_item_id = slot->ring2_item_id;
                player->armlet1_graphic_id = slot->armlet1_graphic_id;
                player->armlet1_item_id = slot->armlet1_item_id;
                player->armlet2_graphic_id = slot->armlet2_graphic_id;
                player->armlet2_item_id = slot->armlet2_item_id;
                player->bracer1_graphic_id = slot->bracer1_graphic_id;
                player->bracer1_item_id = slot->bracer1_item_id;
                player->bracer2_graphic_id = slot->bracer2_graphic_id;
                player->bracer2_item_id = slot->bracer2_item_id;
                player->on_chair = slot->on_chair;
                player->admin_level = slot->admin_level;
                player->weight_current = slot->weight_current;
                player->home_id = slot->home_id;
                for (int j = 0; j < 7; j++)
                    player->element_resistances[j] = slot->element_resistances[j];
                player->class_min_damage = slot->class_min_damage;
                player->class_max_damage = slot->class_max_damage;
                player->class_accuracy = slot->class_accuracy;
                player->class_evasion = slot->class_evasion;
                player->class_armor = slot->class_armor;
                player->min_damage = slot->min_damage;
                player->max_damage = slot->max_damage;
                player->accuracy = slot->accuracy;
                player->evasion = slot->evasion;
                player->armor = slot->armor;
                player->equip_bonus_hp = slot->equip_bonus_hp;
                player->equip_bonus_tp = slot->equip_bonus_tp;
                player->equip_strength_bonus = slot->equip_strength_bonus;
                player->equip_wisdom_bonus = slot->equip_wisdom_bonus;
                player->equip_intelligence_bonus = slot->equip_intelligence_bonus;
                player->equip_agility_bonus = slot->equip_agility_bonus;
                player->equip_constitution_bonus = slot->equip_constitution_bonus;
                player->equip_charisma_bonus = slot->equip_charisma_bonus;
                player->target_map = slot->map_id;
                player->target_x = (short)slot->x;
                player->target_y = (short)slot->y;
                player->on_chair = slot->on_chair;
                player->sitting = slot->sitting;
                player->inventory.clear();
                player->trade_items.clear();
                player->bank.clear();
                player->action_queue.clear();
                *(TTimeStamp *)&player->walk_tick = DateTimeToTimeStamp(Now());
                player->quest_trackers.clear();
                player->quest_history.clear();
                int field_0x19b4 = player->money_bank;
                if (slot->invblob2 != "EOF" && slot->invblob2.Length() > 3)
                {
                    PacketReader_Init(server, slot->invblob2, ':');
                    bool done = false;
                    while (!done)
                    {
                        String s = PacketReader_GetBreakString(server);
                        if (s != "EOF" && s.Length() > 0)
                        {
                            try
                            {
                                int item_id = StrToInt(s);
                                int amount =
                                    StrToInt(PacketReader_GetBreakString(server));
                                PlayerInventory inv(item_id);
                                inv.amount = amount;
                                player->bank.insert(player->bank.end(), inv);
                            }
                            catch (...)
                            {
                                done = true;
                            }
                        }
                        else
                        {
                            done = true;
                        }
                    }
                }
                if (slot->invblob1 != "EOF" && slot->invblob1.Length() > 3)
                {
                    PacketReader_Init(server, slot->invblob1, ':');
                    bool done = false;
                    while (!done)
                    {
                        String s = PacketReader_GetBreakString(server);
                        if (s != "EOF" && s.Length() > 0)
                        {
                            try
                            {
                                int item_id = StrToInt(s);
                                int amount =
                                    StrToInt(PacketReader_GetBreakString(server));
                                if (amount < 0)
                                    continue;
                                if (item_id == 1)
                                {
                                    field_0x19b4 += amount;
                                    if (amount > 0x1e8480)
                                        amount = 0x1e8480;
                                }
                                player->weight_current +=
                                    ItemValues::GetWeight(GUI->item_values, item_id) *
                                    amount;
                                PlayerInventory inv(item_id);
                                inv.amount = amount;
                                player->inventory.insert(player->inventory.end(), inv);
                            }
                            catch (...)
                            {
                                done = true;
                            }
                        }
                        else
                        {
                            done = true;
                        }
                    }
                }
                if (slot->skillblob != "EOF" && slot->skillblob.Length() > 3)
                {
                    PacketReader_Init(server, slot->skillblob, ':');
                    bool done = false;
                    while (!done)
                    {
                        String s = PacketReader_GetBreakString(server);
                        if (s != "EOF" && s.Length() > 0)
                        {
                            try
                            {
                                int skill_id = StrToInt(s);
                                int level = StrToInt(PacketReader_GetBreakString(server));
                                PlayerSkill skill(skill_id);
                                skill.level = level;
                                player->spells.insert(player->spells.end(), skill);
                            }
                            catch (...)
                            {
                                done = true;
                            }
                        }
                        else
                        {
                            done = true;
                        }
                    }
                }
                String field_0x334 = slot->quest_cache;
                String field_0x338 = slot->quest_blob;
                if (slot->quest_cache != "EOF" && slot->quest_cache.Length() > 3)
                {
                    PacketReader_Init(server, slot->quest_cache, ':');
                    bool done = false;
                    while (!done)
                    {
                        String s = PacketReader_GetBreakString(server);
                        if (s != "EOF" && s.Length() > 0)
                        {
                            try
                            {
                                int quest_id = StrToInt(s);
                                int state_index =
                                    StrToInt(PacketReader_GetBreakString(server));
                                int version =
                                    StrToInt(PacketReader_GetBreakString(server));
                                if (QuestContainer::GetQuestVersion(server->quest_engine,
                                                                    quest_id) == version)
                                {
                                    PlayerQuest quest(quest_id, state_index, version);
                                    quest.counters[0] = (short)StrToInt(
                                        PacketReader_GetBreakString(server));
                                    quest.counters[1] = (short)StrToInt(
                                        PacketReader_GetBreakString(server));
                                    quest.counters[2] = (short)StrToInt(
                                        PacketReader_GetBreakString(server));
                                    quest.counters[3] = (short)StrToInt(
                                        PacketReader_GetBreakString(server));
                                    quest.counters[4] = (short)StrToInt(
                                        PacketReader_GetBreakString(server));
                                    player->quest_trackers.insert(
                                        player->quest_trackers.end(), quest);
                                    Player_ApplyQuestActions(
                                        server, player, &quest, false);
                                }
                            }
                            catch (...)
                            {
                                done = true;
                            }
                        }
                        else
                        {
                            done = true;
                        }
                    }
                }
                if (slot->quest_blob != "EOF" && slot->quest_blob.Length() > 3)
                {
                    PacketReader_Init(server, slot->quest_blob, ':');
                    bool done = false;
                    while (!done)
                    {
                        String s = PacketReader_GetBreakString(server);
                        if (s != "EOF" && s.Length() > 0)
                        {
                            try
                            {
                                int quest_id = StrToInt(s);
                                PlayerQuest quest(quest_id, 0, 0);
                                player->quest_history.insert(player->quest_history.end(),
                                                             quest);
                            }
                            catch (...)
                            {
                                done = true;
                            }
                        }
                        else
                        {
                            done = true;
                        }
                    }
                }
                player->home_name = InnValues::GetName(GUI->inn_values, player->home_id);
                player->enter_game_timestamp = Now();
                player->weight_max = player->adj_strength + 0x46;
                if (player->weight_max > 0xfa)
                    player->weight_max = 0xfa;
                break;
            }
            if (!found)
            {
                Banned::AddBan(
                    server->banned, player->remote_ip, player->hdid, (bool)0, 0x3840);
                return false;
            }
            if (player->map_id == 0)
                return false;
            if (!mySQLdb::IsAlphabeticText(server->mysql_controls, player->name) &&
                player->name != "vult-r")
                return false;
            player->map_has_quakes =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_quakes;
            player->map_has_hp_drain =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_hp_drain;
            player->map_has_tp_drain =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_tp_drain;
            player->map_has_spikes =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_spikes;
            MapContainer::Mapcontrol_IncPlayerCount(server->map_control, player->map_id);
            String out = EO_EncodeNumber(server, 2, 2);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(Settings::GetJoinMessage(server->settings), out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            for (int n = 0; n < 8; n++)
            {
                out.Insert(NewsTopics::Get(GUI->news_control, n), out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            }
            int weight_current = player->weight_current;
            int weight_max = player->weight_max;
            if (weight_current > 0xfa)
                weight_current = 0xfa;
            if (weight_max > 0xfa)
                weight_max = 0xfa;
            out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
            vector<PlayerInventory>::iterator iter;
            for (iter = player->inventory.begin(); iter != player->inventory.end();
                 iter++)
            {
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 4), out.Length() + 1);
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            vector<PlayerSkill>::iterator iter2;
            for (iter2 = player->spells.begin(); iter2 != player->spells.end(); iter2++)
            {
                out.Insert(EO_EncodeNumber(server, iter2->skill_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter2->level, 2), out.Length() + 1);
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(Refresh_BuildReply(server, player), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Welcome, out);
            out = EO_GetBreakByte(server, EO_BREAK_BYTE);
            out.Insert(Player_SerializeAvatar(server, player, -1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Agree, PacketFamily_Players, out);
            player->logged_in = 1;
            player->session_id = RandRange(0xc350) + 0x2710;
            mySQLdb::Mysql_ExecDirect(
                server->mysql_controls,
                player->account_ident,
                "UPDATE endl_characters SET online = 1 WHERE ident = " +
                    IntToStr(player->character_id));
            Players::Players_UpdatePeakOnline(server->players);
            return true;
        }
        if (action == PacketAction_Agree)
        {
            if (data.Length() < 3)
                return false;
            if (EO_DecodeNumber(server, data.SubString(2, 2)) != player->session_id)
                return false;
            int code = EO_DecodeNumber(server, String(data[1]));
            if (code == 1)
            {
                Client_SendRaw(
                    server,
                    player,
                    Mapcontrol_ReadRawFile(server->map_control,
                                           EO_DecodeNumber(server, data.SubString(4, 2))),
                    5);
            }
            if (code == 2)
            {
                int index = EO_DecodeNumber(server, data.SubString(4, 1));
                if (index > 0 && GUI->item_values->string_list->Count >= index)
                    Client_SendRaw(server,
                                   player,
                                   EO_EncodeNumber(server, index, 1) +
                                       GUI->item_values->string_list->Strings[index - 1],
                                   6);
            }
            if (code == 3)
            {
                int index = EO_DecodeNumber(server, data.SubString(4, 1));
                if (index > 0 && GUI->npc_values->string_list->Count >= index)
                    Client_SendRaw(server,
                                   player,
                                   EO_EncodeNumber(server, index, 1) +
                                       GUI->npc_values->string_list->Strings[index - 1],
                                   7);
            }
            if (code == 4)
            {
                int index = EO_DecodeNumber(server, data.SubString(4, 1));
                if (index > 0 && GUI->skill_values->string_list->Count >= index)
                    Client_SendRaw(server,
                                   player,
                                   EO_EncodeNumber(server, index, 1) +
                                       GUI->skill_values->string_list->Strings[index - 1],
                                   8);
            }
            if (code == 5)
            {
                int index = EO_DecodeNumber(server, data.SubString(4, 1));
                if (index > 0 && GUI->class_values->string_list->Count >= index)
                    Client_SendRaw(server,
                                   player,
                                   EO_EncodeNumber(server, index, 1) +
                                       GUI->class_values->string_list->Strings[index - 1],
                                   0xc);
            }
            return true;
        }
    }
    if (family == PacketFamily_Range && action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 1)
            return false;
        PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        String players = PacketReader_GetBreakString(server);
        String npcs = PacketReader_GetBreakString(server);
        String out = EO_GetBreakByte(server, EO_BREAK_BYTE);
        int count = 0;
        if (players.Length() > 0)
        {
            for (int i = 0; i * 2 + 2 <= players.Length(); i++)
            {
                int id = EO_DecodeNumber(server, players.SubString(i * 2 + 1, 2));
                Player *target = Players::Players_GetById(server->players, id);
                if (target != NULL && target->map_id == player->map_id)
                {
                    count++;
                    out.Insert(Player_SerializeAvatar(server, target, -1),
                               out.Length() + 1);
                }
            }
        }
        out.Insert(EO_EncodeNumber(server, count, 1), 1);
        if (npcs.Length() > 0)
        {
            for (int j = 0; j + 1 <= npcs.Length(); j++)
            {
                int id = EO_DecodeNumber(server, npcs.SubString(j + 1, 1));
                String name = NpcRange_Lookup(server, player, id);
                if (name.Length() > 0)
                    out.Insert(name, out.Length() + 1);
            }
        }
        if (out.Length() < 3)
            return true;
        Client_SendEncoded(server, player, PacketAction_Reply, PacketFamily_Range, out);
        return true;
    }
    if (family == PacketFamily_PlayerRange && action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 2)
            return false;
        int target_id = EO_DecodeNumber(server, data.SubString(1, 2));
        Player *target = Players::Players_GetById(server->players, target_id);
        if (target == NULL)
            return true;
        if (target->map_id != player->map_id)
            return true;
        String reply = EO_GetBreakByte(server, EO_BREAK_BYTE);
        reply.Insert(Player_SerializeAvatar(server, target, -1), reply.Length() + 1);
        reply.Insert(EO_EncodeNumber(server, 1, 1), 1);
        if (reply.Length() < 3)
            return true;
        Client_SendEncoded(server, player, PacketAction_Reply, PacketFamily_Range, reply);
        return true;
    }
    if (family == PacketFamily_NpcRange && action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 3)
            return false;
        int n = data.Length() - 2;
        int cnt = 0;
        String names = "";
        for (int i = 0; i < n; i++)
        {
            int id = EO_DecodeNumber(server, data.SubString(i + 3, 1));
            String name = NpcRange_Lookup(server, player, id);
            if (name.Length() > 0)
            {
                names.Insert(name, names.Length() + 1);
                cnt++;
            }
        }
        if (cnt < 1)
            return true;
        names.Insert(EO_EncodeNumber(server, cnt, 1), 1);
        Client_SendEncoded(server, player, PacketAction_Agree, PacketFamily_Npc, names);
        return true;
    }
    if (family == PacketFamily_Face)
    {
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int delay = stamp.Time - player->walk_tick;
        if (delay > WALK_DELAY_SANITY_MS)
            delay = WALK_MIN_DELAY_MS;
        if (delay < WALK_MIN_DELAY_MS)
        {
            if (player->action_queue.size() > ACTION_QUEUE_MAX)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        else
        {
            if (player->action_queue.size() > 0)
            {
                PlayerCommand command(family, action, data);
                player->action_queue.insert(player->action_queue.end(), command);
                return true;
            }
            else
            {
                return Face_Execute(server, player, action, &data);
            }
        }
    }
    if (family == PacketFamily_Players)
    {
        if (action == PacketAction_List)
        {
            if (!player->logged_in)
                return false;
            Client_SendRaw(server, player, Server_BuildOnlineNames(server), 0xb);
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4 || data.Length() > 0x10)
                return false;
            Player *found = Players::Players_FindByName(server->players, data);
            if (found == NULL)
            {
                Client_SendEncoded(
                    server, player, PacketAction_Ping, PacketFamily_Players, data);
                return true;
            }
            if (found->hide_online)
            {
                Client_SendEncoded(
                    server, player, PacketAction_Ping, PacketFamily_Players, data);
                return true;
            }
            if (found->map_id == player->map_id)
            {
                Client_SendEncoded(
                    server, player, PacketAction_Pong, PacketFamily_Players, data);
                return true;
            }
            Client_SendEncoded(
                server, player, PacketAction_Net242, PacketFamily_Players, data);
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            Client_SendRaw(server, player, Server_BuildOnlineList(server), 9);
            return true;
        }
    }
    if (family == PacketFamily_Emote && action == PacketAction_Report)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 1)
            return false;
        int emote = EO_DecodeNumber(server, String(data[1]));
        if ((unsigned int)EO_DecodeNumber(server, String(data[1])) > Emote_Embarrassed &&
            EO_DecodeNumber(server, String(data[1])) != Emote_Playful)
            return true;
        String buf = EO_EncodeNumber(server, player->player_id, 2);
        buf.Insert(data[1], buf.Length() + 1);
        Server_BroadcastNearby(
            server, player, PacketAction_Player, PacketFamily_Emote, buf);
        return true;
    }
    if (family == PacketFamily_Refresh && action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        player->flush_queue = 1;
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Refresh,
                           Refresh_BuildReply(server, player));
        return true;
    }
    if (family == PacketFamily_Sit)
    {
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 1)
                return false;
            int sit_action = EO_DecodeNumber(server, String(data[1]));
            if (sit_action == SitAction_Sit)
            {
                if (!player->sitting && !player->on_chair)
                {
                    player->sitting = 1;
                    player->on_chair = 0;
                    String msg = EO_EncodeNumber(server, player->player_id, 2);
                    msg.Insert(EO_EncodeNumber(server, player->x, 1), msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, player->y, 1), msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, player->direction, 1),
                               msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, 0, 1), msg.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Reply, PacketFamily_Sit, msg);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Player, PacketFamily_Sit, msg);
                    return true;
                }
                else
                {
                    if (!player->on_chair)
                    {
                        player->on_chair = 0;
                        player->sitting = 0;
                        String msg = EO_EncodeNumber(server, player->player_id, 2);
                        msg.Insert(EO_EncodeNumber(server, player->x, 1),
                                   msg.Length() + 1);
                        msg.Insert(EO_EncodeNumber(server, player->y, 1),
                                   msg.Length() + 1);
                        Client_SendEncoded(
                            server, player, PacketAction_Close, PacketFamily_Sit, msg);
                        Server_BroadcastNearby(
                            server, player, PacketAction_Remove, PacketFamily_Sit, msg);
                        return true;
                    }
                    else
                    {
                        family = PacketFamily_Chair;
                    }
                }
            }
            else
            {
                if (!player->sitting)
                    return true;
                player->on_chair = 0;
                player->sitting = 0;
                String msg = EO_EncodeNumber(server, player->player_id, 2);
                msg.Insert(EO_EncodeNumber(server, player->x, 1), msg.Length() + 1);
                msg.Insert(EO_EncodeNumber(server, player->y, 1), msg.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Close, PacketFamily_Sit, msg);
                Server_BroadcastNearby(
                    server, player, PacketAction_Remove, PacketFamily_Sit, msg);
                return true;
            }
        }
    }
    if (family == PacketFamily_Chair)
    {
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int delay = stamp.Time - player->walk_tick;
        if (delay > WALK_DELAY_SANITY_MS)
            delay = WALK_MIN_DELAY_MS;
        if (delay < WALK_MIN_DELAY_MS)
        {
            if (player->action_queue.size() > ACTION_QUEUE_MAX)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        else
        {
            if (player->action_queue.size() > 0)
            {
                PlayerCommand command(family, action, data);
                player->action_queue.insert(player->action_queue.end(), command);
                return true;
            }
            else
            {
                return Chair_Execute(server, player, action, &data);
            }
        }
    }
    if (family == PacketFamily_Door && action == PacketAction_Open)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 2)
            return false;
        MapCoord coord;
        coord.x = EO_DecodeNumber(server, data.SubString(1, 1));
        coord.y = EO_DecodeNumber(server, data.SubString(2, 1));
        if (Server_InViewRange(server, coord.x, coord.y, player->x, player->y))
        {
            String encoded = EO_EncodeNumber(server, coord.x, 1);
            encoded.Insert(EO_EncodeNumber(server, coord.y, 2), encoded.Length() + 1);
            int warp = MapContainer::Mapcontrol_GetWarpDoorAt(
                server->map_control, player->map_id, coord);
            if (warp > 0)
            {
                if (Players::Player_HasKeyItem(server->players, player, warp))
                {
                    if (MapContainer::Mapcontrol_ToggleDoor(
                            server->map_control, player->map_id, coord.x, coord.y))
                        Server_BroadcastNearTile(server,
                                                 -0xd,
                                                 player->map_id,
                                                 coord,
                                                 PacketAction_Open,
                                                 PacketFamily_Door,
                                                 encoded);
                }
                else
                {
                    Client_SendEncoded(server,
                                       player,
                                       PacketAction_Close,
                                       PacketFamily_Door,
                                       EO_EncodeNumber(server, warp, 2));
                }
                return true;
            }
            if (MapContainer::Mapcontrol_ToggleDoor(
                    server->map_control, player->map_id, coord.x, coord.y))
                Server_BroadcastNearTile(server,
                                         -0xd,
                                         player->map_id,
                                         coord,
                                         PacketAction_Open,
                                         PacketFamily_Door,
                                         encoded);
        }
        return true;
    }
    if (family == PacketFamily_Item)
    {
        if (action == PacketAction_Use)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            int item_type = ItemValues::GetType(GUI->item_values, item_id);
            if (!Players::Player_RemoveItem(server->players, player, item_id, 1))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Agree,
                                   PacketFamily_Item,
                                   EO_EncodeNumber(server, item_id, 2));
                return true;
            }
            player->weight_current -= ItemValues::GetWeight(GUI->item_values, item_id);
            if (player->weight_current < 0)
                player->weight_current = 0;
            int weight_current = player->weight_current;
            int weight_max = player->weight_max;
            if (weight_current > 250)
                weight_current = 250;
            if (weight_max > 250)
                weight_max = 250;
            if (item_type == 0x19)
            {
                if (Players::Player_UnequipAll(server->players, player))
                {
                    String out = "";
                    out.Insert(EO_EncodeNumber(server, player->player_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, player->boots_graphic_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, player->armor_graphic_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, player->hat_graphic_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, player->weapon_graphic_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, player->shield_graphic_id, 2),
                               out.Length() + 1);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Agree, PacketFamily_Avatar, out);
                }
                Player::UpdateBaseStats(player);
                Player_CalculateStats(server, player);
                Player::CalculateHP_TP_SP(player);
                String out = EO_EncodeNumber(server, item_type, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_strength, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_intelligence, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_wisdom, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_agility, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_constitution, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_charisma, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->min_damage, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_damage, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->accuracy, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->evasion, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->armor, 2), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                Player_FireQuestTriggers(server, player, 0x190, 0);
                return true;
            }
            if (item_type == 0x17)
            {
                int spec1 = ItemValues::GetSpec1(GUI->item_values, item_id);
                String out = EO_EncodeNumber(server, item_type, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, spec1, 2), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                out = EO_EncodeNumber(server, player->player_id, 2);
                out.Insert(EO_EncodeNumber(server, spec1, 3), out.Length() + 1);
                Server_BroadcastNearby(
                    server, player, PacketAction_Player, PacketFamily_Effect, out);
                Player_FireQuestTriggers(server, player, 0x190, 0);
                return true;
            }
            if (item_type == 0x18)
            {
                int spec1 = ItemValues::GetSpec1(GUI->item_values, item_id);
                player->hair_color = spec1;
                String out = EO_EncodeNumber(server, item_type, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, spec1, 1), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                out = EO_EncodeNumber(server, player->player_id, 2);
                out.Insert(EO_EncodeNumber(server, 3, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, spec1, 1), out.Length() + 1);
                Server_BroadcastNearby(
                    server, player, PacketAction_Agree, PacketFamily_Avatar, out);
                Player_FireQuestTriggers(server, player, 0x190, 0);
                return true;
            }
            if (item_type == 0x16)
            {
                String out = EO_EncodeNumber(server, item_type, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                Player_FireQuestTriggers(server, player, 0x190, 0);
                return true;
            }
            if (item_type == 0x6)
            {
                player->experience += ItemValues::GetSpec1(GUI->item_values, item_id);
                int levels = Players::Player_TryLevelUp(server->players, player);
                if (levels > 0)
                {
                    Server_BroadcastNearby(server,
                                           player,
                                           PacketAction_Accept,
                                           PacketFamily_Item,
                                           EO_EncodeNumber(server, player->player_id, 2));
                }
                String out = EO_EncodeNumber(server, item_type, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->experience, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, levels, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->stat_points, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->skill_points, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_sp, 2), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                Player_FireQuestTriggers(server, player, 0x190, 0);
                return true;
            }
            if (item_type == 0x3)
            {
                int hp = ItemValues::GetHP(GUI->item_values, item_id);
                int tp = ItemValues::GetTP(GUI->item_values, item_id);
                player->hp += hp;
                player->tp += tp;
                if (player->hp > player->max_hp)
                    player->hp = player->max_hp;
                if (player->tp > player->max_tp)
                    player->tp = player->max_tp;
                String out = EO_EncodeNumber(server, item_type, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, hp, 4), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->hp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->tp, 2), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                if (hp > 0)
                {
                    int percent = player->hp * 100 / player->max_hp;
                    out = EO_EncodeNumber(server, player->player_id, 2);
                    out.Insert(EO_EncodeNumber(server, hp, 4), out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, percent, 1), out.Length() + 1);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Agree, PacketFamily_Recover, out);
                    if (player->in_party)
                    {
                        String msg = EO_EncodeNumber(server, player->player_id, 2);
                        msg.Insert(EO_EncodeNumber(server, Player::HpPercent(player), 1),
                                   msg.Length() + 1);
                        Server_BroadcastToParty(
                            server, player, PacketAction_Agree, PacketFamily_Party, msg);
                    }
                }
                Player_FireQuestTriggers(server, player, 0x190, 0);
                return true;
            }
            if (item_type == 0x4)
            {
                if (player->map_id == 0)
                    return true;
                if (Mapcontrol_GetCanScroll(server->map_control, player->map_id))
                {
                    Players::Player_AddItem(server->players, player, item_id, 1);
                    return true;
                }
                if (ItemValues::GetLevelRequirement(GUI->item_values, item_id) >
                    player->level)
                {
                    String out = "The scroll is unreadable, it requires level " +
                                 IntToStr(ItemValues::GetLevelRequirement(
                                     GUI->item_values, item_id));
                    Client_SendEncoded(
                        server, player, PacketAction_Server, PacketFamily_Talk, out);
                    Players::Player_AddItem(server->players, player, item_id, 1);
                    return true;
                }
                int scroll_map = ItemValues::GetScrollMap(GUI->item_values, item_id);
                ItemSpecXY spec = ItemValues::GetSpecXY(GUI->item_values, item_id);
                if (scroll_map < 1 || (int)server->map_control->maps.size() < scroll_map)
                {
                    if (scroll_map == 0 && spec.spec2 == 0 && spec.spec3 == 0)
                    {
                        scroll_map = InnValues::GetSpawnMap(
                            GUI->inn_values, player->home_id, player->level);
                        spec.spec2 = InnValues::GetSpawnX(
                            GUI->inn_values, player->home_id, player->level);
                        spec.spec3 = InnValues::GetSpawnY(
                            GUI->inn_values, player->home_id, player->level);
                    }
                    else
                    {
                        return true;
                    }
                }
                if (Mapcontrol_GetByIndex(server->map_control, scroll_map - 1)->width <
                        1 ||
                    Mapcontrol_GetByIndex(server->map_control, scroll_map - 1)->height <
                        1)
                    return true;
                String out = EO_EncodeNumber(server, 4, 1);
                out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_current, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Item, out);
                Player_Warp(server,
                            player,
                            scroll_map,
                            *(MapCoord *)&spec,
                            WarpEffect_Scroll,
                            false);
                return true;
            }
        }
        if (action == PacketAction_Drop)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 7)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            unsigned int amount = EO_DecodeNumber(server, data.SubString(3, 3));
            int x = EO_DecodeNumber(server, data.SubString(6, 1));
            int y = EO_DecodeNumber(server, data.SubString(7, 1));
            if (ItemValues::GetSpecial(GUI->item_values, item_id) == 4)
                return true;
            if (player->map_id == Settings::GetJailMap(server->settings))
                return false;
            if (player->drop_counter < 1)
                return true;
            player->drop_counter--;
            if (amount > 0x989680 || amount < 1)
                return true;
            if (x > 0xfa || y > 0xfa)
            {
                x = player->x;
                y = player->y;
            }
            if (Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->ground_items.size() > 0x3e7)
                return true;
            if (!Coords_IsWithinTwo(server, player->x, player->y, x, y))
                return true;
            if (!Mapcontrol_IsDropTileClear(server->map_control, player->map_id, x, y))
                return true;
            if (!Mapcontrol_CanDropItemAt(
                    server->map_control, player->map_id, x, y, player->account_ident))
                return true;
            if (!Players::Player_RemoveItem(server->players, player, item_id, amount))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Agree,
                                   PacketFamily_Item,
                                   EO_EncodeNumber(server, item_id, 2));
                return true;
            }
            int ground_index =
                MapContainer::Mapcontrol_AddGroundItem(server->map_control,
                                                       player->map_id,
                                                       item_id,
                                                       x,
                                                       y,
                                                       player->item_change_count,
                                                       player->account_ident,
                                                       6);
            if (ground_index < 0)
                return true;
            player->weight_current -= ItemValues::GetWeight(GUI->item_values, item_id) *
                                      player->item_change_count;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int drop_weight_current = player->weight_current;
            int drop_weight_max = player->weight_max;
            if (drop_weight_current > 250)
                drop_weight_current = 250;
            if (drop_weight_max > 250)
                drop_weight_max = 250;
            String out = EO_EncodeNumber(server, item_id, 2);
            out.Insert(EO_EncodeNumber(server, ground_index, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->item_change_count, 3),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, x, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, y, 1), out.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Add, PacketFamily_Item, out);
            out = EO_EncodeNumber(server, item_id, 2);
            out.Insert(EO_EncodeNumber(server, player->item_change_count, 3),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, ground_index, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, x, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, y, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, drop_weight_current, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, drop_weight_max, 1), out.Length() + 1);
            Client_SendEncoded(server, player, PacketAction_Drop, PacketFamily_Item, out);
            Player_FireQuestTriggers(server, player, 0x190, 0);
            return true;
        }
        if (action == PacketAction_Junk)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 5)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            unsigned int amount = EO_DecodeNumber(server, data.SubString(3, 3));
            if (amount > 0x989680)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, item_id, amount))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Agree,
                                   PacketFamily_Item,
                                   EO_EncodeNumber(server, item_id, 2));
                return true;
            }
            player->weight_current -= ItemValues::GetWeight(GUI->item_values, item_id) *
                                      player->item_change_count;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int junk_weight_current = player->weight_current;
            int junk_weight_max = player->weight_max;
            if (junk_weight_current > 250)
                junk_weight_current = 250;
            if (junk_weight_max > 250)
                junk_weight_max = 250;
            String out = EO_EncodeNumber(server, item_id, 2);
            out.Insert(EO_EncodeNumber(server, player->item_change_count, 3),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, junk_weight_current, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, junk_weight_max, 1), out.Length() + 1);
            Client_SendEncoded(server, player, PacketAction_Junk, PacketFamily_Item, out);
            Player_FireQuestTriggers(server, player, 0x190, 0);
            return true;
        }
        if (action == PacketAction_Get)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int ground_index = EO_DecodeNumber(server, data.SubString(1, 2));
            GroundItemInfo info = Mapcontrol_TakeGroundItemInfo(
                server->map_control, player->map_id, ground_index, player->account_ident);
            if (info.x == -2)
            {
                String out = EO_EncodeNumber(server, 2, 2);
                Client_SendEncoded(
                    server, player, PacketAction_Spec, PacketFamily_Item, out);
                return true;
            }
            if (info.x < 0 || info.y < 0)
                return true;
            if (!Coords_IsWithinTwo(server, player->x, player->y, info.x, info.y))
                return true;
            Mapcontrol_RemoveGroundItem(
                server->map_control, player->map_id, ground_index);
            Players::Player_AddItem(server->players, player, info.item_id, info.amount);
            player->weight_current +=
                ItemValues::GetWeight(GUI->item_values, info.item_id) * info.amount;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int get_weight_current = player->weight_current;
            int get_weight_max = player->weight_max;
            if (get_weight_current > 250)
                get_weight_current = 250;
            if (get_weight_max > 250)
                get_weight_max = 250;
            String out = EO_EncodeNumber(server, ground_index, 2);
            Server_BroadcastNearby(
                server, player, PacketAction_Remove, PacketFamily_Item, out);
            out = EO_EncodeNumber(server, ground_index, 2);
            out.Insert(EO_EncodeNumber(server, info.item_id, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, info.amount, 3), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, get_weight_current, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, get_weight_max, 1), out.Length() + 1);
            Client_SendEncoded(server, player, PacketAction_Get, PacketFamily_Item, out);
            Player_FireQuestTriggers(server, player, 0x190, 0);
            return true;
        }
    }
    if (family == PacketFamily_Warp)
    {
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int read_map = EO_DecodeNumber(server, data.SubString(1, 2));
            int token = EO_DecodeNumber(server, data.SubString(3, 2));
            if (player->session_id != token)
                return false;
            if (player->warp_map != read_map)
                return false;
            if (player->read_len > 0)
            {
                Player *target = server->players->by_id[player->read_len];
                if (target != NULL)
                {
                    Client_SendEncoded(server,
                                       target,
                                       PacketAction_Close,
                                       PacketFamily_Trade,
                                       EO_EncodeNumber(server, player->player_id, 2));
                }
            }
            Server_BroadcastNearby(server,
                                   player,
                                   PacketAction_Remove,
                                   PacketFamily_Avatar,
                                   EO_EncodeNumber(server, player->player_id, 2) +
                                       EO_EncodeNumber(server, player->warp_state, 1));
            if (player->map_id > 0)
            {
                player->map_has_quakes = false;
                player->map_has_hp_drain = false;
                player->map_has_tp_drain = false;
                player->map_has_spikes = false;
                MapContainer::Mapcontrol_DecPlayerCount(server->map_control,
                                                        player->map_id);
            }
            player->map_switch_pending = true;
            player->target_map = player->map_id;
            player->target_x = player->x;
            player->target_y = player->y;
            player->map_id = 0;
            player->x = 0;
            player->y = 0;
            player->idle_ticks = 0;
            player->dead = false;
            player->read_len = -1;
            player->guild_inviter_id = -1;
            player->session_token = -1;
            player->field_0x8c = "";
            Client_SendRaw(
                server, player, Mapcontrol_ReadRawFile(server->map_control, read_map), 4);
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            if (player->warp_state < 0)
                return true;
            int warp_map = EO_DecodeNumber(server, data.SubString(1, 2));
            int token = EO_DecodeNumber(server, data.SubString(3, 2));
            if (player->session_id != token)
                return false;
            if (player->warp_map != warp_map)
                return false;
            if (player->warp_x > 0xf0 || player->warp_x < 0)
                return true;
            if (player->warp_y > 0xf0 || player->warp_y < 0)
                return true;
            if (player->read_len > 0)
            {
                Player *target = server->players->by_id[player->read_len];
                if (target != NULL)
                {
                    Client_SendEncoded(server,
                                       target,
                                       PacketAction_Close,
                                       PacketFamily_Trade,
                                       EO_EncodeNumber(server, player->player_id, 2));
                }
            }
            if (player->map_id > 0)
            {
                Server_BroadcastNearby(
                    server,
                    player,
                    PacketAction_Remove,
                    PacketFamily_Avatar,
                    EO_EncodeNumber(server, player->player_id, 2) +
                        EO_EncodeNumber(server, player->warp_state, 1));
                MapContainer::Mapcontrol_DecPlayerCount(server->map_control,
                                                        player->map_id);
            }
            int saved_state = player->warp_state;
            player->map_id = player->warp_map;
            player->x = player->warp_x;
            player->y = player->warp_y;
            player->idle_ticks = 0;
            player->dead = false;
            if (!player->arena_playing)
                player->arena_queued = false;
            if (player->arena_playing)
                player->arena_queued = true;
            player->arena_playing = false;
            player->read_len = -1;
            player->guild_inviter_id = -1;
            player->session_token = -1;
            player->field_0x8c = "";
            player->warp_state = 0;
            player->warp_pending = false;
            player->map_switch_pending = false;
            player->on_chair = false;
            player->sitting = false;
            player->map_has_quakes =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_quakes;
            player->map_has_hp_drain =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_hp_drain;
            player->map_has_tp_drain =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_tp_drain;
            player->map_has_spikes =
                Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                    ->has_spikes;
            MapContainer::Mapcontrol_IncPlayerCount(server->map_control, player->map_id);
            String out = EO_GetBreakByte(server, EO_BREAK_BYTE);
            out.Insert(Player_SerializeAvatar(server, player, saved_state),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Agree, PacketFamily_Players, out);
            out = EO_EncodeNumber(server, 2, 1);
            out.Insert(EO_EncodeNumber(server, player->map_id, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, saved_state, 1), out.Length() + 1);
            out.Insert(Refresh_BuildReply(server, player), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Agree, PacketFamily_Warp, out);
            Player_FireQuestTriggers(server, player, 0xb, 0);
            return true;
        }
    }
    if (family == PacketFamily_Paperdoll)
    {
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int id = EO_DecodeNumber(server, data.SubString(1, 2));
            if (player->player_id == id)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Paperdoll,
                                   Paperdoll_BuildReply(server, player));
                return true;
            }
            Player *target = Players::Players_GetById(server->players, id);
            if (target == NULL)
                return true;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Paperdoll,
                               Paperdoll_BuildReply(server, target));
            return true;
        }
        if (action == PacketAction_Add)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 3)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            int slot = EO_DecodeNumber(server, data.SubString(3, 1));
            ItemValue *item = ItemValues::GetByIndex(GUI->item_values, item_id - 1);
            if (!ClassValues::ClassMatches(
                    GUI->class_values, player->class_id, item->class_requirement))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Ping,
                                   PacketFamily_Paperdoll,
                                   EO_EncodeNumber(server, player->class_id, 1));
                return true;
            }
            if (player->level < item->level_requirement)
                return true;
            if (player->adj_strength < item->strength_requirement)
                return true;
            if (player->adj_intelligence < item->intelligence_requirement)
                return true;
            if (player->adj_wisdom < item->wisdom_requirement)
                return true;
            if (player->adj_agility < item->agility_requirement)
                return true;
            if (player->adj_constitution < item->constitution_requirement)
                return true;
            if (player->adj_charisma < item->charisma_requirement)
                return true;
            if (!Players::Player_EquipItem(server->players, player, item_id, slot))
                return true;
            if (item->element < 7)
                player->element_resistances[item->element] += item->element_damage;
            player->min_damage += item->min_damage;
            player->max_damage += item->max_damage;
            player->accuracy += item->accuracy;
            player->evasion += item->evade;
            player->armor += item->armor;
            player->equip_bonus_hp += item->hp;
            player->equip_bonus_tp += item->tp;
            player->equip_strength_bonus += item->strength;
            player->equip_wisdom_bonus += item->wisdom;
            player->equip_intelligence_bonus += item->intelligence;
            player->equip_agility_bonus += item->agility;
            player->equip_constitution_bonus += item->constitution;
            player->equip_charisma_bonus += item->charisma;
            Player::UpdateBaseStats(player);
            Player_CalculateStats(server, player);
            Player::CalculateHP_TP_SP(player);
            String out = "";
            out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->boots_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->armor_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->hat_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->weapon_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->shield_graphic_id, 2),
                       out.Length() + 1);
            if (player->equip_result > 1)
                Server_BroadcastNearby(
                    server, player, PacketAction_Agree, PacketFamily_Avatar, out);
            out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->equip_result_count, 3),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, slot, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_strength, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_intelligence, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_wisdom, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_agility, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_constitution, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_charisma, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->min_damage, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_damage, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->accuracy, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->evasion, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->armor, 2), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Agree, PacketFamily_Paperdoll, out);
            return true;
        }
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 3)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            int slot = EO_DecodeNumber(server, data.SubString(3, 1));
            if (ItemValues::GetSpecial(GUI->item_values, item_id) == 5)
                return true;
            if (!Players::Player_UnequipItem(server->players, player, item_id, slot))
                return true;
            ItemValue *item = ItemValues::GetByIndex(GUI->item_values, item_id - 1);
            if (item->element < 7)
                player->element_resistances[item->element] -= item->element_damage;
            player->min_damage -= item->min_damage;
            player->max_damage -= item->max_damage;
            player->accuracy -= item->accuracy;
            player->evasion -= item->evade;
            player->armor -= item->armor;
            player->equip_bonus_hp -= item->hp;
            player->equip_bonus_tp -= item->tp;
            player->equip_strength_bonus -= item->strength;
            player->equip_wisdom_bonus -= item->wisdom;
            player->equip_intelligence_bonus -= item->intelligence;
            player->equip_agility_bonus -= item->agility;
            player->equip_constitution_bonus -= item->constitution;
            player->equip_charisma_bonus -= item->charisma;
            Player::UpdateBaseStats(player);
            Player_CalculateStats(server, player);
            Player::CalculateHP_TP_SP(player);
            String out = "";
            out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->boots_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->armor_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->hat_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->weapon_graphic_id, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->shield_graphic_id, 2),
                       out.Length() + 1);
            if (player->equip_result > 1)
                Server_BroadcastNearby(
                    server, player, PacketAction_Agree, PacketFamily_Avatar, out);
            out.Insert(EO_EncodeNumber(server, item_id, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, slot, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_strength, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_intelligence, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_wisdom, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_agility, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_constitution, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_charisma, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->min_damage, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_damage, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->accuracy, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->evasion, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->armor, 2), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Remove, PacketFamily_Paperdoll, out);
            return true;
        }
    }
    if (family == PacketFamily_Book && action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 2)
            return false;
        int target_id = EO_DecodeNumber(server, data.SubString(1, 2));
        if (player->player_id == target_id)
            return true;
        Player *target = Players::Players_GetById(server->players, target_id);
        if (target == NULL)
            return true;
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Book,
                           Player_SerializePaperdoll(server, target));
        return true;
    }
    if (family == PacketFamily_Chest)
    {
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            MapCoord coords;
            coords.x = EO_DecodeNumber(server, data.SubString(1, 1));
            coords.y = EO_DecodeNumber(server, data.SubString(2, 1));
            int slot = EO_DecodeNumber(server, data.SubString(3, 2));
            if (!Coords_IsAdjacent(server, coords.x, coords.y, player->x, player->y))
                return true;
            ItemStack stack = MapContainer::Mapcontrol_TakeChestItem(
                server->map_control, player->map_id, coords, slot);
            if (stack.id < 1)
                return true;
            Players::Player_AddItem(server->players, player, stack.id, stack.amount);
            player->weight_current +=
                ItemValues::GetWeight(GUI->item_values, stack.id) * stack.amount;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int weight = player->weight_current;
            int weight_max = player->weight_max;
            if (weight > 250)
                weight = 250;
            if (weight_max > 250)
                weight_max = 250;
            String item_str = Mapcontrol_BuildChestItemsString(
                server->map_control, player->map_id, coords);
            Server_BroadcastAdjacent(
                server, player, coords, PacketAction_Agree, PacketFamily_Chest, item_str);
            String out = EO_EncodeNumber(server, stack.id, 2);
            out.Insert(EO_EncodeNumber(server, stack.amount, 3), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, weight, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
            out.Insert(item_str, out.Length() + 1);
            Client_SendEncoded(server, player, PacketAction_Get, PacketFamily_Chest, out);
            return true;
        }
        if (action == PacketAction_Add)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 7)
                return false;
            MapCoord coords;
            coords.x = EO_DecodeNumber(server, data.SubString(1, 1));
            coords.y = EO_DecodeNumber(server, data.SubString(2, 1));
            int item_id = EO_DecodeNumber(server, data.SubString(3, 2));
            int amount = EO_DecodeNumber(server, data.SubString(5, 3));
            if (ItemValues::GetSpecial(GUI->item_values, item_id) == 4)
                return true;
            if ((unsigned int)amount > 10000000)
                return true;
            int slot_count = MapContainer::Mapcontrol_GetChestSlotCount(
                server->map_control, player->map_id, coords);
            if (slot_count < 0 || slot_count > 4)
            {
                Client_SendEncoded(
                    server, player, PacketAction_Spec, PacketFamily_Chest, "0");
                return true;
            }
            if (!Coords_IsAdjacent(server, coords.x, coords.y, player->x, player->y))
                return true;
            if (!Players::Player_RemoveItem(server->players, player, item_id, amount))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Agree,
                                   PacketFamily_Item,
                                   EO_EncodeNumber(server, item_id, 2));
                return true;
            }
            player->weight_current -= ItemValues::GetWeight(GUI->item_values, item_id) *
                                      player->item_change_count;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int weight = player->weight_current;
            int weight_max = player->weight_max;
            if (weight > 250)
                weight = 250;
            if (weight_max > 250)
                weight_max = 250;
            MapContainer::Mapcontrol_AddChestItem(server->map_control,
                                                  player->map_id,
                                                  coords,
                                                  item_id,
                                                  player->item_change_count);
            String item_str = Mapcontrol_BuildChestItemsString(
                server->map_control, player->map_id, coords);
            Server_BroadcastAdjacent(
                server, player, coords, PacketAction_Agree, PacketFamily_Chest, item_str);
            String out = EO_EncodeNumber(server, item_id, 2);
            out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, weight, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, weight_max, 1), out.Length() + 1);
            out.Insert(item_str, out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Chest, out);
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            MapCoord coords;
            coords.x = EO_DecodeNumber(server, data.SubString(1, 1));
            coords.y = EO_DecodeNumber(server, data.SubString(2, 1));
            if (!Coords_IsAdjacent(server, coords.x, coords.y, player->x, player->y))
                return true;
            String out = Mapcontrol_BuildChestItemsString(
                server->map_control, player->map_id, coords);
            if (out == "N")
            {
                Client_SendEncoded(
                    server, player, PacketAction_Close, PacketFamily_Chest, "N");
                return true;
            }
            int chest_slot =
                Mapcontrol_GetChestKeyAt(server->map_control, player->map_id, coords);
            if (chest_slot < 1)
            {
                out.Insert(data.SubString(1, 2), 1);
                Client_SendEncoded(
                    server, player, PacketAction_Open, PacketFamily_Chest, out);
                return true;
            }
            if (Players::Player_HasKeyItem(server->players, player, chest_slot))
            {
                out.Insert(data.SubString(1, 2), 1);
                Client_SendEncoded(
                    server, player, PacketAction_Open, PacketFamily_Chest, out);
                return true;
            }
            Client_SendEncoded(server,
                               player,
                               PacketAction_Close,
                               PacketFamily_Chest,
                               EO_EncodeNumber(server, chest_slot, 2));
            return true;
        }
    }
    if (family == PacketFamily_Shop)
    {
        if (action == PacketAction_Create)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int craft_id = EO_DecodeNumber(server, data.SubString(1, 2));
            int shop_id = EO_DecodeNumber(server, data.SubString(3, 4));
            if (player->session_token != shop_id)
                return true;
            ShopCraftIngredient ingredient1 =
                ShopValues::GetCraftIngredient1(GUI->shop_values, shop_id, craft_id);
            ShopCraftIngredient ingredient2 =
                ShopValues::GetCraftIngredient2(GUI->shop_values, shop_id, craft_id);
            ShopCraftIngredient ingredient3 =
                ShopValues::GetCraftIngredient3(GUI->shop_values, shop_id, craft_id);
            ShopCraftIngredient ingredient4 =
                ShopValues::GetCraftIngredient4(GUI->shop_values, shop_id, craft_id);
            if (ingredient1.item_id < 1 && ingredient2.item_id < 1 &&
                ingredient3.item_id < 1 && ingredient4.item_id < 1)
                return true;
            bool can_craft = true;
            bool any_ingredient = false;
            if (ingredient1.item_id > 0 && ingredient1.amount > 0)
            {
                any_ingredient = true;
                if (Players::Players_GetItemAmount(
                        server->players, player, ingredient1.item_id) <
                    ingredient1.amount)
                    can_craft = false;
            }
            if (ingredient2.item_id > 0 && ingredient2.amount > 0)
            {
                any_ingredient = true;
                if (Players::Players_GetItemAmount(
                        server->players, player, ingredient2.item_id) <
                    ingredient2.amount)
                    can_craft = false;
            }
            if (ingredient3.item_id > 0 && ingredient3.amount > 0)
            {
                any_ingredient = true;
                if (Players::Players_GetItemAmount(
                        server->players, player, ingredient3.item_id) <
                    ingredient3.amount)
                    can_craft = false;
            }
            if (ingredient4.item_id > 0 && ingredient4.amount > 0)
            {
                any_ingredient = true;
                if (Players::Players_GetItemAmount(
                        server->players, player, ingredient4.item_id) <
                    ingredient4.amount)
                    can_craft = false;
            }
            if (!can_craft || !any_ingredient)
                return true;
            String reply = EO_EncodeNumber(server, craft_id, 2);
            if (ingredient1.item_id > 0 && ingredient1.amount > 0 &&
                Players::Player_RemoveItem(
                    server->players, player, ingredient1.item_id, ingredient1.amount))
            {
                reply.Insert(EO_EncodeNumber(server, ingredient1.item_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                             reply.Length() + 1);
            }
            if (ingredient2.item_id > 0 && ingredient2.amount > 0 &&
                Players::Player_RemoveItem(
                    server->players, player, ingredient2.item_id, ingredient2.amount))
            {
                reply.Insert(EO_EncodeNumber(server, ingredient2.item_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                             reply.Length() + 1);
            }
            if (ingredient3.item_id > 0 && ingredient3.amount > 0 &&
                Players::Player_RemoveItem(
                    server->players, player, ingredient3.item_id, ingredient3.amount))
            {
                reply.Insert(EO_EncodeNumber(server, ingredient3.item_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                             reply.Length() + 1);
            }
            if (ingredient4.item_id > 0 && ingredient4.amount > 0 &&
                Players::Player_RemoveItem(
                    server->players, player, ingredient4.item_id, ingredient4.amount))
            {
                reply.Insert(EO_EncodeNumber(server, ingredient4.item_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                             reply.Length() + 1);
            }
            Players::Player_AddItem(server->players, player, craft_id, 1);
            player->weight_current += ItemValues::GetWeight(GUI->item_values, craft_id);
            if (player->weight_current < 0)
                player->weight_current = 0;
            int weight = player->weight_current;
            int weight_max = player->weight_max;
            if (weight > 250)
                weight = 250;
            if (weight_max > 250)
                weight_max = 250;
            reply.Insert(EO_EncodeNumber(server, weight_max, 1), 3);
            reply.Insert(EO_EncodeNumber(server, weight, 1), 3);
            Client_SendEncoded(
                server, player, PacketAction_Create, PacketFamily_Shop, reply);
            Player_FireQuestTriggers(server, player, 400, 0);
            return true;
        }
        if (action == PacketAction_Buy)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 10)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            unsigned int amount = EO_DecodeNumber(server, data.SubString(3, 4));
            int shop_id = EO_DecodeNumber(server, data.SubString(7, 4));
            if (amount > 100)
            {
                Banned::AddBan(
                    server->banned, player->remote_ip, player->hdid, (char)0, 1200);
                return false;
            }
            if (player->session_token != shop_id)
                return true;
            int price =
                ShopValues::GetBuyPrice(GUI->shop_values, shop_id, item_id, amount);
            if (price < 0)
                return true;
            if (amount < 1)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, 1, price))
                return true;
            Players::Player_AddItem(server->players, player, item_id, amount);
            player->weight_current +=
                ItemValues::GetWeight(GUI->item_values, item_id) * amount;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int weight = player->weight_current;
            int weight_max = player->weight_max;
            if (weight > 250)
                weight = 250;
            if (weight_max > 250)
                weight_max = 250;
            String reply = EO_EncodeNumber(server, player->item_change_remaining, 4);
            reply.Insert(EO_EncodeNumber(server, item_id, 2), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, amount, 4), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, weight, 1), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, weight_max, 1), reply.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Buy, PacketFamily_Shop, reply);
            Player_FireQuestTriggers(server, player, 400, 0);
            return true;
        }
        if (action == PacketAction_Sell)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 10)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            unsigned int amount = EO_DecodeNumber(server, data.SubString(3, 4));
            int shop_id = EO_DecodeNumber(server, data.SubString(7, 4));
            if (amount > 100)
            {
                Banned::AddBan(
                    server->banned, player->remote_ip, player->hdid, (char)0, 1200);
                return false;
            }
            if (player->session_token != shop_id)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, item_id, amount))
                return true;
            int price = ShopValues::GetSellPrice(
                GUI->shop_values, shop_id, item_id, player->item_change_count);
            if (price < 0)
                return true;
            Players::Player_AddItem(server->players, player, 1, price);
            player->weight_current -= ItemValues::GetWeight(GUI->item_values, item_id) *
                                      player->item_change_count;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int weight = player->weight_current;
            int weight_max = player->weight_max;
            if (weight > 250)
                weight = 250;
            if (weight_max > 250)
                weight_max = 250;
            int gold = Players::Players_GetItemAmount(server->players, player, 1);
            String reply = EO_EncodeNumber(server, player->item_change_remaining, 4);
            reply.Insert(EO_EncodeNumber(server, item_id, 2), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, gold, 4), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, weight, 1), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, weight_max, 1), reply.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Sell, PacketFamily_Shop, reply);
            Player_FireQuestTriggers(server, player, 400, 0);
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return true;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Shop)
                return true;
            player->session_token = type_info.behavior_id;
            Client_SendEncoded(
                server,
                player,
                PacketAction_Open,
                PacketFamily_Shop,
                ShopValues::BuildOpenData(GUI->shop_values, type_info.behavior_id));
            return true;
        }
    }
    if (family == PacketFamily_Locker)
    {
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            MapCoord coords;
            coords.x = EO_DecodeNumber(server, data.SubString(1, 1));
            coords.y = EO_DecodeNumber(server, data.SubString(2, 1));
            int item_id = EO_DecodeNumber(server, data.SubString(3, 2));
            if (!Coords_IsAdjacent(server, coords.x, coords.y, player->x, player->y))
                return true;
            if (Mapcontrol_GetTileSpecValueAt(
                    server->map_control, player->map_id, coords.x, coords.y) != 0xf)
                return true;
            if (!Players::Player_RemoveBankItem(server->players, player, item_id))
                return true;
            Players::Player_AddItem(
                server->players, player, item_id, player->item_change_count);
            player->weight_current += ItemValues::GetWeight(GUI->item_values, item_id) *
                                      player->item_change_count;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int take_weight_current = player->weight_current;
            int take_weight_max = player->weight_max;
            if (take_weight_current > 250)
                take_weight_current = 250;
            if (take_weight_max > 250)
                take_weight_max = 250;
            String out = EO_EncodeNumber(server, item_id, 2);
            out.Insert(EO_EncodeNumber(server, player->item_change_count, 3),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, take_weight_current, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, take_weight_max, 1), out.Length() + 1);
            vector<PlayerInventory>::iterator iter;
            for (iter = player->bank.begin(); iter != player->bank.end(); iter++)
            {
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 3), out.Length() + 1);
            }
            Client_SendEncoded(
                server, player, PacketAction_Get, PacketFamily_Locker, out);
            Player_FireQuestTriggers(server, player, 0x190, 0);
            return true;
        }
        if (action == PacketAction_Add)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 7)
                return false;
            MapCoord coords;
            coords.x = EO_DecodeNumber(server, data.SubString(1, 1));
            coords.y = EO_DecodeNumber(server, data.SubString(2, 1));
            int item_id = EO_DecodeNumber(server, data.SubString(3, 2));
            int amount = EO_DecodeNumber(server, data.SubString(5, 3));
            if ((unsigned int)amount > 0xc8 || item_id < 2)
                return true;
            if (player->bank.size() > 0x3c)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Spec,
                                   PacketFamily_Locker,
                                   EO_EncodeNumber(server, player->locker_bank, 1));
                return true;
            }
            if (player->bank.size() > player->locker_bank * 5 + 0x18)
            {
                if (!Players::Player_HasBankItem(server->players, player, item_id))
                {
                    Client_SendEncoded(server,
                                       player,
                                       PacketAction_Spec,
                                       PacketFamily_Locker,
                                       EO_EncodeNumber(server, player->locker_bank, 1));
                    return true;
                }
            }
            if (!Coords_IsAdjacent(server, coords.x, coords.y, player->x, player->y))
                return true;
            if (Mapcontrol_GetTileSpecValueAt(
                    server->map_control, player->map_id, coords.x, coords.y) != 0xf)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, item_id, amount))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Agree,
                                   PacketFamily_Item,
                                   EO_EncodeNumber(server, item_id, 2));
                return true;
            }
            player->weight_current -=
                ItemValues::GetWeight(GUI->item_values, item_id) * (unsigned int)amount;
            if (player->weight_current < 0)
                player->weight_current = 0;
            int add_weight_current = player->weight_current;
            int add_weight_max = player->weight_max;
            if (add_weight_current > 250)
                add_weight_current = 250;
            if (add_weight_max > 250)
                add_weight_max = 250;
            Players::Player_AddBankItem(
                server->players, player, item_id, player->item_change_count);
            String out = EO_EncodeNumber(server, item_id, 2);
            out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, add_weight_current, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, add_weight_max, 1), out.Length() + 1);
            vector<PlayerInventory>::iterator iter;
            for (iter = player->bank.begin(); iter != player->bank.end(); iter++)
            {
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 3), out.Length() + 1);
            }
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Locker, out);
            Player_FireQuestTriggers(server, player, 0x190, 0);
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            MapCoord coords;
            coords.x = EO_DecodeNumber(server, data.SubString(1, 1));
            coords.y = EO_DecodeNumber(server, data.SubString(2, 1));
            if (!Coords_IsAdjacent(server, coords.x, coords.y, player->x, player->y))
                return true;
            if (Mapcontrol_GetTileSpecValueAt(
                    server->map_control, player->map_id, coords.x, coords.y) != 0xf)
                return true;
            String out = data.SubString(1, 2);
            vector<PlayerInventory>::iterator iter;
            for (iter = player->bank.begin(); iter != player->bank.end(); iter++)
            {
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 3), out.Length() + 1);
            }
            Client_SendEncoded(
                server, player, PacketAction_Open, PacketFamily_Locker, out);
            return true;
        }
        if (action == PacketAction_Buy)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 1)
                return false;
            if (player->locker_bank > 6)
                return false;
            int cost = player->locker_bank * 0x3e8 + 0x3e8;
            if (!Players::Player_RemoveItem(server->players, player, 1, cost))
                return true;
            player->locker_bank++;
            String out = EO_EncodeNumber(server, player->item_change_remaining, 4);
            out.Insert(EO_EncodeNumber(server, player->locker_bank, 1), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Buy, PacketFamily_Locker, out);
            return true;
        }
    }
    if (family == PacketFamily_Board)
    {
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int board = EO_DecodeNumber(server, data.SubString(1, 2));
            int post_id = EO_DecodeNumber(server, data.SubString(3, 2));
            MsgBoardController::DeletePost(GUI->msgboard_control, board, post_id);
            return true;
        }
        if (action == PacketAction_Create)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int board = EO_DecodeNumber(server, data.SubString(1, 2));
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String subject = PacketReader_GetBreakString(server);
            String message = PacketReader_GetBreakString(server);
            if (subject.Length() < 1 || message.Length() < 1)
                return true;
            if (subject.Length() > 0x20)
                subject = subject.SubString(1, 0x20);
            if (message.Length() > 0x4b0)
                message = message.SubString(1, 0x4b0);
            if (MsgBoardController::CountPosts(
                    GUI->msgboard_control, board, player->name) > 1)
                return true;
            MsgBoardController::AddPost(
                GUI->msgboard_control, board, player->name, subject, message, 0);
            return true;
        }
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int board = EO_DecodeNumber(server, data.SubString(1, 2));
            int post_id = EO_DecodeNumber(server, data.SubString(3, 2));
            String post =
                MsgBoardController::GetPost(GUI->msgboard_control, board, post_id);
            if (post.Length() < 1)
                return true;
            Client_SendEncoded(
                server, player, PacketAction_Player, PacketFamily_Board, post);
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int board = EO_DecodeNumber(server, data.SubString(1, 2));
            String board_data =
                MsgBoardController::GetBoard(GUI->msgboard_control, board + 1);
            if (board_data.Length() < 1)
                return true;
            Client_SendEncoded(
                server, player, PacketAction_Open, PacketFamily_Board, board_data);
            return true;
        }
    }
    if (family == PacketFamily_Barber)
    {
        if (action == PacketAction_Buy)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int style = EO_DecodeNumber(server, data.SubString(1, 1));
            int color = EO_DecodeNumber(server, data.SubString(2, 1));
            int cost = EO_DecodeNumber(server, data.SubString(3, 4));
            if (style < 0 || style > 0x14)
                return false;
            if (color < 0 || color > 9)
                return false;
            if (cost < 0x30d40 || cost > 0x493e0)
                return false;
            if (player->session_token != cost)
                return false;
            int price = 0xc8;
            if (player->level > 0)
                price = player->level * 0xc8;
            if (!Players::Player_RemoveItem(server->players, player, 1, price))
                return true;
            player->hair_style = style;
            player->hair_color = color;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            out.Insert(EO_EncodeNumber(server, 2, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, style, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, color, 1), out.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Agree, PacketFamily_Avatar, out);
            out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4), 1);
            Client_SendEncoded(
                server, player, PacketAction_Agree, PacketFamily_Barber, out);
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return false;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Barber)
                return true;
            player->session_token = RandRange(SESSION_TOKEN_SPAN) + SESSION_BASE_BARBER;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Open,
                               PacketFamily_Barber,
                               EO_EncodeNumber(server, player->session_token, 4));
            return true;
        }
    }
    if (family == PacketFamily_Citizen)
    {
        if (action == PacketAction_Request || action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            int inn_index = EO_DecodeNumber(server, data.SubString(3, 2));
            if (player->session_id != npc_index)
                return true;
            if (player->session_token != inn_index)
                return false;
            if (player->hp >= player->max_hp)
                return true;
            int cost = (player->max_hp - player->hp) + (player->max_tp - player->tp);
            if (action == PacketAction_Request)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Request,
                                   PacketFamily_Citizen,
                                   EO_EncodeNumber(server, cost, 4));
                return true;
            }
            else
            {
                int sleep_map = InnValues::GetSleepMap(GUI->inn_values, inn_index);
                if (sleep_map < 1)
                    return true;
                if (!Players::Player_RemoveItem(server->players, player, 1, cost))
                    return true;
                MapCoord sleep_pos;
                sleep_pos.x = InnValues::GetSleepX(GUI->inn_values, inn_index);
                sleep_pos.y = InnValues::GetSleepY(GUI->inn_values, inn_index);
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Accept,
                    PacketFamily_Citizen,
                    EO_EncodeNumber(server, player->item_change_remaining, 4));
                player->hp = player->max_hp;
                player->tp = player->max_tp;
                if (player->in_party)
                {
                    String msg = EO_EncodeNumber(server, player->player_id, 2);
                    msg.Insert(EO_EncodeNumber(server, Player::HpPercent(player), 1),
                               msg.Length() + 1);
                    Server_BroadcastToParty(
                        server, player, PacketAction_Agree, PacketFamily_Party, msg);
                }
                Player_Warp(server, player, sleep_map, sleep_pos, WarpEffect_None, false);
                return true;
            }
        }
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int id = EO_DecodeNumber(server, data.SubString(1, 2));
            if (player->session_token != id)
                return false;
            if (player->home_id != id)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Remove,
                    PacketFamily_Citizen,
                    EO_EncodeNumber(server, InnUnsubscribeReply_NotCitizen, 1));
                return true;
            }
            if (player->home_id == 0)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Remove,
                    PacketFamily_Citizen,
                    EO_EncodeNumber(server, InnUnsubscribeReply_NotCitizen, 1));
                return true;
            }
            player->home_id = 0;
            player->home_name = InnValues::GetName(GUI->inn_values, 0);
            Client_SendEncoded(
                server,
                player,
                PacketAction_Remove,
                PacketFamily_Citizen,
                EO_EncodeNumber(server, InnUnsubscribeReply_Unsubscribed, 1));
            return true;
        }
        if (action == PacketAction_Reply)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            int npc_index = EO_DecodeNumber(server, PacketReader_GetBreakString(server));
            int inn_index = EO_DecodeNumber(server, PacketReader_GetBreakString(server));
            String ans1 = PacketReader_GetBreakString(server);
            String ans2 = PacketReader_GetBreakString(server);
            String ans3 = PacketReader_GetBreakString(server);
            if (player->session_id != npc_index)
                return true;
            if (player->session_token != inn_index)
                return false;
            int count = 0;
            if (LowerCase(ans1) !=
                LowerCase(InnValues::GetAnswer(GUI->inn_values, inn_index, 0)))
                count++;
            if (LowerCase(ans2) !=
                LowerCase(InnValues::GetAnswer(GUI->inn_values, inn_index, 1)))
                count++;
            if (LowerCase(ans3) !=
                LowerCase(InnValues::GetAnswer(GUI->inn_values, inn_index, 2)))
                count++;
            if (count == 0)
            {
                player->home_id = inn_index;
                player->home_name = InnValues::GetName(GUI->inn_values, inn_index);
            }
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Citizen,
                               EO_EncodeNumber(server, count, 1));
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return false;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Inn)
                return true;
            player->session_token = type_info.behavior_id - 1;
            String out = EO_EncodeNumber(server, player->session_token, 3);
            out.Insert(EO_EncodeNumber(server, player->home_id, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->session_id, 2), out.Length() + 1);
            out.Insert(InnValues::GetQuestion(GUI->inn_values, type_info.behavior_id - 1),
                       out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Open, PacketFamily_Citizen, out);
            return true;
        }
    }
    if (family == PacketFamily_Bank)
    {
        if (action == PacketAction_Add)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 7)
                return false;
            unsigned int amount = EO_DecodeNumber(server, data.SubString(1, 4));
            int session = EO_DecodeNumber(server, data.SubString(5, 3));
            if (session < 0x186a0 || session > 0x30d40)
                return false;
            if (player->session_token != session)
                return false;
            if (player->money_bank > 100000000000)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, 1, amount))
                return true;
            player->money_bank = player->money_bank + amount;
            String out = EO_EncodeNumber(server, player->item_change_remaining, 4);
            out.Insert(EO_EncodeNumber(server, player->money_bank, 4), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Bank, out);
            return true;
        }
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 7)
                return false;
            unsigned int amount = EO_DecodeNumber(server, data.SubString(1, 4));
            int session = EO_DecodeNumber(server, data.SubString(5, 3));
            if (player->session_token != session)
                return true;
            if (player->money_bank < amount)
                return true;
            player->money_bank = player->money_bank - amount;
            Players::Player_AddItem(server->players, player, 1, amount);
            String out = EO_EncodeNumber(server, player->item_change_amount, 4);
            out.Insert(EO_EncodeNumber(server, player->money_bank, 4), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Bank, out);
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return false;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Bank)
                return true;
            player->session_token = RandRange(SESSION_TOKEN_SPAN) + SESSION_BASE_BANK;
            String out = EO_EncodeNumber(server, player->money_bank, 4);
            out.Insert(EO_EncodeNumber(server, player->session_token, 3),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->locker_bank, 1), out.Length() + 1);
            Client_SendEncoded(server, player, PacketAction_Open, PacketFamily_Bank, out);
            return true;
        }
    }
    if (family == PacketFamily_Jukebox)
    {
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            String tracks = JukeBoxController::BuildRecentTracksString(
                GUI->jukebox_control, player->map_id);
            if (tracks.Length() > 0)
                Client_SendEncoded(
                    server, player, PacketAction_Open, PacketFamily_Jukebox, tracks);
            return true;
        }
        if (action == PacketAction_Msg)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 4)
                return false;
            int track = EO_DecodeNumber(server, data.SubString(3, 2)) + 1;
            if (track < 1)
                return false;
            if (JukeBoxController::TryPlayTrack(
                    GUI->jukebox_control, player->map_id, player->name))
            {
                if (!Players::Player_RemoveItem(server->players, player, 1, 0x19))
                {
                    Client_SendEncoded(server,
                                       player,
                                       PacketAction_Reply,
                                       PacketFamily_Jukebox,
                                       EO_EncodeNumber(server, 1, 2));
                    return true;
                }
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Agree,
                    PacketFamily_Jukebox,
                    EO_EncodeNumber(server, player->item_change_remaining, 4));
                Server_BroadcastToMap(server,
                                      player->map_id,
                                      PacketAction_Use,
                                      PacketFamily_Jukebox,
                                      EO_EncodeNumber(server, track, 2));
                return true;
            }
            return true;
        }
        if (action == PacketAction_Use)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int track = EO_DecodeNumber(server, String(data[1]));
            int index = EO_DecodeNumber(server, String(data[2]));
            if (track != 0x31 && track != 0x32)
                return false;
            if (index < 1 || index > 0x24)
                return false;
            if (player->weapon_graphic_id != track)
                return false;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            out.Insert(EO_EncodeNumber(server, player->direction, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, track, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, index, 1), out.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Msg, PacketFamily_Jukebox, out);
            return true;
        }
    }
    if (family == PacketFamily_Trade)
    {
        if (action == PacketAction_Add)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            int amount = EO_DecodeNumber(server, data.SubString(3, 4));
            int have = Players::Players_GetItemAmount(server->players, player, item_id);
            if (ItemValues::GetSpecial(GUI->item_values, item_id) == 4)
                return true;
            if ((unsigned int)amount > (unsigned int)have || (unsigned int)amount < 1)
                return true;
            if (player->read_len < 1)
                return false;
            if ((unsigned int)Players::Players_GetItemAmount(
                    server->players, player, item_id) < (unsigned int)amount)
            {
                Banned::AddBan(
                    server->banned, player->remote_ip, player->hdid, (char)0, 0xc8);
                return false;
            }
            Player *target = server->players->by_id[player->read_len];
            if (target == NULL)
                return false;
            if (player->map_id != target->map_id)
                return true;
            if (target->read_len != player->player_id)
            {
                player->trade_accepted = false;
                return false;
            }
            if (!Players::Player_AddTradeItem(server->players, player, item_id, amount))
                return true;
            player->trade_accepted = false;
            player->trade_value = 100;
            target->trade_accepted = false;
            target->trade_value = 100;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            vector<PlayerInventory>::iterator iter;
            for (iter = player->trade_items.begin(); iter != player->trade_items.end();
                 iter++)
            {
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 4), out.Length() + 1);
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, target->player_id, 2), out.Length() + 1);
            vector<PlayerInventory>::iterator iter2;
            for (iter2 = target->trade_items.begin(); iter2 != target->trade_items.end();
                 iter2++)
            {
                out.Insert(EO_EncodeNumber(server, iter2->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter2->amount, 4), out.Length() + 1);
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Trade, out);
            Client_SendEncoded(
                server, target, PacketAction_Reply, PacketFamily_Trade, out);
            return true;
        }
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            Player *target = server->players->by_id[player->read_len];
            if (target == NULL)
                return false;
            if (player->map_id != target->map_id)
                return false;
            if (target->read_len != player->player_id)
            {
                player->trade_accepted = false;
                return false;
            }
            int item_id = EO_DecodeNumber(server, data.SubString(1, 2));
            if (!Players::Player_RemoveTradeItem(server->players, player, item_id))
                return true;
            player->trade_accepted = false;
            player->trade_value = 100;
            target->trade_accepted = false;
            target->trade_value = 100;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            vector<PlayerInventory>::iterator iter;
            for (iter = player->trade_items.begin(); iter != player->trade_items.end();
                 iter++)
            {
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 4), out.Length() + 1);
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, target->player_id, 2), out.Length() + 1);
            vector<PlayerInventory>::iterator iter2;
            for (iter2 = target->trade_items.begin(); iter2 != target->trade_items.end();
                 iter2++)
            {
                out.Insert(EO_EncodeNumber(server, iter2->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter2->amount, 4), out.Length() + 1);
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Trade, out);
            Client_SendEncoded(
                server, target, PacketAction_Reply, PacketFamily_Trade, out);
            return true;
        }
        if (action == PacketAction_Agree)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 1)
                return false;
            int arg = EO_DecodeNumber(server, String(data[1]));
            Player *target = server->players->by_id[player->read_len];
            if (target == NULL)
                return false;
            if (player->map_id != target->map_id)
                return false;
            if (target->read_len != player->player_id)
            {
                player->trade_accepted = false;
                return false;
            }
            if (target->trade_accepted == 0 || arg != 1)
            {
                if (arg == 0)
                {
                    player->trade_accepted = 0;
                    player->trade_value = 100;
                }
                if (arg == 1)
                {
                    player->trade_accepted = 1;
                    player->trade_value = (int)target->trade_items.size();
                }
                String out = EO_EncodeNumber(server, player->player_id, 2);
                out.Insert(EO_EncodeNumber(server, arg, 1), out.Length() + 1);
                Client_SendEncoded(
                    server, target, PacketAction_Agree, PacketFamily_Trade, out);
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Spec,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, arg, 1));
                return true;
            }
            if ((int)player->trade_items.size() != target->trade_value)
            {
                player->trade_accepted = 0;
                player->trade_value = 100;
                target->trade_accepted = 0;
                target->trade_value = 100;
                String out = EO_EncodeNumber(server, player->player_id, 2);
                vector<PlayerInventory>::iterator iter;
                for (iter = player->trade_items.begin();
                     iter != player->trade_items.end();
                     iter++)
                {
                    out.Insert(EO_EncodeNumber(server, iter->item_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, iter->amount, 4),
                               out.Length() + 1);
                }
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, target->player_id, 2),
                           out.Length() + 1);
                vector<PlayerInventory>::iterator iter2;
                for (iter2 = target->trade_items.begin();
                     iter2 != target->trade_items.end();
                     iter2++)
                {
                    out.Insert(EO_EncodeNumber(server, iter2->item_id, 2),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, iter2->amount, 4),
                               out.Length() + 1);
                }
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Admin, PacketFamily_Trade, out);
                Client_SendEncoded(
                    server, target, PacketAction_Admin, PacketFamily_Trade, out);
                return true;
            }
            vector<PlayerInventory>::iterator iter;
            for (iter = player->trade_items.begin(); iter != player->trade_items.end();
                 iter++)
            {
                int item_id = iter->item_id;
                int have =
                    Players::Players_GetItemAmount(server->players, player, item_id);
                if (have < 0)
                    have = 0;
                if ((unsigned int)iter->amount > (unsigned int)have)
                    goto trade_invalid_player;
                if ((unsigned int)iter->amount >= 1)
                    continue;
            trade_invalid_player:
                player->trade_items.clear();
                player->trade_accepted = 0;
                target->trade_accepted = 0;
                Client_SendEncoded(server,
                                   target,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, player->player_id, 2));
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, target->player_id, 2));
                return true;
            }
            vector<PlayerInventory>::iterator iter2;
            for (iter2 = target->trade_items.begin(); iter2 != target->trade_items.end();
                 iter2++)
            {
                int item_id = iter2->item_id;
                int have =
                    Players::Players_GetItemAmount(server->players, target, item_id);
                if (have < 0)
                    have = 0;
                if ((unsigned int)iter2->amount > (unsigned int)have)
                    goto trade_invalid_target;
                if ((unsigned int)iter2->amount >= 1)
                    continue;
            trade_invalid_target:
                target->trade_items.clear();
                target->trade_accepted = 0;
                player->trade_accepted = 0;
                Client_SendEncoded(server,
                                   target,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, player->player_id, 2));
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, target->player_id, 2));
                return true;
            }
            if (player->trade_items.size() < 1)
            {
                Client_SendEncoded(server,
                                   target,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, player->player_id, 2));
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, target->player_id, 2));
                return true;
            }
            if (target->trade_items.size() < 1)
            {
                Client_SendEncoded(server,
                                   target,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, player->player_id, 2));
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Close,
                                   PacketFamily_Trade,
                                   EO_EncodeNumber(server, target->player_id, 2));
                return true;
            }
            String out = EO_EncodeNumber(server, player->player_id, 2);
            for (iter = player->trade_items.begin(); iter != player->trade_items.end();
                 iter++)
            {
                Players::Player_RemoveItem(
                    server->players, player, iter->item_id, iter->amount);
                Players::Player_AddItem(
                    server->players, target, iter->item_id, iter->amount);
                out.Insert(EO_EncodeNumber(server, iter->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter->amount, 4), out.Length() + 1);
                player->weight_current =
                    player->weight_current -
                    ItemValues::GetWeight(GUI->item_values, iter->item_id) *
                        (unsigned int)iter->amount;
                target->weight_current =
                    target->weight_current +
                    ItemValues::GetWeight(GUI->item_values, iter->item_id) *
                        (unsigned int)iter->amount;
                if (player->weight_current < 0)
                    player->weight_current = 0;
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, target->player_id, 2), out.Length() + 1);
            for (iter2 = target->trade_items.begin(); iter2 != target->trade_items.end();
                 iter2++)
            {
                Players::Player_RemoveItem(
                    server->players, target, iter2->item_id, iter2->amount);
                Players::Player_AddItem(
                    server->players, player, iter2->item_id, iter2->amount);
                out.Insert(EO_EncodeNumber(server, iter2->item_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, iter2->amount, 4), out.Length() + 1);
                target->weight_current =
                    target->weight_current -
                    ItemValues::GetWeight(GUI->item_values, iter2->item_id) *
                        (unsigned int)iter2->amount;
                player->weight_current =
                    player->weight_current +
                    ItemValues::GetWeight(GUI->item_values, iter2->item_id) *
                        (unsigned int)iter2->amount;
                if (player->weight_current < 0)
                    player->weight_current = 0;
            }
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            player->trade_items.clear();
            player->read_len = -1;
            player->trade_accepted = 0;
            player->trade_value = 100;
            target->trade_items.clear();
            target->read_len = -1;
            target->trade_accepted = 0;
            target->trade_value = 100;
            Client_SendEncoded(server, player, PacketAction_Use, PacketFamily_Trade, out);
            Client_SendEncoded(server, target, PacketAction_Use, PacketFamily_Trade, out);
            String out2 = EO_EncodeNumber(server, player->player_id, 2);
            out2.Insert(EO_EncodeNumber(server, 0xc, 1), out2.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Player, PacketFamily_Emote, out2);
            out2 = EO_EncodeNumber(server, target->player_id, 2);
            out2.Insert(EO_EncodeNumber(server, 0xc, 1), out2.Length() + 1);
            Server_BroadcastNearby(
                server, target, PacketAction_Player, PacketFamily_Emote, out2);
            Player_FireQuestTriggers(server, player, 0x190, 0);
            Player_FireQuestTriggers(server, target, 0x190, 0);
            return true;
        }
        if (action == PacketAction_Close)
        {
            if (!player->logged_in)
                return false;
            Player *target = server->players->by_id[player->read_len];
            if (player->read_len < 1)
                return true;
            if (target == NULL)
                return true;
            player->trade_items.clear();
            target->trade_items.clear();
            player->read_len = -1;
            target->read_len = -1;
            player->trade_accepted = 0;
            player->trade_value = 100;
            Client_SendEncoded(server,
                               target,
                               PacketAction_Close,
                               PacketFamily_Trade,
                               EO_EncodeNumber(server, player->player_id, 2));
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 3)
                return false;
            int type = EO_DecodeNumber(server, String(data[1]));
            int target_id = EO_DecodeNumber(server, data.SubString(2, 2));
            if (type != 0x8a)
                return true;
            Player *target = server->players->by_id[target_id];
            if (target == NULL)
                return true;
            if (player->map_id == Settings::GetJailMap(server->settings))
                return false;
            if (player->map_id != target->map_id)
                return true;
            if (target->trade_accepted != 0)
            {
                if (target->read_len == player->player_id)
                {
                    Banned::AddBan(
                        server->banned, player->remote_ip, player->hdid, false, 0x3840);
                    return false;
                }
                return true;
            }
            player->read_len = target_id;
            player->trade_accepted = 0;
            player->trade_value = 100;
            player->trade_items.clear();
            String out = EO_EncodeNumber(server, type, 1);
            out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
            out.Insert(player->name, out.Length() + 1);
            Client_SendEncoded(
                server, target, PacketAction_Request, PacketFamily_Trade, out);
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 3)
                return false;
            int type = EO_DecodeNumber(server, String(data[1]));
            int target_id = EO_DecodeNumber(server, data.SubString(2, 2));
            Player *target = server->players->by_id[target_id];
            if (target == NULL)
                return true;
            if (player->map_id != target->map_id)
                return true;
            if (type != 0)
                return true;
            if (target->read_len != player->player_id)
                return true;
            player->read_len = target->player_id;
            player->trade_accepted = 0;
            target->trade_accepted = 0;
            player->trade_value = 100;
            player->trade_items.clear();
            target->trade_items.clear();
            String out = EO_EncodeNumber(server, player->player_id, 2);
            out.Insert(player->name, out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, target->player_id, 2), out.Length() + 1);
            out.Insert(target->name, out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Open, PacketFamily_Trade, out);
            Client_SendEncoded(
                server, target, PacketAction_Open, PacketFamily_Trade, out);
            return true;
        }
    }
    if (family == PacketFamily_Party)
    {
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 1)
                return false;
            if (!player->in_party)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Close,
                                   PacketFamily_Party,
                                   String(EO_GetBreakByte(server, EO_BREAK_BYTE)));
                return true;
            }
            int count = player->CountPartyMembers();
            if (EO_DecodeNumber(server, String(data[1])) != count)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_List,
                                   PacketFamily_Party,
                                   Party_EncodeMemberList(server, player));
            }
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 3)
                return false;
            int type = EO_DecodeNumber(server, String(data[1]));
            int id = EO_DecodeNumber(server, data.SubString(2, 2));
            Player *target = Players::Players_GetById(server->players, id);
            if (target == NULL)
                return true;
            if (type == 0)
            {
                if (player->in_party && target->in_party &&
                    player->party_leader_id == target->party_leader_id)
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Party,
                        EO_EncodeNumber(server, PartyReplyCode_AlreadyInYourParty, 1) +
                            target->name);
                    return true;
                }
                if (target->in_party && target->CountPartyMembers() + 1 >=
                                            Settings::GetGroupMax(server->settings))
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Party,
                        EO_EncodeNumber(server, PartyReplyCode_PartyIsFull, 1));
                    return true;
                }
                player->read_pos = target->player_id;
                String msg = EO_EncodeNumber(server, type, 1);
                msg.Insert(EO_EncodeNumber(server, player->player_id, 2),
                           msg.Length() + 1);
                msg.Insert(player->name, msg.Length() + 1);
                Client_SendEncoded(
                    server, target, PacketAction_Request, PacketFamily_Party, msg);
                return true;
            }
            if (type == 1)
            {
                if (target->in_party)
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Party,
                        EO_EncodeNumber(server, PartyReplyCode_AlreadyInAnotherParty, 1) +
                            target->name);
                    return true;
                }
                if (player->in_party && player->CountPartyMembers() + 1 >=
                                            Settings::GetGroupMax(server->settings))
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Party,
                        EO_EncodeNumber(server, PartyReplyCode_PartyIsFull, 1));
                    return true;
                }
                player->read_pos = target->player_id;
                String msg = EO_EncodeNumber(server, type, 1);
                msg.Insert(EO_EncodeNumber(server, player->player_id, 2),
                           msg.Length() + 1);
                msg.Insert(player->name, msg.Length() + 1);
                Client_SendEncoded(
                    server, target, PacketAction_Request, PacketFamily_Party, msg);
                return true;
            }
            return false;
        }
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (!player->in_party)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Close,
                                   PacketFamily_Party,
                                   String(EO_GetBreakByte(server, EO_BREAK_BYTE)));
                return true;
            }
            if (data.Length() < 2)
                return false;
            Player *target = Players::Players_GetById(
                server->players, EO_DecodeNumber(server, data.SubString(1, 2)));
            if (target == NULL)
                return false;
            if (!target->in_party)
                return false;
            if (player->party_leader_id != target->party_leader_id)
                return false;
            Server_BroadcastToParty(server,
                                    player,
                                    PacketAction_Remove,
                                    PacketFamily_Party,
                                    EO_EncodeNumber(server, target->player_id, 2));
            Players::Player_LeaveParty(server->players, target);
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 3)
                return false;
            int type = EO_DecodeNumber(server, String(data[1]));
            int id = EO_DecodeNumber(server, data.SubString(2, 2));
            Player *target = Players::Players_GetById(server->players, id);
            if (target == NULL)
                return true;
            if (target->read_pos != player->player_id)
                return true;
            if (type == 0)
            {
                if (target->in_party)
                {
                    Server_BroadcastToPartyExceptSelf(
                        server,
                        target,
                        PacketAction_Remove,
                        PacketFamily_Party,
                        EO_EncodeNumber(server, target->player_id, 2));
                    Players::Player_LeaveParty(server->players, target);
                }
                if (player->in_party)
                {
                    String msg = EO_EncodeNumber(server, target->player_id, 2);
                    msg.Insert(EO_EncodeNumber(server, 0, 1), msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, target->level, 1),
                               msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, Player::HpPercent(target), 1),
                               msg.Length() + 1);
                    msg.Insert(target->name, msg.Length() + 1);
                    Server_BroadcastToParty(
                        server, player, PacketAction_Add, PacketFamily_Party, msg);
                    Players::Party_AddNewMember(server->players, player, target);
                    Client_SendEncoded(server,
                                       target,
                                       PacketAction_Create,
                                       PacketFamily_Party,
                                       Party_EncodeMemberList(server, target));
                    return true;
                }
                else
                {
                    Player::AddPartyMember(player, player->player_id);
                    Player::AddPartyMember(player, target->player_id);
                    Player::AddPartyMember(target, player->player_id);
                    Player::AddPartyMember(target, target->player_id);
                    player->party_leader_id = player->player_id;
                    target->party_leader_id = player->player_id;
                    player->in_party = 1;
                    target->in_party = 1;
                    Server_BroadcastToParty(server,
                                            player,
                                            PacketAction_Create,
                                            PacketFamily_Party,
                                            Party_EncodeMemberList(server, player));
                    return true;
                }
            }
            if (type == 1)
            {
                if (player->in_party)
                {
                    Server_BroadcastToPartyExceptSelf(
                        server,
                        player,
                        PacketAction_Remove,
                        PacketFamily_Party,
                        EO_EncodeNumber(server, player->player_id, 2));
                    Players::Player_LeaveParty(server->players, player);
                }
                if (target->in_party)
                {
                    String msg = EO_EncodeNumber(server, player->player_id, 2);
                    msg.Insert(EO_EncodeNumber(server, 0, 1), msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, player->level, 1),
                               msg.Length() + 1);
                    msg.Insert(EO_EncodeNumber(server, Player::HpPercent(player), 1),
                               msg.Length() + 1);
                    msg.Insert(player->name, msg.Length() + 1);
                    Server_BroadcastToParty(
                        server, target, PacketAction_Add, PacketFamily_Party, msg);
                    Players::Party_AddNewMember(server->players, target, player);
                    Client_SendEncoded(server,
                                       player,
                                       PacketAction_Create,
                                       PacketFamily_Party,
                                       Party_EncodeMemberList(server, player));
                    return true;
                }
                else
                {
                    Player::AddPartyMember(player, target->player_id);
                    Player::AddPartyMember(player, player->player_id);
                    Player::AddPartyMember(target, target->player_id);
                    Player::AddPartyMember(target, player->player_id);
                    player->party_leader_id = target->player_id;
                    target->party_leader_id = target->player_id;
                    player->in_party = 1;
                    target->in_party = 1;
                    Server_BroadcastToParty(server,
                                            target,
                                            PacketAction_Create,
                                            PacketFamily_Party,
                                            Party_EncodeMemberList(server, target));
                    return true;
                }
            }
            return false;
        }
    }
    if (family == PacketFamily_Guild)
    {
        if (action == PacketAction_Buy)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 8)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            int gold = EO_DecodeNumber(server, data.SubString(5, 4));
            if (player->session_token != session_id)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            if (player->guild_tag.Length() < 2)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, 1, gold))
                return true;
            mySQLdb::Mysql_ExecDirect(server->mysql_controls,
                                      0,
                                      "UPDATE endl_guilds SET money = money + " +
                                          IntToStr(player->item_change_count) +
                                          " WHERE tag = '" + player->guild_tag + "'");
            Client_SendEncoded(server,
                               player,
                               PacketAction_Buy,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, player->item_change_remaining, 4));
            return true;
        }
        if (action == PacketAction_Rank)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 9)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            int rank = EO_DecodeNumber(server, data.SubString(5, 1));
            String member_name = data.SubString(6, data.Length() - 5);
            if (rank < 1 || rank > 9)
                return true;
            if (player->session_token != session_id)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            if (player->guild_tag.Length() < 2)
                return true;
            if (player->guild_rank_id != 1)
                return true;
            String rank_field = "rank" + IntToStr(rank);
            mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                       0x47,
                                       player->player_id,
                                       player->query_id,
                                       data,
                                       "SELECT " + rank_field +
                                           " FROM endl_guilds WHERE tag = '" +
                                           player->guild_tag + "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Kick)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 7)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            String member_name = data.SubString(5, data.Length() - 4);
            if (player->session_token != session_id)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            if (player->guild_tag.Length() < 2)
                return true;
            if (player->guild_rank_id != 1)
                return true;
            Player *member = Players::Players_FindByName(server->players, member_name);
            if (member == NULL)
            {
                mySQLdb::Mysql_SubmitQuery(
                    server->mysql_controls,
                    0x49,
                    player->player_id,
                    player->query_id,
                    data,
                    "SELECT ident_guild, ident_rank FROM endl_characters WHERE "
                    "name = '" +
                        member_name + "' AND ident_guild = '" + player->guild_tag +
                        "' LIMIT 1");
                return true;
            }
            if (member->guild_tag != player->guild_tag)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_RemoveLeader, 2));
                return true;
            }
            if (member->guild_rank_id == 1)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Guild,
                    EO_EncodeNumber(server, GuildReply_RemoveNotMember, 2));
                return true;
            }
            member->guild_rank_id = 9;
            member->guild_tag = "0";
            member->guild_name = "";
            member->guild_rank_name = "";
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_Removed, 2));
            return true;
        }
        if (action == PacketAction_Junk)
            return true;
        if (action == PacketAction_Agree)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 7)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            int info_type = EO_DecodeNumber(server, data.SubString(5, 2));
            String rest = data.SubString(7, data.Length() - 6);
            if (player->session_token != session_id)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            if (player->guild_tag.Length() < 2)
                return true;
            if (player->guild_rank_id != 1)
                return true;
            if (info_type == GuildInfoType_Description)
            {
                mySQLdb::Mysql_ExecDirect(
                    server->mysql_controls,
                    0,
                    "UPDATE endl_guilds SET description = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls, rest, 0) +
                        "' WHERE tag = '" + player->guild_tag + "'");
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Updated, 2));
                return true;
            }
            if (info_type == GuildInfoType_Ranks)
            {
                PacketReader_Init(server, rest, EO_GetBreakByte(server, EO_BREAK_BYTE));
                String out = "UPDATE endl_guilds SET ";
                out.Insert(
                    "rank1 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank2 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank3 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank4 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank5 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank6 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank7 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank8 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "',",
                    out.Length() + 1);
                out.Insert(
                    "rank9 = '" +
                        mySQLdb::Mysql_SanitizeString(server->mysql_controls,
                                                      PacketReader_GetBreakString(server),
                                                      0) +
                        "' ",
                    out.Length() + 1);
                out.Insert("WHERE tag = '" + player->guild_tag + "'", out.Length() + 1);
                mySQLdb::Mysql_ExecDirect(server->mysql_controls, 0, out);
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_RanksUpdated, 2));
                return true;
            }
        }
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            int info_type = EO_DecodeNumber(server, data.SubString(5, 2));
            if (player->session_token != session_id)
                return true;
            if (player->guild_tag.Length() < 2)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            if (info_type == GuildInfoType_Description)
            {
                if (player->guild_rank_id != 1)
                    return true;
                mySQLdb::Mysql_SubmitQuery(
                    server->mysql_controls,
                    0x4a,
                    player->player_id,
                    player->query_id,
                    data,
                    "SELECT description FROM endl_guilds WHERE tag = '" +
                        player->guild_tag + "' LIMIT 1");
                return true;
            }
            if (info_type == GuildInfoType_Ranks)
            {
                mySQLdb::Mysql_SubmitQuery(
                    server->mysql_controls,
                    0x4b,
                    player->player_id,
                    player->query_id,
                    data,
                    "SELECT rank1,rank2,rank3,rank4,rank5,rank6,rank7,rank8,rank9 "
                    "FROM endl_guilds WHERE tag = '" +
                        player->guild_tag + "' LIMIT 1");
                return true;
            }
            if (info_type == GuildInfoType_Bank)
            {
                mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                           0x4c,
                                           player->player_id,
                                           player->query_id,
                                           data,
                                           "SELECT money FROM endl_guilds WHERE tag = '" +
                                               player->guild_tag + "' LIMIT 1");
            }
            return true;
        }
        if (action == PacketAction_Tell)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_id)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            String guild = mySQLdb::Db_SanitizeString(
                server->mysql_controls, data.SubString(5, data.Length() - 4));
            if (guild.Length() < 2)
                return true;
            if (guild.Length() > 3)
                return true;
            String query = "SELECT ident_rank, name, rank FROM endl_characters WHERE "
                           "ident_guild = '" +
                           guild + "' ORDER BY ident_rank, name LIMIT 100";
            mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                       0x4d,
                                       player->player_id,
                                       player->query_id,
                                       data,
                                       query);
            return true;
        }
        if (action == PacketAction_Report)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_id)
                return true;
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            String guild = mySQLdb::Db_SanitizeString(
                server->mysql_controls, data.SubString(5, data.Length() - 4));
            if (guild.Length() < 2)
                return true;
            if (guild.Length() > 3)
                return true;
            String query =
                "SELECT * FROM endl_guilds WHERE tag = '" + guild + "' LIMIT 1";
            mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                       0x4e,
                                       player->player_id,
                                       player->query_id,
                                       data,
                                       query);
            return true;
        }
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_id)
                return true;
            player->guild_tag = "";
            player->guild_name = "";
            player->guild_rank_id = 9;
            player->guild_rank_name = "";
            player->guild_inviter_id = -1;
            return true;
        }
        if (action == PacketAction_Player)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 0xa)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_id)
                return true;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String guild = mySQLdb::Db_SanitizeString(
                server->mysql_controls, PacketReader_GetBreakString(server));
            String recruiter = mySQLdb::Db_SanitizeString(
                server->mysql_controls, PacketReader_GetBreakString(server));
            if (player->guild_tag.Length() > 1)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_AlreadyMember, 2));
                return true;
            }
            Player *target = Players::Players_FindByName(server->players, recruiter);
            if (target == NULL)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Guild,
                    EO_EncodeNumber(server, GuildReply_RecruiterOffline, 2));
                return true;
            }
            if (target->map_id != player->map_id)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Guild,
                    EO_EncodeNumber(server, GuildReply_RecruiterNotHere, 2));
                return true;
            }
            if (target->guild_tag.Length() < 2)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Guild,
                    EO_EncodeNumber(server, GuildReply_RecruiterWrongGuild, 2));
                return true;
            }
            if (LowerCase(target->guild_tag) != LowerCase(guild))
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Guild,
                    EO_EncodeNumber(server, GuildReply_RecruiterWrongGuild, 2));
                return true;
            }
            if (target->guild_rank_id > 2)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_NotRecruiter, 2));
                return true;
            }
            player->field_0x8c = target->name;
            String msg = EO_EncodeNumber(server, GuildReply_JoinRequest, 2);
            msg.Insert(EO_EncodeNumber(server, player->player_id, 2), msg.Length() + 1);
            msg.Insert(player->name, msg.Length() + 1);
            Client_SendEncoded(
                server, target, PacketAction_Reply, PacketFamily_Guild, msg);
            return true;
        }
        if (action == PacketAction_Create)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 0xd)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_id)
                return true;
            if (session_id < 0x493e0 || session_id > 0x61a80)
                return true;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String tag_upper = AnsiUpperCase(mySQLdb::Db_SanitizeString(
                server->mysql_controls, PacketReader_GetBreakString(server)));
            String name = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
            String description = mySQLdb::Db_SanitizeString(
                server->mysql_controls, PacketReader_GetBreakString(server));
            name = mySQLdb::Mysql_SanitizeString(server->mysql_controls, name, 0);
            tag_upper =
                mySQLdb::Mysql_SanitizeString(server->mysql_controls, tag_upper, 1);
            if (tag_upper.Length() < 2 || tag_upper.Length() > 3)
                return true;
            if (name.Length() < 4 || name.Length() > 0x18)
                return true;
            if (player->guild_inviter_id != player->player_id)
                return true;
            player->guild_inviter_id = -1;
            if (player->guild_tag.Length() > 1)
                return true;
            if (tag_upper == "GM" || tag_upper == "HGM" || tag_upper == "GOD" ||
                tag_upper == "ADM" || tag_upper == "SUK" || tag_upper == "SUX" ||
                tag_upper == "ASS" || tag_upper == "FUK" || tag_upper == "BRA" ||
                tag_upper == "FUC" || tag_upper == "SEX" || tag_upper == "CUM" ||
                tag_upper == "HOE" || tag_upper == "TIT" || tag_upper == "HO" ||
                tag_upper == "FU" || tag_upper == "KKK" || tag_upper == "XXX")
                return true;
            if (!CharName_CheckUnique(server, name))
                return true;
            String tag_first = tag_upper[1];
            String name_first = name[1];
            if (LowerCase(tag_first) != LowerCase(name_first))
                return true;
            if (tag_upper[1] == ' ' || tag_upper[2] == ' ')
                return true;
            if (!mySQLdb::IsAlphabeticText(server->mysql_controls, tag_upper))
            {
                Banned::AddBan(
                    server->banned, player->remote_ip, player->hdid, (char)0, 0x3840);
                return false;
            }
            mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                       0x51,
                                       player->player_id,
                                       player->query_id,
                                       data,
                                       "SELECT name FROM endl_guilds WHERE name = '" +
                                           name + "' OR tag = '" + tag_upper + "'");
            return true;
        }
        if (action == PacketAction_Use)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 2)
                return false;
            if (player->guild_tag.Length() < 2)
                return true;
            if (player->guild_rank_id > 2)
                return true;
            Player *target = Players::Players_GetById(
                server->players, EO_DecodeNumber(server, data.SubString(1, 2)));
            if (target == NULL)
                return true;
            if (target->guild_tag.Length() > 1)
                return true;
            if (target->map_id != player->map_id)
                return true;
            if (player->name != target->field_0x8c)
                return true;
            mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                       0x52,
                                       player->player_id,
                                       player->query_id,
                                       data,
                                       "SELECT * FROM endl_guilds WHERE tag = '" +
                                           player->guild_tag + "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 6)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            int inviter_id = EO_DecodeNumber(server, data.SubString(5, 2));
            if (session_id != 0x4eea)
                return false;
            if (player->guild_tag.Length() > 1)
                return true;
            Player *inviter = Players::Players_GetById(server->players, inviter_id);
            if (inviter == NULL)
                return true;
            if (inviter->player_id == player->player_id)
                return true;
            if (inviter->map_id != player->map_id)
                return true;
            int invites = Players::Players_CountGuildInvites(server->players, inviter);
            if (invites < 0xa)
            {
                player->guild_inviter_id = inviter_id;
                if (invites == 9)
                {
                    Client_SendEncoded(
                        server,
                        inviter,
                        PacketAction_Reply,
                        PacketFamily_Guild,
                        EO_EncodeNumber(server, GuildReply_CreateAddConfirm, 2) +
                            player->name);
                    return true;
                }
                Client_SendEncoded(server,
                                   inviter,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_CreateAdd, 2) +
                                       player->name);
            }
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 0xd)
                return false;
            int session_id = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_id)
                return true;
            if (session_id < 0x493e0 || session_id > 0x61a80)
                return true;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String tag = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                    PacketReader_GetBreakString(server));
            String name = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
            if (tag.Length() < 2 || tag.Length() > 3)
                return true;
            if (name.Length() < 4 || name.Length() > 0x18)
                return true;
            if (player->guild_tag.Length() > 1)
                return true;
            if (tag == "GM" || tag == "HGM" || tag == "GOD" || tag == "ADM" ||
                tag == "SUK" || tag == "SUX" || tag == "ASS" || tag == "FUK" ||
                tag == "BRA" || tag == "FUC" || tag == "SEX" || tag == "CUM" ||
                tag == "HOE" || tag == "TIT" || tag == "HO" || tag == "FU" ||
                tag == "KKK" || tag == "XXX")
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_NotApproved, 2));
                return true;
            }
            if (!CharName_CheckUnique(server, name))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_NotApproved, 2));
                return true;
            }
            String tag_first = tag[1];
            String name_first = name[1];
            if (LowerCase(tag_first) != LowerCase(name_first))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_NotApproved, 2));
                return true;
            }
            if (tag[1] == ' ' || tag[2] == ' ')
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_NotApproved, 2));
                return true;
            }
            if (Players::Players_CountGuildOnMap(server->players, player) < 0xa)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_NoCandidates, 2));
                return true;
            }
            if (!Server_TickOncePerFiveSeconds(server))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Guild,
                                   EO_EncodeNumber(server, GuildReply_Busy, 2));
                return true;
            }
            mySQLdb::Mysql_SubmitQuery(server->mysql_controls,
                                       0x50,
                                       player->player_id,
                                       player->query_id,
                                       data,
                                       "SELECT name FROM endl_guilds WHERE name = '" +
                                           name + "' OR tag = '" + tag + "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return true;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Guild)
                return true;
            player->session_token = RandRange(SESSION_TOKEN_SPAN) + SESSION_BASE_GUILD;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Open,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, player->session_token, 3));
            return true;
        }
    }
    if (family == PacketFamily_Quest)
    {
        if (action == PacketAction_List)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() != 1)
                return false;
            int type = EO_DecodeNumber(server, data);
            if (type == 1)
            {
                String out = EO_EncodeNumber(server, 1, 1);
                out.Insert(EO_EncodeNumber(server, player->quest_trackers.size(), 2),
                           out.Length() + 1);
                PlayerQuest *iter;
                for (iter = player->quest_trackers.begin();
                     iter != player->quest_trackers.end();
                     iter++)
                {
                    QuestState *state = QuestContainer::GetState(
                        server->quest_engine, iter->quest_id, iter->state_index);
                    if (state != NULL)
                    {
                        out.Insert(QuestContainer::GetQuestName(server->quest_engine,
                                                                iter->quest_id),
                                   out.Length() + 1);
                        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                   out.Length() + 1);
                        out.Insert(state->description, out.Length() + 1);
                        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                   out.Length() + 1);
                        int cond = state->fast_dispatch_condition_type;
                        int v1 = 0;
                        int v2 = 0;
                        if (state->fast_dispatch_rule_index < 5 &&
                            state->fast_dispatch_condition_type > 0 &&
                            state->fast_dispatch_condition_type < 10)
                        {
                            v2 = iter->counters[state->fast_dispatch_rule_index];
                            v1 = state->rules[state->fast_dispatch_rule_index]->args[1];
                            if (state->fast_dispatch_condition_type == 9)
                                v1 = state->rules[state->fast_dispatch_rule_index]
                                         ->args[0];
                            if (v1 < 1)
                                v1 = 1;
                        }
                        out.Insert(EO_EncodeNumber(server, cond, 2), out.Length() + 1);
                        out.Insert(EO_EncodeNumber(server, v2, 2), out.Length() + 1);
                        out.Insert(EO_EncodeNumber(server, v1, 2), out.Length() + 1);
                        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                   out.Length() + 1);
                    }
                }
                Client_SendEncoded(
                    server, player, PacketAction_List, PacketFamily_Quest, out);
                return true;
            }
            if (type == 2)
            {
                String out = EO_EncodeNumber(server, 2, 1);
                out.Insert(EO_EncodeNumber(server, player->quest_trackers.size(), 2),
                           out.Length() + 1);
                PlayerQuest *iter;
                for (iter = player->quest_history.begin();
                     iter != player->quest_history.end();
                     iter++)
                {
                    out.Insert(QuestContainer::GetQuestName(server->quest_engine,
                                                            iter->quest_id),
                               out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                }
                Client_SendEncoded(
                    server, player, PacketAction_List, PacketFamily_Quest, out);
                return true;
            }
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 0xa)
                return false;
            int session = EO_DecodeNumber(server, data.SubString(1, 2));
            int token = EO_DecodeNumber(server, data.SubString(3, 2));
            int quest_id = EO_DecodeNumber(server, data.SubString(5, 2));
            int rule = EO_DecodeNumber(server, data.SubString(7, 2));
            int type = EO_DecodeNumber(server, data.SubString(9, 1));
            int arg = EO_DecodeNumber(server, data.SubString(0xa, 1));
            int result = -1;
            if (player->session_id != session)
                return false;
            if (player->session_token != token)
                return false;
            if (player->field_0x80 != rule)
                return false;
            player->session_token = RandRange(SESSION_TOKEN_SPAN);
            PlayerQuest *iter;
            for (iter = player->quest_trackers.begin();
                 iter != player->quest_trackers.end();
                 iter++)
            {
                if (iter->quest_id != quest_id)
                    continue;
                int value = -1;
                if (type == 1)
                    value = QuestContainer::GetRuleValue(
                        server->quest_engine, iter->quest_id, iter->state_index, rule);
                if (type == 2)
                    value = QuestContainer::GetRuleValue2(
                        server->quest_engine, iter->quest_id, iter->state_index, arg);
                if (value < 0)
                    continue;
                iter->state_index = (short)value;
                Player_ApplyQuestActions(server, player, iter, true);
                if (iter->done != 0)
                {
                    player->quest_trackers.erase(iter);
                    break;
                }
                if (player->map_id <= 0)
                    break;
                {
                    String out = QuestContainer::GetActionData2(
                        server->quest_engine, iter->quest_id, iter->state_index, rule);
                    if (out.Length() < 1)
                        break;
                    player->session_id = RandRange(0xc350) + 0x2710;
                    player->session_token = RandRange(0x7530) + SESSION_BASE_QUEST;
                    player->field_0x80 = rule;
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), 1);
                    out.Insert(QuestContainer::GetQuestName(server->quest_engine,
                                                            iter->quest_id),
                               1);
                    out.Insert(EO_EncodeNumber(server, iter->quest_id, 2), 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), 1);
                    out.Insert(EO_EncodeNumber(server, player->session_token, 2), 1);
                    out.Insert(EO_EncodeNumber(server, player->session_id, 2), 1);
                    out.Insert(EO_EncodeNumber(server, iter->quest_id, 2), 1);
                    out.Insert(EO_EncodeNumber(server, rule, 2), 1);
                    out.Insert(EO_EncodeNumber(server, 1, 1), 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Dialog, PacketFamily_Quest, out);
                }
                break;
            }
            return true;
        }
        if (action == PacketAction_Use)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            int quest_filter = EO_DecodeNumber(server, data.SubString(3, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return true;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Quest)
                return true;
            if (QuestContainer::GetQuestLoaded(server->quest_engine,
                                               type_info.behavior_id))
            {
                bool found = true;
                PlayerQuest *iter;
                for (iter = player->quest_history.begin();
                     iter != player->quest_history.end();
                     iter++)
                    if (iter->quest_id == type_info.behavior_id)
                    {
                        found = false;
                        break;
                    }
                for (iter = player->quest_trackers.begin();
                     iter != player->quest_trackers.end();
                     iter++)
                    if (iter->quest_id == type_info.behavior_id)
                    {
                        found = false;
                        break;
                    }
                if (found && player->quest_trackers.size() < 10)
                {
                    int version = QuestContainer::GetQuestVersion(server->quest_engine,
                                                                  type_info.behavior_id);
                    PlayerQuest tracker(type_info.behavior_id, 0, version);
                    player->quest_trackers.insert(player->quest_trackers.end(), tracker);
                }
            }
            String out;
            String name;
            String payload;
            int count = 0;
            PlayerQuest *iter;
            for (iter = player->quest_trackers.begin();
                 iter != player->quest_trackers.end();
                 iter++)
            {
                if (iter->quest_id == quest_filter || quest_filter == 0)
                {
                    if (name.Length() == 0)
                    {
                        if (Mapcontrol_TryTakeQuestCooldown(server->map_control,
                                                            player->map_id))
                        {
                            name = QuestContainer::GetActionData(server->quest_engine,
                                                                 iter->quest_id,
                                                                 iter->state_index,
                                                                 type_info.behavior_id);
                            if (name.Length() > 0)
                            {
                                name.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), 1);
                                name.Insert(EO_EncodeNumber(server, npc_index, 2), 1);
                                Server_BroadcastNearTile(server,
                                                         -1,
                                                         player->map_id,
                                                         coords,
                                                         PacketAction_Report,
                                                         PacketFamily_Quest,
                                                         name);
                            }
                        }
                    }
                    if (out.Length() == 0)
                    {
                        out = QuestContainer::GetActionData2(server->quest_engine,
                                                             iter->quest_id,
                                                             iter->state_index,
                                                             type_info.behavior_id);
                        if (out.Length() > 0)
                        {
                            player->session_id = RandRange(0xc350) + 0x2710;
                            player->session_token =
                                RandRange(0x7530) + SESSION_BASE_QUEST;
                            player->field_0x80 = type_info.behavior_id;
                            payload = EO_EncodeNumber(server, type_info.behavior_id, 2);
                            payload.Insert(EO_EncodeNumber(server, iter->quest_id, 2),
                                           payload.Length() + 1);
                            payload.Insert(EO_EncodeNumber(server, player->session_id, 2),
                                           payload.Length() + 1);
                            payload.Insert(
                                EO_EncodeNumber(server, player->session_token, 2),
                                payload.Length() + 1);
                            payload.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                           payload.Length() + 1);
                            payload.Insert(EO_EncodeNumber(server, iter->quest_id, 2),
                                           payload.Length() + 1);
                            payload.Insert(QuestContainer::GetQuestName(
                                               server->quest_engine, iter->quest_id),
                                           payload.Length() + 1);
                            payload.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                           payload.Length() + 1);
                            count++;
                        }
                    }
                    else
                    {
                        if (quest_filter != 0)
                            continue;
                        if (QuestContainer::GetActionData2(server->quest_engine,
                                                           iter->quest_id,
                                                           iter->state_index,
                                                           type_info.behavior_id) != "")
                        {
                            payload.Insert(EO_EncodeNumber(server, iter->quest_id, 2),
                                           payload.Length() + 1);
                            payload.Insert(QuestContainer::GetQuestName(
                                               server->quest_engine, iter->quest_id),
                                           payload.Length() + 1);
                            payload.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                           payload.Length() + 1);
                            count++;
                        }
                    }
                }
            }
            if (out.Length() > 0)
            {
                payload.Insert(EO_EncodeNumber(server, count, 1), 1);
                out.Insert(payload, 1);
                Client_SendEncoded(
                    server, player, PacketAction_Dialog, PacketFamily_Quest, out);
            }
            return true;
        }
    }
    if (family == PacketFamily_StatSkill)
    {
        if (action == PacketAction_Take)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int session_token = EO_DecodeNumber(server, data.SubString(1, 4));
            int spell_id = EO_DecodeNumber(server, data.SubString(5, 2));
            if (player->session_token != session_token)
                return true;
            if (!LearnValues::HasSkill(GUI->learn_values, session_token, spell_id))
                return true;
            if (Players::Player_HasSpellId(server->players, player, spell_id))
                return true;
            LearnItemVal skill =
                LearnValues::GetSkill(GUI->learn_values, session_token, spell_id);
            if (!ClassValues::ClassMatches(
                    GUI->class_values, player->class_id, skill.class_requirement))
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_StatSkill,
                    EO_EncodeNumber(server, SkillMasterReply_WrongClass, 2) +
                        EO_EncodeNumber(server, player->class_id, 1));
                return true;
            }
            if (player->level < skill.level_requirement)
                return true;
            if (player->adj_strength < skill.str_requirement)
                return true;
            if (player->adj_intelligence < skill.wis_requirement)
                return true;
            if (player->adj_wisdom < skill.int_requirement)
                return true;
            if (player->adj_agility < skill.agi_requirement)
                return true;
            if (player->adj_constitution < skill.con_requirement)
                return true;
            if (player->adj_charisma < skill.cha_requirement)
                return true;
            if (skill.skill_requirement_1 > 0 &&
                !Players::Player_HasSpellId(
                    server->players, player, skill.skill_requirement_1))
                return true;
            if (skill.skill_requirement_2 > 0 &&
                !Players::Player_HasSpellId(
                    server->players, player, skill.skill_requirement_2))
                return true;
            if (skill.skill_requirement_3 > 0 &&
                !Players::Player_HasSpellId(
                    server->players, player, skill.skill_requirement_3))
                return true;
            if (skill.skill_requirement_4 > 0 &&
                !Players::Player_HasSpellId(
                    server->players, player, skill.skill_requirement_4))
                return true;
            if (!Players::Player_RemoveItem(server->players, player, 1, skill.price))
                return true;
            Players::Player_AddSpell(server->players, player, spell_id);
            String out = EO_EncodeNumber(server, spell_id, 2);
            out.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                       out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Take, PacketFamily_StatSkill, out);
            return true;
        }
        if (action == PacketAction_Junk)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int session_token = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != session_token)
                return true;
            if (player->boots_graphic_id != 0 || player->accessory_graphic_id != 0 ||
                player->gloves_graphic_id != 0 || player->armor_graphic_id != 0 ||
                player->belt_graphic_id != 0 || player->necklace_graphic_id != 0 ||
                player->hat_graphic_id != 0 || player->shield_graphic_id != 0 ||
                player->weapon_graphic_id != 0 || player->ring1_graphic_id != 0 ||
                player->ring2_graphic_id != 0 || player->armlet1_graphic_id != 0 ||
                player->armlet2_graphic_id != 0 || player->bracer1_graphic_id != 0 ||
                player->bracer2_graphic_id != 0)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_StatSkill,
                    EO_EncodeNumber(server, SkillMasterReply_RemoveItems, 2));
                return true;
            }
            player->base_strength = 0;
            player->base_wisdom = 0;
            player->base_intelligence = 0;
            player->base_agility = 0;
            player->base_constitution = 0;
            player->base_charisma = 0;
            Players::Player_ClearSpells(server->players, player);
            player->stat_points = player->level * 3;
            player->skill_points = player->level * 4;
            if (player->hp > player->max_hp)
                player->hp = player->max_hp;
            if (player->tp > player->max_tp)
                player->tp = player->max_tp;
            Player::UpdateBaseStats(player);
            Player_CalculateStats(server, player);
            Player::CalculateHP_TP_SP(player);
            String out = EO_EncodeNumber(server, player->stat_points, 2);
            out.Insert(EO_EncodeNumber(server, player->skill_points, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->hp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->tp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_sp, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_strength, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_intelligence, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_wisdom, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_agility, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_constitution, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->adj_charisma, 2),
                       out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->min_damage, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->max_damage, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->accuracy, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->evasion, 2), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->armor, 2), out.Length() + 1);
            player->base_stats_dirty = true;
            Client_SendEncoded(
                server, player, PacketAction_Junk, PacketFamily_StatSkill, out);
            return true;
        }
        if (action == PacketAction_Remove)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 6)
                return false;
            int session_token = EO_DecodeNumber(server, data.SubString(1, 4));
            int spell_id = EO_DecodeNumber(server, data.SubString(5, 2));
            if (player->session_token != session_token)
                return true;
            if (Players::Player_RemoveSpell(server->players, player, spell_id))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Remove,
                                   PacketFamily_StatSkill,
                                   EO_EncodeNumber(server, spell_id, 2));
            }
            return true;
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return true;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Trainer)
                return true;
            int behavior_id = type_info.behavior_id;
            player->session_token = type_info.behavior_id;
            Client_SendEncoded(
                server,
                player,
                PacketAction_Open,
                PacketFamily_StatSkill,
                LearnValues::BuildOpenData(GUI->learn_values, type_info.behavior_id));
            return true;
        }
        if (action == PacketAction_Add)
        {
            if (data.Length() != 3)
                return false;
            int action_type = EO_DecodeNumber(server, data.SubString(1, 1));
            int stat_id = EO_DecodeNumber(server, data.SubString(2, 2));
            if (action_type == TrainType_Stat)
            {
                if (player->stat_points < 1)
                    return true;
                if (stat_id < StatId_Str || stat_id > StatId_Cha)
                    return true;
                player->stat_points--;
                if (stat_id == StatId_Str)
                    player->base_strength++;
                if (stat_id == StatId_Int)
                    player->base_intelligence++;
                if (stat_id == StatId_Wis)
                    player->base_wisdom++;
                if (stat_id == StatId_Agi)
                    player->base_agility++;
                if (stat_id == StatId_Con)
                    player->base_constitution++;
                if (stat_id == StatId_Cha)
                    player->base_charisma++;
                Player::UpdateBaseStats(player);
                Player_CalculateStats(server, player);
                Player::CalculateHP_TP_SP(player);
                String out = EO_EncodeNumber(server, player->stat_points, 2);
                out.Insert(EO_EncodeNumber(server, player->adj_strength, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_intelligence, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_wisdom, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_agility, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_constitution, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->adj_charisma, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_sp, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->weight_max, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->min_damage, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->max_damage, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->accuracy, 2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->evasion, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->armor, 2), out.Length() + 1);
                player->base_stats_dirty = true;
                Client_SendEncoded(
                    server, player, PacketAction_Player, PacketFamily_StatSkill, out);
                return true;
            }
            if (action_type == TrainType_Skill)
            {
                if (player->skill_points < 1)
                    return true;
                int spell_level =
                    Players::Player_GetSpellLevel(server->players, player, stat_id);
                if (spell_level < 0 || spell_level > 0x63)
                    return true;
                player->skill_points--;
                int new_level =
                    Players::Player_LevelUpSpell(server->players, player, stat_id);
                String out = EO_EncodeNumber(server, player->skill_points, 2);
                out.Insert(EO_EncodeNumber(server, stat_id, 2), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, new_level, 2), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Accept, PacketFamily_StatSkill, out);
                return true;
            }
        }
    }
    if (family == PacketFamily_Marriage)
    {
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 0xa)
                return false;
            int subtype = EO_DecodeNumber(server, data.SubString(1, 1));
            int token = EO_DecodeNumber(server, data.SubString(2, 4));
            if (player->session_token != token)
                return true;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String name = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
            name = LowerCase(name);
            name = mySQLdb::Mysql_SanitizeString(server->mysql_controls, name, false);
            if (name.Length() < 4)
                return false;
            if (subtype == 1)
            {
                if (player->partner_name.Length() > 3)
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Marriage,
                        EO_EncodeNumber(server, MarriageReply_AlreadyMarried, 2));
                    return true;
                }
                if (!Players::Player_RemoveItem(server->players, player, 1, 0x1f4))
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Marriage,
                        EO_EncodeNumber(server, MarriageReply_NotEnoughGold, 2));
                    return true;
                }
                player->partner_name = name.SubString(1, 3);
                String msg = EO_EncodeNumber(server, 3, 2);
                msg.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           msg.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Marriage, msg);
                return true;
            }
            if (subtype == 2)
            {
                if (player->partner_name.Length() < 4)
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Marriage,
                        EO_EncodeNumber(server, MarriageReply_NotMarried, 2));
                    return true;
                }
                if (LowerCase(player->partner_name) != LowerCase(name))
                {
                    Client_SendEncoded(
                        server,
                        player,
                        PacketAction_Reply,
                        PacketFamily_Marriage,
                        EO_EncodeNumber(server, MarriageReply_WrongName, 2));
                    return true;
                }
                Player *target = Players::Players_FindByName(server->players, name);
                if (target != NULL)
                {
                    if (!Players::Player_RemoveItem(server->players, player, 1, 0x2710))
                    {
                        Client_SendEncoded(
                            server,
                            player,
                            PacketAction_Reply,
                            PacketFamily_Marriage,
                            EO_EncodeNumber(server, MarriageReply_NotEnoughGold, 2));
                        return true;
                    }
                    Client_SendEncoded(
                        server,
                        target,
                        PacketAction_Reply,
                        PacketFamily_Marriage,
                        EO_EncodeNumber(server, MarriageReply_DivorceNotification, 2));
                    target->partner_name = "";
                }
                else
                {
                    if (!Server_TickOncePerFiveSeconds(server))
                    {
                        Client_SendEncoded(
                            server,
                            player,
                            PacketAction_Reply,
                            PacketFamily_Marriage,
                            EO_EncodeNumber(server, MarriageReply_ServiceBusy, 2));
                        return true;
                    }
                    if (!Players::Player_RemoveItem(server->players, player, 1, 0x2710))
                    {
                        Client_SendEncoded(
                            server,
                            player,
                            PacketAction_Reply,
                            PacketFamily_Marriage,
                            EO_EncodeNumber(server, MarriageReply_NotEnoughGold, 2));
                        return true;
                    }
                    mySQLdb::Mysql_ExecDirect(
                        server->mysql_controls,
                        player->account_ident,
                        "UPDATE endl_characters SET partner = 'DV-' WHERE name = '" +
                            name + "'");
                }
                player->partner_name = "";
                String msg = EO_EncodeNumber(server, 3, 2);
                msg.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                           msg.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Marriage, msg);
                return true;
            }
        }
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return true;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Lawyer)
                return true;
            player->session_token = RandRange(SESSION_TOKEN_SPAN) + SESSION_BASE_LAWYER;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Open,
                               PacketFamily_Marriage,
                               EO_EncodeNumber(server, player->session_token, 3));
            return true;
        }
    }
    if (family == PacketFamily_Priest)
    {
        if (action == PacketAction_Open)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            if (player->partner_name.Length() > 3)
                return true;
            int npc_index = EO_DecodeNumber(server, data.SubString(1, 2));
            MapCoord coords = Mapcontrol_GetNpcCoordsByIndex(
                server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return false;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            if (WeddingController::Has(GUI->weddings, player->map_id, npc_index))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Priest,
                                   EO_EncodeNumber(server, PriestReply_Busy, 2));
                return true;
            }
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Priest)
                return true;
            if (player->level < 5)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Priest,
                                   EO_EncodeNumber(server, PriestReply_LowLevel, 2));
                return true;
            }
            if (player->gender == 1 && player->armor_graphic_id != 0x15)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Priest,
                                   EO_EncodeNumber(server, PriestReply_NotDressed, 2));
                return true;
            }
            if (player->gender == 0 && player->armor_graphic_id != 2)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Priest,
                                   EO_EncodeNumber(server, PriestReply_NotDressed, 2));
                return true;
            }
            player->session_token = RandRange(SESSION_TOKEN_SPAN) + SESSION_BASE_PRIEST;
            player->npc_index = npc_index;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Open,
                               PacketFamily_Priest,
                               EO_EncodeNumber(server, player->session_token, 4));
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            int token = EO_DecodeNumber(server, data.SubString(1, 4));
            if (player->session_token != token)
                return true;
            if (token < 0xc3500 || token > 0xdbba0)
                return true;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            PacketReader_GetBreakString(server);
            String name = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
            if (name.Length() < 4 || name.Length() > 0x18)
                return true;
            Player *target = Players::Players_FindByName(server->players, name);
            if (target == NULL)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Priest,
                    EO_EncodeNumber(server, PriestReply_PartnerNotPresent, 2));
                return true;
            }
            if (target->player_id == player->player_id)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Priest,
                    EO_EncodeNumber(server, PriestReply_PartnerNotPresent, 2));
                return true;
            }
            if (target->map_id != player->map_id)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Priest,
                    EO_EncodeNumber(server, PriestReply_PartnerNotPresent, 2));
                return true;
            }
            if (target->partner_name.Length() > 3)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Priest,
                    EO_EncodeNumber(server, PriestReply_PartnerAlreadyMarried, 2));
                return true;
            }
            if (LowerCase(name.SubString(1, 3)) != LowerCase(player->partner_name))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Priest,
                                   EO_EncodeNumber(server, PriestReply_NoPermission, 2));
                return true;
            }
            if (LowerCase(target->partner_name) !=
                LowerCase(player->name.SubString(1, 3)))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Priest,
                                   EO_EncodeNumber(server, PriestReply_NoPermission, 2));
                return true;
            }
            if (target->gender == 1 && target->armor_graphic_id != 0x15)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Priest,
                    EO_EncodeNumber(server, PriestReply_PartnerNotDressed, 2));
                return true;
            }
            if (target->gender == 0 && target->armor_graphic_id != 2)
            {
                Client_SendEncoded(
                    server,
                    player,
                    PacketAction_Reply,
                    PacketFamily_Priest,
                    EO_EncodeNumber(server, PriestReply_PartnerNotDressed, 2));
                return true;
            }
            player->read_break = target->player_id;
            String msg = EO_EncodeNumber(server, player->player_id, 2);
            msg.Insert(player->name, msg.Length() + 1);
            Client_SendEncoded(
                server, target, PacketAction_Request, PacketFamily_Priest, msg);
            return true;
        }
        if (action == PacketAction_Accept)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int id = EO_DecodeNumber(server, data.SubString(1, 2));
            Player *target = Players::Players_GetById(server->players, id);
            if (target == NULL)
                return true;
            if (target->map_id != player->map_id)
                return true;
            if (target->read_break != player->player_id)
                return true;
            if (WeddingController::Has(GUI->weddings, target->map_id, target->npc_index))
                return true;
            int npc_id = (int)Mapcontrol_GetNpcIdByIndex(
                server->map_control, target->map_id, target->npc_index);
            NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, npc_id);
            if (type_info.type != NpcType_Priest)
                return true;
            player->npc_index = target->npc_index;
            player->session_token = target->session_token;
            WeddingController::Add(GUI->weddings,
                                   target->map_id,
                                   target->npc_index,
                                   player->player_id,
                                   player->name,
                                   target->player_id,
                                   target->name);
            return true;
        }
        if (action == PacketAction_Use)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 4)
                return false;
            if (player->partner_name.Length() > 3)
                return true;
            WeddingController::Confirm(
                GUI->weddings, player->map_id, player->npc_index, player->player_id);
            return true;
        }
    }
    if (family == PacketFamily_AdminInteract)
    {
        if (action == PacketAction_Tell)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 1)
                return false;
            String msg = EO_EncodeNumber(server, 1, 2);
            msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
            msg.Insert(player->name, msg.Length() + 1);
            msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
            msg.Insert(data, msg.Length() + 1);
            Admin_BroadcastToAll(
                server, PacketAction_Reply, PacketFamily_AdminInteract, msg);
            return true;
        }
        if (action == PacketAction_Report)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 1)
                return false;
            PacketReader_Init(server, data, EO_GetBreakByte(server, EO_BREAK_BYTE));
            String s1 = PacketReader_GetBreakString(server);
            String s2 = PacketReader_GetBreakString(server);
            String msg = EO_EncodeNumber(server, 2, 2);
            msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
            msg.Insert(player->name, msg.Length() + 1);
            msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
            msg.Insert(s2, msg.Length() + 1);
            msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
            msg.Insert(s1, msg.Length() + 1);
            Admin_BroadcastToAll(
                server, PacketAction_Reply, PacketFamily_AdminInteract, msg);
            return true;
        }
    }
    if (family == PacketFamily_Message)
    {
        if (action == PacketAction_Request)
        {
            if (data.Length() != 1)
                return false;
            if (data == "S")
            {
                String out = "S";
                out.Insert(
                    EO_EncodeNumber(server, GUI->server->Socket->ActiveConnections, 2),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(
                        server, Players::Players_GetIdleTimeout(server->players), 2),
                    out.Length() + 1);
                out.Insert(EO_EncodeNumber(
                               server, Players::Players_GetStatTotal(server->players), 2),
                           out.Length() + 1);
                TTimeStamp now = DateTimeToTimeStamp(Now());
                int date_delta =
                    now.Date - mySQLdb::Server_GetUptime(server->mysql_controls).Date;
                int time_delta =
                    now.Time - mySQLdb::Server_GetUptime(server->mysql_controls).Time;
                int minutes = date_delta * 1440 + time_delta / 60000;
                out.Insert(EO_EncodeNumber(server, minutes, 4), out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(
                        server, server->mysql_controls->file_cache->accounts_count, 4),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(
                        server, server->mysql_controls->file_cache->characters_count, 4),
                    out.Length() + 1);
                out.Insert(
                    EO_EncodeNumber(
                        server, server->mysql_controls->file_cache->guilds_count, 4),
                    out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(Server_FormatSentTraffic(server), out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                out.Insert(Server_FormatReceivedTraffic(server), out.Length() + 1);
                out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Message, out);
                return true;
            }
            if (data == "P")
            {
                String out = "P";
                out.Insert(
                    EO_EncodeNumber(
                        server,
                        server->mysql_controls->file_cache->pending_player_writes.size(),
                        2),
                    out.Length() + 1);
                vector<TopPlayer *>::iterator iter;
                for (iter = server->mysql_controls->file_cache->pending_player_writes
                                .begin();
                     iter !=
                     server->mysql_controls->file_cache->pending_player_writes.end();
                     iter++)
                {
                    out.Insert((*iter)->name, out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                    out.Insert((*iter)->title, out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->level, 1),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->experience, 4),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->gender, 1),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->privilege, 1),
                               out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                }
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Message, out);
                return true;
            }
            if (data == "G")
            {
                String out = "G";
                out.Insert(
                    EO_EncodeNumber(
                        server,
                        server->mysql_controls->file_cache->pending_guild_writes.size(),
                        2),
                    out.Length() + 1);
                vector<TopGuild *>::iterator iter;
                for (iter =
                         server->mysql_controls->file_cache->pending_guild_writes.begin();
                     iter !=
                     server->mysql_controls->file_cache->pending_guild_writes.end();
                     iter++)
                {
                    out.Insert((*iter)->ident_guild, out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                    out.Insert((*iter)->guild, out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->exptotal, 4),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->exphigh, 1),
                               out.Length() + 1);
                    out.Insert(EO_EncodeNumber(server, (*iter)->members, 2),
                               out.Length() + 1);
                    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
                }
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Message, out);
                return true;
            }
        }
        if (action == PacketAction_List)
        {
            Client_SendRaw(server, player, Message_BuildServerStatus(server), 0xc);
            return true;
        }
        if (action == PacketAction_Ping)
        {
            if (data.Length() != 2)
                return false;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Pong,
                               PacketFamily_Message,
                               EO_EncodeNumber(server, 2, 2));
            return true;
        }
    }
    if (family == PacketFamily_Global)
    {
        if (!player->logged_in)
            return false;
        if (action == PacketAction_Open)
        {
            player->global_chat = 1;
            String names = server->field_0x8c[0];
            for (int i = 1; i < 7; i++)
                names.Insert(server->field_0x8c[i], names.Length() + 1);
            if (names != "")
                Client_SendEncoded(
                    server, player, PacketAction_List, PacketFamily_Talk, names);
            return true;
        }
        if (action == PacketAction_Close)
        {
            player->global_chat = 0;
            return true;
        }
        if (action == PacketAction_Player)
        {
            player->show_players = 1;
            return true;
        }
        if (action == PacketAction_Remove)
        {
            player->show_players = 0;
            return true;
        }
    }
    return false;
}

void MysqlCallback_Dispatch(Packets *server, mySQLtask *query_result)
{
    Player *player = Players::Players_GetById(server->players, query_result->player_id);
    if (player == NULL)
        return;
    if (player->query_id != query_result->expected_query_id)
        return;
    if (query_result->query_id == 0x40)
    {
        PacketReader_Init(
            server, query_result->data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        String account = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                    PacketReader_GetBreakString(server));
        String password = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        if (GUI->myquery->RecordCount < 1)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_WrongUser, 2) + "NO");
            return;
        }
        if (password !=
            Account_DecodePassword(
                server, mySQLdb::Db_GetString(server->mysql_controls, "password")))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_WrongUserPassword, 2) +
                                   "NO");
            return;
        }
        if ((unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "banned") > 0)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_Banned, 2) + "NO");
            player->removing = true;
            return;
        }
        int ident = mySQLdb::Db_GetInt(server->mysql_controls, "ident");
        String account_name = mySQLdb::Db_GetString(server->mysql_controls, "account");
        String account_type = mySQLdb::Db_GetString(server->mysql_controls, "type");
        if (mySQLdb::IsTaskPending(server->mysql_controls, ident))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_LoggedIn, 2) + "NO");
            return;
        }
        if (Players::Players_IsAccountIdentOnline(server->players, ident))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, LoginReply_LoggedIn, 2) + "NO");
            return;
        }
        if (account_type == "VIP" || account_type == "DEV")
        {
            Logins::SetReservedName(
                server->logins, account, player->socket->RemoteAddress);
            player->remove_timer = -1;
        }
        if (player->remove_timer > 0)
        {
            player->removing = true;
            return;
        }
        player->account_ident = ident;
        player->account_name = account_name;
        player->field_0x48 = account_type;
        if (Players::Players_IsAccountNameTaken(
                server->players, player->account_name, player->player_id))
        {
            Banned::AddBan(server->banned, player->remote_ip, player->hdid, (char)0, 120);
            player->removing = true;
            return;
        }
        player->account_logged_in = true;
        TDateTime now = Now();
        mySQLdb::Mysql_ExecDirect_FromCallback(server->mysql_controls,
                                               player->account_ident,
                                               "UPDATE endl_accounts SET lastvisit = '" +
                                                   now.DateString() +
                                                   "' WHERE ident = " + IntToStr(ident));
        mySQLdb::Mysql_SubmitQuery_FromCallback(
            server->mysql_controls,
            0x41,
            player->player_id,
            player->query_id,
            "",
            "SELECT * FROM endl_characters WHERE ident_account = " +
                IntToStr((unsigned int)player->account_ident) +
                " ORDER BY level DESC LIMIT 3");
        return;
    }
    if (query_result->query_id == 0x41)
    {
        Login_SendCharacterList(server,
                                player,
                                PacketAction_Reply,
                                PacketFamily_Login,
                                EO_EncodeNumber(server, 3, 2));
        return;
    }
    if (query_result->query_id == 0x45)
    {
        if (GUI->myquery->RecordCount > 0)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Character,
                               EO_EncodeNumber(server, CharacterReply_Exists, 2) + "NO");
            return;
        }
        int gender = EO_DecodeNumber(server, query_result->data.SubString(3, 2));
        int hair_modal = EO_DecodeNumber(server, query_result->data.SubString(5, 2));
        int hair_color = EO_DecodeNumber(server, query_result->data.SubString(7, 2));
        int skin_color = EO_DecodeNumber(server, query_result->data.SubString(9, 2));
        String name = mySQLdb::Db_SanitizeString(
            server->mysql_controls,
            PacketReader_GetBreakStringAt(
                server, 2, query_result->data, EO_GetBreakByte(server, EO_BREAK_BYTE)));
        TDateTime now = Now();
        String sql =
            "INSERT INTO endl_characters (ident_account, ident_guild, ident_class, "
            "name, signup, gender, hairmodal, haircolor, skincolor, nav_map, nav_x,"
            " nav_y, hp_max, hp_now, mp_max, mp_now, sp_max, clientusge, money_bank ) "
            "VALUES (";
        sql = sql + IntToStr((unsigned int)player->account_ident) + ",";
        sql = sql + "'0',1,'";
        sql = sql + name + "',";
        sql = sql + "'" + now.DateString() + "',";
        sql = sql + IntToStr(gender) + ",";
        sql = sql + IntToStr(hair_modal) + ",";
        sql = sql + IntToStr(hair_color) + ",";
        sql = sql + IntToStr(skin_color) + ",";
        sql = sql + IntToStr(Settings::GetStartMap(server->settings)) + ",";
        sql = sql + IntToStr(Settings::GetStartX(server->settings)) + ",";
        sql = sql + IntToStr(Settings::GetStartY(server->settings)) + ",";
        sql = sql + "10,10,10,10,20,0,0)";
        mySQLdb::Mysql_ExecDirect_FromCallback(
            server->mysql_controls, player->account_ident, sql);
        mySQLdb::Mysql_SubmitQuery_FromCallback(
            server->mysql_controls,
            0x46,
            player->player_id,
            player->query_id,
            "",
            "SELECT * FROM endl_characters WHERE ident_account = " +
                IntToStr((unsigned int)player->account_ident) +
                " ORDER BY level DESC LIMIT 3");
        server->mysql_controls->file_cache->characters_count++;
        return;
    }
    if (query_result->query_id == 0x46)
    {
        Login_SendCharacterList(server,
                                player,
                                PacketAction_Reply,
                                PacketFamily_Character,
                                EO_EncodeNumber(server, 5, 2));
        player->null_string = "";
        player->session_id = RandRange(50000) + 10000;
        return;
    }
    if (query_result->query_id == 0x43)
    {
        if (GUI->myquery->RecordCount > 0)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Account,
                               EO_EncodeNumber(server, AccountReply_Exists, 2) + "NO");
            return;
        }
        String reply = EO_EncodeNumber(server, player->session_id, 2);
        reply.Insert(EO_EncodeNumber(server, server->ping_history[0], 1),
                     reply.Length() + 1);
        reply.Insert("OK", reply.Length() + 1);
        Client_SendEncoded(
            server, player, PacketAction_Reply, PacketFamily_Account, reply);
        return;
    }
    if (query_result->query_id == 0x44)
    {
        if (GUI->myquery->RecordCount > 0)
        {
            player->removing = true;
            return;
        }
        PacketReader_Init(
            server, query_result->data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        PacketReader_GetBreakString(server);
        int code = EO_DecodeNumber(server, query_result->data.SubString(1, 2));
        String account = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                    PacketReader_GetBreakString(server));
        if (player->session_id != code)
        {
            player->removing = true;
            return;
        }
        if (player->account_create_cooldown > 4)
            return;
        player->account_create_cooldown = 6;
        String password = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        String realname = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        String location = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        String email = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                  PacketReader_GetBreakString(server));
        String serial_c = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        String serial_h = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                     PacketReader_GetBreakString(server));
        TDateTime now = Now();
        String sql = "INSERT INTO endl_accounts (account, password, realname, location, "
                     "email,  signup, lastvisit, serial_c, serial_h , ipaddress, banned) "
                     "VALUES (";
        sql = sql + "'" + account + "',";
        sql = sql + "ENCODE('" + Account_EncodePassword(server, password) +
              "','eoeokeyendl'),";
        sql = sql + "'" + realname + "',";
        sql = sql + "'" + location + "',";
        sql = sql + "'" + email + "',";
        sql = sql + "'" + now.DateString() + "',";
        sql = sql + "'" + now.DateString() + "',";
        sql = sql + "'" + serial_c + "',";
        sql = sql + "'" + serial_h + "',";
        sql = sql + "'" + player->socket->RemoteAddress + "',";
        sql = sql + "0)";
        mySQLdb::Mysql_ExecDirect(server->mysql_controls, 0, sql);
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Account,
                           EO_EncodeNumber(server, AccountReply_Created, 2) + "GO");
        server->mysql_controls->file_cache->accounts_count++;
        player->session_id = RandRange(50000) + 10000;
        return;
    }
    if (query_result->query_id == 0x42)
    {
        PacketReader_Init(
            server, query_result->data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        String account = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                    PacketReader_GetBreakString(server));
        String old_password = mySQLdb::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String new_password = mySQLdb::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        if (GUI->myquery->RecordCount < 1)
        {
            player->removing = true;
            return;
        }
        if (Account_DecodePassword(
                server, mySQLdb::Db_GetString(server->mysql_controls, "password")) !=
                old_password ||
            mySQLdb::Db_GetString(server->mysql_controls, "account") != account)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Account,
                               EO_EncodeNumber(server, AccountReply_ChangeFailed, 2) +
                                   "NO");
            return;
        }
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Account,
                           EO_EncodeNumber(server, AccountReply_Changed, 2) + "OK");
        mySQLdb::Mysql_ExecDirect(server->mysql_controls,
                                  player->account_ident,
                                  "UPDATE endl_accounts SET password = ENCODE('" +
                                      Account_EncodePassword(server, new_password) +
                                      "','eoeokeyendl') WHERE ident = " +
                                      IntToStr((unsigned int)player->account_ident));
        return;
    }
    if (query_result->query_id == 0x47)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        int token = EO_DecodeNumber(server, query_result->data.SubString(5, 1));
        String char_name =
            query_result->data.SubString(6, query_result->data.Length() - 5);
        String rank_label = "rank" + IntToStr(token);
        String rank_value = mySQLdb::Db_GetString(server->mysql_controls, rank_label);
        Player *target = Players::Players_FindByName(server->players, char_name);
        rank_value =
            mySQLdb::Mysql_SanitizeString(server->mysql_controls, rank_value, false);
        if (target == NULL)
        {
            player->field_0x14 = rank_value;
            mySQLdb::Mysql_SubmitQuery_FromCallback(
                server->mysql_controls,
                0x48,
                player->player_id,
                player->query_id,
                query_result->data,
                "SELECT ident_guild, ident_rank FROM endl_characters WHERE name = '" +
                    char_name + "' AND ident_guild = '" + player->guild_tag +
                    "' LIMIT 1");
            return;
        }
        if (target->guild_tag != player->guild_tag)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_RankingNotMember, 2));
            return;
        }
        if (target->guild_rank_id == 1)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_RankingLeader, 2));
            return;
        }
        target->guild_rank_name = rank_value;
        target->guild_rank_id = token;
        Client_SendEncoded(server,
                           target,
                           PacketAction_Accept,
                           PacketFamily_Guild,
                           EO_EncodeNumber(server, token, 1) + rank_value);
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Guild,
                           EO_EncodeNumber(server, GuildReply_Updated, 2));
        return;
    }
    if (query_result->query_id == 0x48)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        int token = EO_DecodeNumber(server, query_result->data.SubString(5, 1));
        String char_name =
            query_result->data.SubString(6, query_result->data.Length() - 5);
        if (AnsiLowerCase(mySQLdb::Db_GetString(server->mysql_controls, "ident_guild")) !=
            AnsiLowerCase(player->guild_tag))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_RankingNotMember, 2));
            return;
        }
        if ((unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "ident_rank") == 1)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_RankingLeader, 2));
            return;
        }
        mySQLdb::Mysql_ExecDirect_FromCallback(
            server->mysql_controls,
            player->account_ident,
            "UPDATE endl_characters SET ident_rank = " + IntToStr(token) + ", rank = '" +
                player->field_0x14 + "' WHERE name = '" + char_name +
                "' AND ident_guild = '" + player->guild_tag + "'");
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Guild,
                           EO_EncodeNumber(server, GuildReply_Updated, 2));
        return;
    }
    if (query_result->query_id == 0x49)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        if (AnsiLowerCase(mySQLdb::Db_GetString(server->mysql_controls, "ident_guild")) !=
            AnsiLowerCase(player->guild_tag))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_RemoveLeader, 2));
            return;
        }
        if ((unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "ident_rank") == 1)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_RemoveNotMember, 2));
            return;
        }
        String char_name =
            query_result->data.SubString(5, query_result->data.Length() - 4);
        mySQLdb::Mysql_ExecDirect_FromCallback(
            server->mysql_controls,
            player->account_ident,
            "UPDATE endl_characters SET ident_guild = '0', ident_rank = 9, "
            "guild = '', rank = '' WHERE name = '" +
                char_name + "' AND ident_guild = '" + player->guild_tag + "'");
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Guild,
                           EO_EncodeNumber(server, GuildReply_Removed, 2));
        return;
    }
    if (query_result->query_id == 0x4a)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        String description = mySQLdb::Db_GetString(server->mysql_controls, "description");
        if (description.Length() == 0)
            description = " ";
        Client_SendEncoded(
            server, player, PacketAction_Take, PacketFamily_Guild, description);
        return;
    }
    if (query_result->query_id == 0x4b)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        String ranks = mySQLdb::Db_GetString(server->mysql_controls, "rank1");
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank2"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank3"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank4"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank5"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank6"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank7"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank8"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        ranks.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank9"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), ranks.Length() + 1);
        Client_SendEncoded(server, player, PacketAction_Rank, PacketFamily_Guild, ranks);
        return;
    }
    if (query_result->query_id == 0x4c)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        Client_SendEncoded(
            server,
            player,
            PacketAction_Sell,
            PacketFamily_Guild,
            EO_EncodeNumber(
                server, mySQLdb::Db_GetInt(server->mysql_controls, "money"), 4));
        return;
    }
    if (query_result->query_id == 0x4d)
    {
        if (GUI->myquery->RecordCount < 1)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_NotFound, 2));
            return;
        }
        String list = EO_EncodeNumber(server, GUI->myquery->RecordCount, 2);
        list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
        while (!GUI->myquery->Eof)
        {
            list.Insert(
                EO_EncodeNumber(
                    server, mySQLdb::Db_GetInt(server->mysql_controls, "ident_rank"), 1),
                list.Length() + 1);
            list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
            list.Insert(mySQLdb::Db_GetString(server->mysql_controls, "name"),
                        list.Length() + 1);
            list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
            list.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank"),
                        list.Length() + 1);
            list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
            GUI->myquery->Next();
        }
        Client_SendEncoded(server, player, PacketAction_Tell, PacketFamily_Guild, list);
        return;
    }
    if (query_result->query_id == 0x4e)
    {
        if (GUI->myquery->RecordCount < 1)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_NotFound, 2));
            return;
        }
        String type = "bankrupt";
        int money = mySQLdb::Db_GetInt(server->mysql_controls, "money");
        String tag = mySQLdb::Db_GetString(server->mysql_controls, "tag");
        if (money >= 0x7d0)
            type = "poor";
        if (money >= 0x2710)
            type = "normal";
        if (money >= 0xc350)
            type = "wealthy";
        if (money >= 0x186a0)
            type = "very wealthy";
        player->field_0x14 = mySQLdb::Db_GetString(server->mysql_controls, "name");
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "tag"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "signup"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            mySQLdb::Db_GetString(server->mysql_controls, "description"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(type, player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank1"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank2"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank3"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank4"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank5"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank6"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank7"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank8"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(mySQLdb::Db_GetString(server->mysql_controls, "rank9"),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                  player->field_0x14.Length() + 1);
        mySQLdb::Mysql_SubmitQuery_FromCallback(
            server->mysql_controls,
            0x4f,
            player->player_id,
            player->query_id,
            "",
            "SELECT ident_rank, name FROM endl_characters WHERE ident_rank < 3 "
            "AND ident_guild = '" +
                tag + "' ORDER by ident_rank asc LIMIT 20");
        return;
    }
    if (query_result->query_id == 0x4f)
    {
        bool has_result = true;
        if (GUI->myquery->RecordCount < 1)
            has_result = false;
        if (has_result)
        {
            player->field_0x14.Insert(
                EO_EncodeNumber(
                    server, mySQLdb::GetResultCount(server->mysql_controls), 2),
                player->field_0x14.Length() + 1);
            player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                      player->field_0x14.Length() + 1);
            while (!mySQLdb::ResultAtEnd(server->mysql_controls))
            {
                player->field_0x14.Insert(
                    EO_EncodeNumber(
                        server,
                        mySQLdb::Db_GetInt(server->mysql_controls, "ident_rank"),
                        1),
                    player->field_0x14.Length() + 1);
                player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                          player->field_0x14.Length() + 1);
                player->field_0x14.Insert(
                    mySQLdb::Db_GetString(server->mysql_controls, "name"),
                    player->field_0x14.Length() + 1);
                player->field_0x14.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE),
                                          player->field_0x14.Length() + 1);
                mySQLdb::NextResultRecord(server->mysql_controls);
            }
        }
        Client_SendEncoded(
            server, player, PacketAction_Report, PacketFamily_Guild, player->field_0x14);
        return;
    }
    if (query_result->query_id == 0x50)
    {
        if (mySQLdb::GetResultCount(server->mysql_controls) > 0)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_Exists, 2));
            return;
        }
        PacketReader_Init(
            server, query_result->data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        PacketReader_GetBreakString(server);
        String guild = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                  PacketReader_GetBreakString(server));
        String name = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                 PacketReader_GetBreakString(server));
        player->guild_inviter_id = player->player_id;
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Guild,
                           EO_EncodeNumber(server, GuildReply_CreateBegin, 2));
        String msg = EO_EncodeNumber(server, player->player_id, 2);
        msg.Insert(name + " (" + guild + ")", msg.Length() + 1);
        Server_BroadcastToMap(
            server, player->map_id, PacketAction_Request, PacketFamily_Guild, msg);
        return;
    }
    if (query_result->query_id == 0x51)
    {
        if (GUI->myquery->RecordCount > 0)
            return;
        if (Players::Players_CountGuildInvites(server->players, player) < 10)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Guild,
                               EO_EncodeNumber(server, GuildReply_NoCandidates, 2));
            return;
        }
        if (!Players::Player_RemoveItem(server->players, player, 1, 0xc350))
            return;
        PacketReader_Init(
            server, query_result->data, EO_GetBreakByte(server, EO_BREAK_BYTE));
        PacketReader_GetBreakString(server);
        String tag = AnsiUpperCase(mySQLdb::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server)));
        String name = mySQLdb::Db_SanitizeString(server->mysql_controls,
                                                 PacketReader_GetBreakString(server));
        String description = mySQLdb::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        player->guild_tag = tag;
        player->guild_name = name;
        player->guild_rank_name = "Leader";
        player->guild_inviter_id = -1;
        player->guild_rank_id = 1;
        Players::Players_GuildSetMemberInfo(server->players, player, name, tag);
        TDateTime now = Now();
        tag = mySQLdb::Db_SanitizeString(server->mysql_controls, tag);
        name = mySQLdb::Db_SanitizeString(server->mysql_controls, name);
        description = mySQLdb::Db_SanitizeString(server->mysql_controls, description);
        String sql = "INSERT INTO endl_guilds (tag, name, description, money, signup, "
                     "rank1, rank2 ) VALUES (";
        sql = sql + "'" + AnsiUpperCase(tag) + "',";
        sql = sql + "'" + name + "',";
        sql = sql + "'" + description + "',";
        sql = sql + "10000,";
        sql = sql + "'" + now.DateString() + "',";
        sql = sql + "'Leader',";
        sql = sql + "'Recruiter')";
        mySQLdb::Mysql_ExecDirect_FromCallback(server->mysql_controls, 0, sql);
        if (tag.Length() == 2)
            tag = tag + " ";
        String msg = EO_EncodeNumber(server, player->player_id, 2);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        msg.Insert(tag, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        msg.Insert(name, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        msg.Insert("Leader", msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        Guild_BroadcastToAll(
            server, player, PacketAction_Create, PacketFamily_Guild, msg);
        msg.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                   msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        Client_SendEncoded(server, player, PacketAction_Create, PacketFamily_Guild, msg);
        server->mysql_controls->file_cache->guilds_count++;
        return;
    }
    if (query_result->query_id == 0x52)
    {
        if (GUI->myquery->RecordCount < 1)
            return;
        Player *other = Players::Players_GetById(
            server->players, EO_DecodeNumber(server, query_result->data.SubString(1, 2)));
        if (other == NULL)
            return;
        if (other->guild_tag.Length() > 1)
            return;
        if (other->map_id != player->map_id)
            return;
        if (AnsiLowerCase(mySQLdb::Db_GetString(server->mysql_controls, "tag")) ==
            AnsiLowerCase(other->guild_tag))
            return;
        if ((unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "money") < 0x3e8)
            return;
        String tag = mySQLdb::Db_GetString(server->mysql_controls, "tag");
        String name = mySQLdb::Db_GetString(server->mysql_controls, "name");
        String rank9 = mySQLdb::Db_GetString(server->mysql_controls, "rank9");
        int money =
            (unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "money") - 0x3e8;
        mySQLdb::Mysql_ExecDirect_FromCallback(
            server->mysql_controls,
            0,
            "UPDATE endl_guilds SET money = " + IntToStr(money) + " WHERE tag = '" +
                player->guild_tag + "'");
        other->guild_tag = tag;
        other->guild_name = name;
        other->guild_rank_name = rank9;
        other->guild_rank_id = 9;
        other->guild_inviter_id = -1;
        player->guild_inviter_id = -1;
        String msg = EO_EncodeNumber(server, player->player_id, 2);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        msg.Insert(tag, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        msg.Insert(name, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        msg.Insert(rank9, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), msg.Length() + 1);
        Client_SendEncoded(server, other, PacketAction_Agree, PacketFamily_Guild, msg);
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Guild,
                           EO_EncodeNumber(server, GuildReply_Accepted, 2));
        return;
    }
    if (query_result->query_id == 0x53)
    {
        vector<TopGuild *>::iterator it =
            server->mysql_controls->file_cache->pending_guild_writes.begin();
        while (it != server->mysql_controls->file_cache->pending_guild_writes.end())
        {
            TopGuild *entry = *it;
            it = server->mysql_controls->file_cache->pending_guild_writes.erase(it);
            delete entry;
        }
        mySQLdb::LoadCachedGuilds(server->mysql_controls);
    }
}

void Player_FireQuestTriggers(Packets *server, Player *player, int state_index, int value)
{
    for (PlayerQuest *iter = player->quest_trackers.begin();
         iter != player->quest_trackers.end();)
    {
        QuestState *state = QuestContainer::GetState(
            server->quest_engine, iter->quest_id, iter->state_index);
        if (state == NULL)
        {
            iter = player->quest_trackers.erase(iter);
            continue;
        }
        Player_EvaluateQuestRules(server, player, iter, state, state_index, value);
        if (iter->done)
        {
            iter = player->quest_trackers.erase(iter);
            continue;
        }
        iter++;
    }
}

void Player_EvaluateQuestRules(Packets *server,
                               Player *player,
                               PlayerQuest *tracker,
                               QuestState *state,
                               int event,
                               int arg)
{
    vector<QuestRule *>::iterator iter;
    int index = -1;
    for (iter = state->rules.begin(); iter != state->rules.end(); iter++)
    {
        index++;
        if ((*iter)->rule == event || event == 400)
        {
            if ((*iter)->rule == 14)
            {
                if ((*iter)->args[0] <=
                    QuestCounters::GetCompletionCount(
                        server->quest_counters, player->name, tracker->quest_id))
                {
                    tracker->state_index = *(short *)&(*iter)->goto_state_index;
                    Player_ApplyQuestActions(server, player, tracker, true);
                    return;
                }
            }
            if (event == 400)
            {
                int arg1 = (*iter)->args[0];
                int arg2 = (*iter)->args[1];
                if ((*iter)->rule == 13)
                {
                    tracker->state_index = *(short *)&(*iter)->goto_state_index;
                    Player_ApplyQuestActions(server, player, tracker, true);
                }
                if ((*iter)->rule == 3)
                {
                    int amount =
                        Players::Players_GetItemAmount(server->players, player, arg1);
                    if (amount >= 0)
                        tracker->counters[index] = (short)amount;
                    else
                        tracker->counters[index] = 0;
                    if (amount >= arg2)
                    {
                        tracker->state_index = *(short *)&(*iter)->goto_state_index;
                        Player_ApplyQuestActions(server, player, tracker, true);
                        return;
                    }
                    continue;
                }
                if ((*iter)->rule == 4)
                {
                    if (Players::Players_GetItemAmount(server->players, player, arg1) <
                        arg2)
                    {
                        tracker->state_index = *(short *)&(*iter)->goto_state_index;
                        Player_ApplyQuestActions(server, player, tracker, true);
                        return;
                    }
                    continue;
                }
            }
            if ((*iter)->rule == 8)
            {
                if (index > 4)
                    continue;
                int a1 = (*iter)->args[0];
                int a2 = (*iter)->args[1];
                if (arg != a1)
                    continue;
                tracker->counters[index]++;
                if (tracker->counters[index] >= a2)
                {
                    tracker->state_index = *(short *)&(*iter)->goto_state_index;
                    Player_ApplyQuestActions(server, player, tracker, true);
                    return;
                }
            }
            if ((*iter)->rule == 9)
            {
                if (index > 4)
                    continue;
                int a1 = (*iter)->args[0];
                tracker->counters[index]++;
                if (tracker->counters[index] >= a1)
                {
                    tracker->state_index = *(short *)&(*iter)->goto_state_index;
                    Player_ApplyQuestActions(server, player, tracker, true);
                    return;
                }
            }
            if ((*iter)->rule == 10)
            {
                int a1 = (*iter)->args[0];
                int a2 = (*iter)->args[1];
                int a3 = (*iter)->args[2];
                if (player->map_id == a1 && player->x == a2 && player->y == a3)
                {
                    tracker->state_index = *(short *)&(*iter)->goto_state_index;
                    Player_ApplyQuestActions(server, player, tracker, true);
                    return;
                }
            }
            if ((*iter)->rule == 11 && player->map_id == (*iter)->args[0])
            {
                tracker->state_index = *(short *)&(*iter)->goto_state_index;
                Player_ApplyQuestActions(server, player, tracker, true);
                return;
            }
            if ((*iter)->rule == 12 && (*iter)->args[0] == arg)
            {
                tracker->state_index = *(short *)&(*iter)->goto_state_index;
                Player_ApplyQuestActions(server, player, tracker, true);
                return;
            }
        }
    }
}

void Player_ApplyQuestActions(Packets *server,
                              Player *player,
                              PlayerQuest *tracker,
                              bool repeat)
{
    if (repeat)
    {
        for (int i = 0; i < 5; i++)
            tracker->counters[i] = 0;
        QuestState *state = QuestContainer::GetState(
            server->quest_engine, tracker->quest_id, tracker->state_index);
        if (state == 0)
        {
            tracker->done = 1;
            return;
        }
        for (vector<QuestAction *>::iterator iter = state->actions.begin();
             iter != state->actions.end();
             iter++)
        {
            if ((*iter)->action == 4)
            {
                int target_map = (*iter)->args[0];
                MapCoord coords;
                coords.x = (*iter)->args[1];
                coords.y = (*iter)->args[2];
                Player_Warp(server, player, target_map, coords, WarpEffect_None, true);
            }
            if ((*iter)->action == 5)
            {
                int item = (*iter)->args[0];
                int amount = (*iter)->args[1];
                if ((*iter)->data[1] == "")
                    amount = 1;
                if (amount > 0)
                {
                    Players::Player_AddItem(server->players, player, item, amount);
                    player->weight_current +=
                        ItemValues::GetWeight(GUI->item_values, item) * amount;
                    if (player->weight_current < 0)
                        player->weight_current = 0;
                    int weight = player->weight_current;
                    if (weight > 250)
                        weight = 250;
                    String reply = EO_EncodeNumber(server, item, 2);
                    reply.Insert(EO_EncodeNumber(server, amount, 3), reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, weight, 1), reply.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Obtain, PacketFamily_Item, reply);
                }
            }
            if ((*iter)->action == 6)
            {
                int item = (*iter)->args[0];
                int amount = (*iter)->args[1];
                if ((*iter)->data[1] == "")
                    amount = 1;
                if (amount > 0)
                {
                    Players::Player_RemoveItemNoQuestRules(
                        server->players, player, item, amount);
                    player->weight_current -=
                        ItemValues::GetWeight(GUI->item_values, item) *
                        player->item_change_count;
                    if (player->weight_current < 0)
                        player->weight_current = 0;
                    int weight = player->weight_current;
                    if (weight > 250)
                        weight = 250;
                    String reply = EO_EncodeNumber(server, item, 2);
                    reply.Insert(
                        EO_EncodeNumber(server, player->item_change_remaining, 4),
                        reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, weight, 1), reply.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Kick, PacketFamily_Item, reply);
                }
            }
            if ((*iter)->action == 0x14)
            {
                QuestCounters::RecordCompletion(
                    server->quest_counters, player->name, tracker->quest_id);
                tracker->done = 1;
                return;
            }
            if ((*iter)->action == 7 || (*iter)->action == 8)
            {
                if ((*iter)->action == 7)
                {
                    bool found = true;
                    for (vector<PlayerQuest>::iterator iter2 =
                             player->quest_history.begin();
                         iter2 != player->quest_history.end();
                         iter2++)
                    {
                        if (iter2->quest_id == tracker->quest_id)
                            found = false;
                    }
                    if (found)
                        player->quest_history.insert(player->quest_history.end(),
                                                     *tracker);
                }
                tracker->done = 1;
                return;
            }
            if ((*iter)->action == 9)
            {
                int class_id = (*iter)->args[0];
                player->class_id = class_id;
                Player::UpdateBaseStats(player);
                Player_CalculateStats(server, player);
                Player::CalculateHP_TP_SP(player);
                String reply = EO_EncodeNumber(server, player->class_id, 2);
                reply.Insert(EO_EncodeNumber(server, player->adj_strength, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->adj_intelligence, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->adj_wisdom, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->adj_agility, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->adj_constitution, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->adj_charisma, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->max_hp, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->max_tp, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->max_sp, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->weight_max, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->min_damage, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->max_damage, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->accuracy, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->evasion, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->armor, 2),
                             reply.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_List, PacketFamily_Recover, reply);
            }
            if ((*iter)->action == 0xa)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Player,
                                   PacketFamily_Jukebox,
                                   EO_EncodeNumber(server, (*iter)->args[0], 1));
            }
            if ((*iter)->action == 0xb)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Player,
                                   PacketFamily_Music,
                                   EO_EncodeNumber(server, (*iter)->args[0], 1));
            }
            if ((*iter)->action == 0xc)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Open,
                                   PacketFamily_Message,
                                   (*iter)->data[0]);
            }
            if ((*iter)->action == 0xd)
            {
                player->experience += (*iter)->args[0];
                int level = Players::Player_TryLevelUp(server->players, player);
                if (level > 0)
                {
                    Server_BroadcastNearby(server,
                                           player,
                                           PacketAction_Accept,
                                           PacketFamily_Item,
                                           EO_EncodeNumber(server, player->player_id, 2));
                }
                String reply = EO_EncodeNumber(server, player->experience, 4);
                reply.Insert(EO_EncodeNumber(server, player->karma + 1000, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, level, 1), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->stat_points, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, player->skill_points, 2),
                             reply.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Recover, reply);
            }
            if ((*iter)->action == 0xe)
            {
                player->experience -= (*iter)->args[0];
                String reply = EO_EncodeNumber(server, player->experience, 4);
                reply.Insert(EO_EncodeNumber(server, player->karma + 1000, 2),
                             reply.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Recover, reply);
            }
            if ((*iter)->action == 0xf)
            {
                player->karma += (*iter)->args[0];
                if (player->karma > 1000)
                    player->karma = 1000;
                String reply = EO_EncodeNumber(server, player->experience, 4);
                reply.Insert(EO_EncodeNumber(server, player->karma + 1000, 2),
                             reply.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Recover, reply);
            }
            if ((*iter)->action == 0x10)
            {
                player->karma -= (*iter)->args[0];
                if (player->karma < -1000)
                    player->karma = -1000;
                String reply = EO_EncodeNumber(server, player->experience, 4);
                reply.Insert(EO_EncodeNumber(server, player->karma + 1000, 2),
                             reply.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Reply, PacketFamily_Recover, reply);
            }
            if ((*iter)->action == 0x11)
            {
                String buf = EO_EncodeNumber(server, 1, 1);
                buf.Insert(EO_EncodeNumber(server, (*iter)->args[0], 1),
                           buf.Length() + 1);
                Server_BroadcastToMap(
                    server, player->map_id, PacketAction_Use, PacketFamily_Effect, buf);
            }
            if ((*iter)->action == 0x12)
            {
                String buf = EO_EncodeNumber(server, player->player_id, 2);
                buf.Insert(EO_EncodeNumber(server, (*iter)->args[0], 3),
                           buf.Length() + 1);
                Server_BroadcastNearby(
                    server, player, PacketAction_Player, PacketFamily_Effect, buf);
                Client_SendEncoded(
                    server, player, PacketAction_Player, PacketFamily_Effect, buf);
            }
        }
    }
    if (repeat)
        Player_FireQuestTriggers(server, player, 0x190, 0);
}

void Login_SendCharacterList(Packets *server,
                             Player *player,
                             PacketAction action,
                             PacketFamily family,
                             String data)
{
    GUI->myquery->Open();
    data.Insert(
        EO_EncodeNumber(server, mySQLdb::GetResultCount(server->mysql_controls), 1),
        data.Length() + 1);
    data.Insert(EO_EncodeNumber(server, 0, 1), data.Length() + 1);
    data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);

    for (int i = 0; i < 3; i++)
    {
        if (player->character_slots[i] != 0)
        {
            Player *slot = player->character_slots[i];
            player->character_slots[i] = 0;
            delete slot;
        }
    }

    int i = 0;
    while (GUI->myquery->Eof == false && i < 3)
    {
        Player *newplayer = new Player(player->socket);
        newplayer->player_id = *(int *)((char *)player->socket + 4);
        newplayer->character_id = mySQLdb::Db_GetInt(server->mysql_controls, "ident");
        newplayer->account_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "ident_account");
        newplayer->class_id = mySQLdb::Db_GetInt(server->mysql_controls, "ident_class");
        newplayer->guild_rank_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "ident_rank");
        newplayer->guild_tag =
            mySQLdb::Db_GetString(server->mysql_controls, "ident_guild");
        newplayer->home_id = mySQLdb::Db_GetInt(server->mysql_controls, "citizenship");
        newplayer->name = mySQLdb::Db_GetString(server->mysql_controls, "name");
        newplayer->partner_name =
            mySQLdb::Db_GetString(server->mysql_controls, "partner");
        newplayer->title = mySQLdb::Db_GetString(server->mysql_controls, "title");
        newplayer->guild_name = mySQLdb::Db_GetString(server->mysql_controls, "guild");
        newplayer->guild_rank_name =
            mySQLdb::Db_GetString(server->mysql_controls, "rank");
        newplayer->experience = mySQLdb::Db_GetInt(server->mysql_controls, "experience");
        newplayer->level = mySQLdb::Db_GetInt(server->mysql_controls, "level");
        newplayer->signup = mySQLdb::Db_GetString(server->mysql_controls, "signup");
        newplayer->gender = mySQLdb::Db_GetInt(server->mysql_controls, "gender");
        newplayer->hair_style = mySQLdb::Db_GetInt(server->mysql_controls, "hairmodal");
        newplayer->hair_color = mySQLdb::Db_GetInt(server->mysql_controls, "haircolor");
        newplayer->skin = mySQLdb::Db_GetInt(server->mysql_controls, "skincolor");
        newplayer->map_id = mySQLdb::Db_GetInt(server->mysql_controls, "nav_map");
        newplayer->x = mySQLdb::Db_GetInt(server->mysql_controls, "nav_x");
        newplayer->y = mySQLdb::Db_GetInt(server->mysql_controls, "nav_y");
        newplayer->direction =
            mySQLdb::Db_GetInt(server->mysql_controls, "nav_direction");
        newplayer->base_hp = mySQLdb::Db_GetInt(server->mysql_controls, "hp_max");
        newplayer->hp = mySQLdb::Db_GetInt(server->mysql_controls, "hp_now");
        newplayer->base_tp = mySQLdb::Db_GetInt(server->mysql_controls, "mp_max");
        newplayer->tp = mySQLdb::Db_GetInt(server->mysql_controls, "mp_now");
        newplayer->base_sp = mySQLdb::Db_GetInt(server->mysql_controls, "sp_max");
        newplayer->usage = mySQLdb::Db_GetInt(server->mysql_controls, "clientusge");
        newplayer->money_bank = mySQLdb::Db_GetInt(server->mysql_controls, "money_bank");
        newplayer->locker_bank =
            mySQLdb::Db_GetInt(server->mysql_controls, "locker_bank");
        newplayer->stat_points =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_points");
        newplayer->skill_points =
            mySQLdb::Db_GetInt(server->mysql_controls, "skill_points");
        newplayer->karma = mySQLdb::Db_GetInt(server->mysql_controls, "alignment_good");
        newplayer->base_strength =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_strenght");
        newplayer->base_wisdom =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_wisdom");
        newplayer->base_intelligence =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_intelligence");
        newplayer->base_agility =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_agility");
        newplayer->base_constitution =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_constitution");
        newplayer->base_charisma =
            mySQLdb::Db_GetInt(server->mysql_controls, "stat_charisma");
        newplayer->boots_item_id = mySQLdb::Db_GetInt(server->mysql_controls, "eq_boots");
        newplayer->accessory_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_pants");
        newplayer->gloves_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_gloves");
        newplayer->armor_item_id = mySQLdb::Db_GetInt(server->mysql_controls, "eq_armor");
        newplayer->belt_item_id = mySQLdb::Db_GetInt(server->mysql_controls, "eq_belt");
        newplayer->necklace_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_necklage");
        newplayer->hat_item_id = mySQLdb::Db_GetInt(server->mysql_controls, "eq_hat");
        newplayer->shield_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_shield");
        newplayer->weapon_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_weapon");
        newplayer->ring1_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_ring_l");
        newplayer->ring2_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_ring_r");
        newplayer->armlet1_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_armlet_l");
        newplayer->armlet2_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_armlet_r");
        newplayer->bracer1_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_bracer_l");
        newplayer->bracer2_item_id =
            mySQLdb::Db_GetInt(server->mysql_controls, "eq_bracer_r");

        for (int k = 0; k < 7; k++)
            newplayer->element_resistances[k] = 0;
        newplayer->weight_current = 0;
        newplayer->min_damage = 0;
        newplayer->max_damage = 0;
        newplayer->accuracy = 0;
        newplayer->evasion = 0;
        newplayer->armor = 0;
        newplayer->class_min_damage = 0;
        newplayer->class_max_damage = 0;
        newplayer->class_accuracy = 0;
        newplayer->class_evasion = 0;
        newplayer->class_armor = 0;
        Player_ApplyEquipmentBonuses(server, newplayer);
        Player::UpdateBaseStats(newplayer);
        Player_CalculateStats(server, newplayer);
        Player::CalculateHP_TP_SP(newplayer);
        newplayer->on_chair = 0;
        newplayer->sitting = 0;

        if ((unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "sitting") == 1)
            newplayer->on_chair = 1;
        if ((unsigned int)mySQLdb::Db_GetInt(server->mysql_controls, "sitting") == 2)
            newplayer->sitting = 1;
        newplayer->admin_level = mySQLdb::Db_GetInt(server->mysql_controls, "privilege");
        newplayer->quest_cache =
            mySQLdb::Db_GetString(server->mysql_controls, "questcache");
        newplayer->quest_blob =
            mySQLdb::Db_GetString(server->mysql_controls, "questblob") +
            mySQLdb::Db_GetString(server->mysql_controls, "questblob2");
        newplayer->invblob1 = mySQLdb::Db_GetString(server->mysql_controls, "invblob") +
                              mySQLdb::Db_GetString(server->mysql_controls, "invblob2");
        newplayer->invblob2 = mySQLdb::Db_GetString(server->mysql_controls, "invblob3") +
                              mySQLdb::Db_GetString(server->mysql_controls, "invblob4");
        newplayer->skillblob =
            mySQLdb::Db_GetString(server->mysql_controls, "skillblob") +
            mySQLdb::Db_GetString(server->mysql_controls, "skillblob2");

        if (i < 3)
        {
            player->character_slots[i] = newplayer;
            i++;
        }

        data.Insert(mySQLdb::Db_GetString(server->mysql_controls, "name"),
                    data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(EO_EncodeNumber(
                        server, mySQLdb::Db_GetInt(server->mysql_controls, "ident"), 4),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(
                        server, mySQLdb::Db_GetInt(server->mysql_controls, "level"), 1),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(
                        server, mySQLdb::Db_GetInt(server->mysql_controls, "gender"), 1),
                    data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(
                server, mySQLdb::Db_GetInt(server->mysql_controls, "hairmodal"), 1),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(
                server, mySQLdb::Db_GetInt(server->mysql_controls, "haircolor"), 1),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(
                server, mySQLdb::Db_GetInt(server->mysql_controls, "skincolor"), 1),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(
                server, mySQLdb::Db_GetInt(server->mysql_controls, "privilege"), 1),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(server,
                            ItemValues::GetSpec1ForTypes(
                                GUI->item_values,
                                mySQLdb::Db_GetInt(server->mysql_controls, "eq_boots")),
                            2),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(server,
                            ItemValues::GetSpec1ForTypes(
                                GUI->item_values,
                                mySQLdb::Db_GetInt(server->mysql_controls, "eq_armor")),
                            2),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(server,
                            ItemValues::GetSpec1ForTypes(
                                GUI->item_values,
                                mySQLdb::Db_GetInt(server->mysql_controls, "eq_hat")),
                            2),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(server,
                            ItemValues::GetSpec1ForTypes(
                                GUI->item_values,
                                mySQLdb::Db_GetInt(server->mysql_controls, "eq_shield")),
                            2),
            data.Length() + 1);
        data.Insert(
            EO_EncodeNumber(server,
                            ItemValues::GetSpec1ForTypes(
                                GUI->item_values,
                                mySQLdb::Db_GetInt(server->mysql_controls, "eq_weapon")),
                            2),
            data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);

        GUI->myquery->Next();
    }

    Client_SendEncoded(server, player, action, family, data);
}

String Party_EncodeMemberList(Packets *server, Player *player)
{
    if (!player->in_party)
        return "";
    String s = "";
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        Player *member = Players::Players_GetById(server->players, player->party_ids[i]);
        if (member != NULL)
        {
            int is_leader = 0;
            if (player->party_leader_id == member->player_id)
                is_leader = 1;
            s.Insert(EO_EncodeNumber(server, member->player_id, 2), s.Length() + 1);
            s.Insert(EO_EncodeNumber(server, is_leader, 1), s.Length() + 1);
            s.Insert(EO_EncodeNumber(server, member->level, 1), s.Length() + 1);
            s.Insert(EO_EncodeNumber(server, Player::HpPercent(member), 1),
                     s.Length() + 1);
            s.Insert(member->name, s.Length() + 1);
            s.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), s.Length() + 1);
        }
    }
    return s;
}

String Walk_BuildReply(Packets *server, Player *player)
{
    String buf = "";
    try
    {
        Player **iter;
        Npc **niter;
        ItemObj **iiter;
        for (iter = server->players->players.begin();
             iter != server->players->players.end();
             iter++)
        {
            if ((*iter)->map_id == player->map_id)
            {
                if (Server_InViewRing(
                        server, player->x, player->y, (*iter)->x, (*iter)->y))
                    buf.Insert(EO_EncodeNumber(server, (*iter)->player_id, 2),
                               buf.Length() + 1);
            }
        }
        buf.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), buf.Length() + 1);
        if (player->map_id > 0)
        {
            if (player->map_id <= (int)server->map_control->maps.size())
            {
                for (niter = (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                           player->map_id - 1)
                                 ->npc_list.begin();
                     niter != (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                            player->map_id - 1)
                                  ->npc_list.end();
                     niter++)
                {
                    if (Server_InViewRing(
                            server, player->x, player->y, (*niter)->x, (*niter)->y))
                        buf.Insert(EO_EncodeNumber(server, (*niter)->index, 1),
                                   buf.Length() + 1);
                }
            }
        }
        buf.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), buf.Length() + 1);
        if (player->map_id > 0)
        {
            if (player->map_id <= (int)server->map_control->maps.size())
            {
                for (iiter = (ItemObj **)Mapcontrol_GetByIndex(server->map_control,
                                                               player->map_id - 1)
                                 ->ground_items.begin();
                     iiter != (ItemObj **)Mapcontrol_GetByIndex(server->map_control,
                                                                player->map_id - 1)
                                  ->ground_items.end();
                     iiter++)
                {
                    if (Server_InItemViewRing(
                            server, player->x, player->y, (*iiter)->x, (*iiter)->y))
                    {
                        buf.Insert(EO_EncodeNumber(server, (*iiter)->index, 2),
                                   buf.Length() + 1);
                        buf.Insert(EO_EncodeNumber(server, (*iiter)->item_id, 2),
                                   buf.Length() + 1);
                        buf.Insert(EO_EncodeNumber(server, (*iiter)->x, 1),
                                   buf.Length() + 1);
                        buf.Insert(EO_EncodeNumber(server, (*iiter)->y, 1),
                                   buf.Length() + 1);
                        buf.Insert(EO_EncodeNumber(server, (*iiter)->amount, 3),
                                   buf.Length() + 1);
                    }
                }
            }
        }
    }
    catch (...)
    {
        buf = "";
    }
    return buf;
}

String Refresh_BuildReply(Packets *server, Player *player)
{
    String data = EO_GetBreakByte(server, EO_BREAK_BYTE);
    int count = 0;
    Player **iter;
    try
    {
        for (iter = server->players->players.begin();
             iter != server->players->players.end();
             iter++)
        {
            if ((*iter)->map_id == player->map_id &&
                Server_InViewRange(server, player->x, player->y, (*iter)->x, (*iter)->y))
            {
                count++;
                data.Insert((*iter)->name, data.Length() + 1);
                data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->player_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->map_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->x, 2), data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->y, 2), data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->direction, 1),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->class_id, 1),
                            data.Length() + 1);
                data.Insert((*iter)->guild_tag, data.Length() + 1);
                if ((*iter)->guild_tag.Length() == 2)
                    data.Insert(" ", data.Length() + 1);
                if ((*iter)->guild_tag.Length() == 1)
                    data.Insert("  ", data.Length() + 1);
                if ((*iter)->guild_tag.Length() == 0)
                    data.Insert("   ", data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->level, 1),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->gender, 1),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->hair_style, 1),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->hair_color, 1),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->skin, 1), data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->max_hp, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->hp, 2), data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->max_tp, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->tp, 2), data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->boots_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->accessory_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->gloves_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->belt_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->armor_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->necklace_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->hat_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->shield_graphic_id, 2),
                            data.Length() + 1);
                data.Insert(EO_EncodeNumber(server, (*iter)->weapon_graphic_id, 2),
                            data.Length() + 1);
                int sit_state = 0;
                if ((*iter)->on_chair)
                    sit_state = 1;
                if ((*iter)->sitting)
                    sit_state = 2;
                data.Insert(EO_EncodeNumber(server, sit_state, 1), data.Length() + 1);
                if ((*iter)->hidden)
                    data.Insert(EO_EncodeNumber(server, 1, 1), data.Length() + 1);
                else
                    data.Insert(EO_EncodeNumber(server, 0, 1), data.Length() + 1);
                data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
            }
        }
        data.Insert(EO_EncodeNumber(server, count, 1), 1);
        Npc **niter;
        if (player->map_id > 0 && player->map_id <= (int)server->map_control->maps.size())
        {
            for (niter = (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                       player->map_id - 1)
                             ->npc_list.begin();
                 niter !=
                 (Npc **)Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                     ->npc_list.end();
                 niter++)
            {
                if ((*niter)->alive &&
                    Server_InViewRange(
                        server, player->x, player->y, (*niter)->x, (*niter)->y))
                {
                    data.Insert(EO_EncodeNumber(server, (*niter)->index, 1),
                                data.Length() + 1);
                    data.Insert(EO_EncodeNumber(server, (*niter)->id, 2),
                                data.Length() + 1);
                    if (!player->cheater_flag)
                    {
                        data.Insert(EO_EncodeNumber(server, (*niter)->x, 1),
                                    data.Length() + 1);
                        data.Insert(EO_EncodeNumber(server, (*niter)->y, 1),
                                    data.Length() + 1);
                    }
                    else
                    {
                        int cheat_x = (*niter)->x + server->cheat_offset_x - 1;
                        int cheat_y = (*niter)->y + server->cheat_offset_y - 1;
                        if (cheat_x < 1)
                            cheat_x = 0;
                        if (cheat_y < 1)
                            cheat_y = 0;
                        data.Insert(EO_EncodeNumber(server, cheat_x, 1),
                                    data.Length() + 1);
                        data.Insert(EO_EncodeNumber(server, cheat_y, 1),
                                    data.Length() + 1);
                    }
                    data.Insert(
                        EO_EncodeNumber(server, (unsigned short)(*niter)->direction, 1),
                        data.Length() + 1);
                }
            }
        }
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        ItemObj **iiter;
        if (player->map_id > 0 && player->map_id <= (int)server->map_control->maps.size())
        {
            for (iiter = (ItemObj **)Mapcontrol_GetByIndex(server->map_control,
                                                           player->map_id - 1)
                             ->ground_items.begin();
                 iiter != (ItemObj **)Mapcontrol_GetByIndex(server->map_control,
                                                            player->map_id - 1)
                              ->ground_items.end();
                 iiter++)
            {
                if (Server_InViewRange(
                        server, player->x, player->y, (*iiter)->x, (*iiter)->y))
                {
                    data.Insert(EO_EncodeNumber(server, (*iiter)->index, 2),
                                data.Length() + 1);
                    data.Insert(EO_EncodeNumber(server, (*iiter)->item_id, 2),
                                data.Length() + 1);
                    data.Insert(EO_EncodeNumber(server, (*iiter)->x, 1),
                                data.Length() + 1);
                    data.Insert(EO_EncodeNumber(server, (*iiter)->y, 1),
                                data.Length() + 1);
                    data.Insert(EO_EncodeNumber(server, (*iiter)->amount, 3),
                                data.Length() + 1);
                }
            }
        }
    }
    catch (...)
    {
        data = "NO";
    }
    return data;
}

String Player_SerializeAvatar(Packets *server, Player *player, int arg)
{
    String out = player->name;
    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
    try
    {
        out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->map_id, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->x, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->y, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->direction, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->class_id, 1), out.Length() + 1);
        out.Insert(player->guild_tag, out.Length() + 1);
        if (player->guild_tag.Length() == 2)
            out.Insert(" ", out.Length() + 1);
        if (player->guild_tag.Length() == 1)
            out.Insert("  ", out.Length() + 1);
        if (player->guild_tag.Length() == 0)
            out.Insert("   ", out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->level, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->gender, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->hair_style, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->hair_color, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->skin, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->max_hp, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->hp, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->max_tp, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->tp, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->boots_graphic_id, 2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->accessory_graphic_id, 2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->gloves_graphic_id, 2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->belt_graphic_id, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->armor_graphic_id, 2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->necklace_graphic_id, 2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->hat_graphic_id, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->shield_graphic_id, 2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->weapon_graphic_id, 2),
                   out.Length() + 1);
        int sit_state = 0;
        if (player->on_chair)
            sit_state = 1;
        if (player->sitting)
            sit_state = 2;
        out.Insert(EO_EncodeNumber(server, sit_state, 1), out.Length() + 1);
        if (player->hidden)
            out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
        else
            out.Insert(EO_EncodeNumber(server, 0, 1), out.Length() + 1);
        if (arg < 0)
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        else
        {
            out.Insert(EO_EncodeNumber(server, arg, 1), out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return out;
}

String Player_SerializePaperdoll(Packets *server, Player *player)
{
    String out = "";
    try
    {
        vector<PlayerQuest>::iterator it;
        out.Insert(player->name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        out.Insert(player->home_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        out.Insert(player->partner_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        out.Insert(player->title, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        out.Insert(player->guild_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        out.Insert(player->guild_rank_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->class_id, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->gender, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->admin_level, 1), out.Length() + 1);
        if (player->in_party)
        {
            if (player->admin_level > AdminLevel_Spy)
            {
                if (player->admin_level < AdminLevel_GameMaster)
                    out.Insert(EO_EncodeNumber(server, 9, 1), out.Length() + 1);
                else
                    out.Insert(EO_EncodeNumber(server, 10, 1), out.Length() + 1);
            }
            else
                out.Insert(EO_EncodeNumber(server, 6, 1), out.Length() + 1);
        }
        else
        {
            if (player->admin_level > AdminLevel_Spy)
            {
                if (player->admin_level < AdminLevel_GameMaster)
                    out.Insert(EO_EncodeNumber(server, 4, 1), out.Length() + 1);
                else
                    out.Insert(EO_EncodeNumber(server, 5, 1), out.Length() + 1);
            }
            else
                out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
        }
        out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        for (it = player->quest_history.begin(); it != player->quest_history.end(); it++)
        {
            out.Insert(QuestContainer::GetQuestName(server->quest_engine, it->quest_id),
                       out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return out;
}

String Paperdoll_BuildReply(Packets *server, Player *player)
{
    String data = "";
    try
    {
        data.Insert(player->name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(player->home_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(player->partner_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(player->title, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(player->guild_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(player->guild_rank_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->player_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->class_id, 1), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->gender, 1), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->admin_level, 1), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->boots_item_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->accessory_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->gloves_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->belt_item_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->armor_item_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->necklace_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->hat_item_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->shield_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->weapon_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->ring1_item_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->ring2_item_id, 2), data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->armlet1_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->armlet2_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->bracer1_item_id, 2),
                    data.Length() + 1);
        data.Insert(EO_EncodeNumber(server, player->bracer2_item_id, 2),
                    data.Length() + 1);
        if (player->in_party)
        {
            if (player->admin_level > AdminLevel_Spy)
            {
                if (player->admin_level < AdminLevel_GameMaster)
                    data.Insert(EO_EncodeNumber(server, 9, 1), data.Length() + 1);
                else
                    data.Insert(EO_EncodeNumber(server, 10, 1), data.Length() + 1);
            }
            else
                data.Insert(EO_EncodeNumber(server, 6, 1), data.Length() + 1);
        }
        else
        {
            if (player->admin_level > AdminLevel_Spy)
            {
                if (player->admin_level < AdminLevel_GameMaster)
                    data.Insert(EO_EncodeNumber(server, 4, 1), data.Length() + 1);
                else
                    data.Insert(EO_EncodeNumber(server, 5, 1), data.Length() + 1);
            }
            else
                data.Insert(EO_EncodeNumber(server, 1, 1), data.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return data;
}

String Server_BuildOnlineNames(Packets *server)
{
    if (server->online_names_ttl < 1)
    {
        String names = EO_GetBreakByte(server, EO_BREAK_BYTE);
        int count = 0;
        for (Player **iter = server->players->players.begin();
             iter != server->players->players.end();
             iter++)
        {
            if ((*iter)->logged_in && !(*iter)->hide_online)
            {
                names.Insert((*iter)->name, names.Length() + 1);
                names.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), names.Length() + 1);
                count++;
            }
        }
        names.Insert(EO_EncodeNumber(server, count, 2), 1);
        if (count > 0x18)
        {
            server->online_names_cache = names;
            server->online_names_ttl = 4;
        }
        return names;
    }
    else
    {
        return server->online_names_cache;
    }
}

String Message_BuildServerStatus(Packets *server)
{
    String names = EO_GetBreakByte(server, EO_BREAK_BYTE);
    int count = 0;
    for (Player **iter = server->players->players.begin();
         iter != server->players->players.end();
         iter++)
    {
        if ((*iter)->logged_in && !(*iter)->hide_online)
        {
            names.Insert((*iter)->name, names.Length() + 1);
            names.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), names.Length() + 1);
            names.Insert((*iter)->title, names.Length() + 1);
            names.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->level, 1), names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->experience, 4),
                         names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->gender, 1), names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->admin_level, 1),
                         names.Length() + 1);
            names.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), names.Length() + 1);
            count++;
        }
    }
    names.Insert(EO_EncodeNumber(server, count, 2), 1);
    return names;
}

String Server_BuildOnlineList(Packets *server)
{
    if (server->online_list_ttl < 1)
    {
        String list = EO_GetBreakByte(server, EO_BREAK_BYTE);
        int count = 0;
        for (Player **iter = server->players->players.begin();
             iter != server->players->players.end();
             iter++)
        {
            if ((*iter)->logged_in && !(*iter)->hide_online)
            {
                list.Insert((*iter)->name, list.Length() + 1);
                list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
                list.Insert((*iter)->title, list.Length() + 1);
                list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
                list.Insert(EO_EncodeNumber(server, (*iter)->level, 1),
                            list.Length() + 1);
                if ((*iter)->in_party)
                {
                    if ((*iter)->admin_level > AdminLevel_Spy)
                    {
                        if ((*iter)->admin_level < AdminLevel_GameMaster)
                            list.Insert(EO_EncodeNumber(server, 9, 1), list.Length() + 1);
                        else
                            list.Insert(EO_EncodeNumber(server, 10, 1),
                                        list.Length() + 1);
                    }
                    else
                        list.Insert(EO_EncodeNumber(server, 6, 1), list.Length() + 1);
                }
                else
                {
                    if ((*iter)->admin_level > AdminLevel_Spy)
                    {
                        if ((*iter)->admin_level < AdminLevel_GameMaster)
                            list.Insert(EO_EncodeNumber(server, 4, 1), list.Length() + 1);
                        else
                            list.Insert(EO_EncodeNumber(server, 5, 1), list.Length() + 1);
                    }
                    else
                        list.Insert(EO_EncodeNumber(server, 1, 1), list.Length() + 1);
                }
                list.Insert(EO_EncodeNumber(server, (*iter)->class_id, 1),
                            list.Length() + 1);
                list.Insert((*iter)->guild_tag, list.Length() + 1);
                if ((*iter)->guild_tag.Length() == 2)
                    list.Insert(" ", list.Length() + 1);
                if ((*iter)->guild_tag.Length() == 1)
                    list.Insert("  ", list.Length() + 1);
                if ((*iter)->guild_tag.Length() == 0)
                    list.Insert("   ", list.Length() + 1);
                list.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), list.Length() + 1);
                count++;
            }
        }
        list.Insert(EO_EncodeNumber(server, count, 2), 1);
        if (count >= 25)
        {
            server->online_list_cache = list;
            server->online_list_ttl = 4;
        }
        return list;
    }
    else
    {
        return server->online_list_cache;
    }
}

String NpcRange_Lookup(Packets *server, Player *player, unsigned int npc_index)
{
    String fragment = "";
    Npc **iter;
    try
    {
        if (player->map_id > 0)
        {
            if (player->map_id <= (int)server->map_control->maps.size())
            {
                for (iter = (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                          player->map_id - 1)
                                ->npc_list.begin();
                     iter != (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                           player->map_id - 1)
                                 ->npc_list.end();
                     iter++)
                {
                    if ((*iter)->index == npc_index && (*iter)->alive)
                    {
                        fragment.Insert(EO_EncodeNumber(server, (*iter)->index, 1),
                                        fragment.Length() + 1);
                        fragment.Insert(EO_EncodeNumber(server, (*iter)->id, 2),
                                        fragment.Length() + 1);
                        if (!player->cheater_flag)
                        {
                            fragment.Insert(EO_EncodeNumber(server, (*iter)->x, 1),
                                            fragment.Length() + 1);
                            fragment.Insert(EO_EncodeNumber(server, (*iter)->y, 1),
                                            fragment.Length() + 1);
                        }
                        else
                        {
                            int cheat_x = (*iter)->x + server->cheat_offset_x - 1;
                            int cheat_y = (*iter)->y + server->cheat_offset_y - 1;
                            if (cheat_x < 1)
                                cheat_x = 0;
                            if (cheat_y < 1)
                                cheat_y = 0;
                            fragment.Insert(EO_EncodeNumber(server, cheat_x, 1),
                                            fragment.Length() + 1);
                            fragment.Insert(EO_EncodeNumber(server, cheat_y, 1),
                                            fragment.Length() + 1);
                        }
                        fragment.Insert(
                            EO_EncodeNumber(
                                server, (unsigned short)(*iter)->direction, 1),
                            fragment.Length() + 1);
                        break;
                    }
                }
            }
        }
    }
    catch (...)
    {
    }
    return fragment;
}

bool FUN_00462374(Packets *server, Player *player, String data)
{
    if (data.Length() > 8 && data.Length() < 0x2a)
    {
        if (EO_DecodeByte(server, data[1]) == 0xff &&
            EO_DecodeByte(server, data[2]) == 0xff)
        {
            if (data.Length() > 10)
            {
                int hdid_len = EO_DecodeNumber(server, data[10]);
                player->hdid = data.SubString(11, data.Length() - 10);
                if (player->hdid.Length() != hdid_len)
                {
                    player->socket->SendText(Server_BuildInitBanReply(server));
                    return false;
                }
                if (Banned::IsBanned(server->banned, player->remote_ip, player->hdid))
                {
                    player->socket->SendText(Server_BuildInitBanReply(server));
                    return false;
                }
            }
            int session =
                FUN_00470598((int)server, EO_DecodeNumber(server, data.SubString(3, 3)));
            int patch = EO_DecodeNumber(server, data[6]);
            int minor = EO_DecodeNumber(server, data[7]);
            int major = EO_DecodeNumber(server, data[8]);
            int magic = EO_DecodeNumber(server, data[9]);
            if (patch >= server->version_patch && minor >= server->version_minor &&
                major >= server->version_major && magic == 0x70)
            {
                if (patch < 10)
                {
                    if (GUI->server->Socket->ActiveConnections >
                        Settings::GetMaxConnections(server->settings))
                        player->remove_timer = 7;
                    else
                        player->remove_timer = -1;
                    player->field_0x34 = session;
                    player->initialized = true;
                    player->socket->SendText(Server_BuildInitOkReply(server, player));
                    return true;
                }
            }
            else
            {
                player->socket->SendText(Server_BuildInitVersionReply(server));
                return false;
            }
        }
    }
    Logins::AddLogin(server->logins, player->remote_ip);
    return false;
}

String Server_BuildInitOkReply(Packets *server, Player *player)
{
    int total = server->ping_history[0] + 0x0d;
    int major = total / 7;
    int minor = total % 7;
    String out = EO_GetBreakByte(server, EO_BREAK_BYTE);
    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, 2), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, major), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, minor), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, player->server_encryption_multiple),
               out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, player->client_encryption_multiple),
               out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, player->field_0x34, 3), out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, out.Length(), 2), 1);
    return out;
}

String Server_BuildInitVersionReply(Packets *server)
{
    String out = EO_GetBreakByte(server, EO_BREAK_BYTE);
    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, 1), out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, server->version_patch, 1), out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, server->version_minor, 1), out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, server->version_major, 1), out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, out.Length(), 2), 1);
    return out;
}

String Server_BuildInitBanReply(Packets *server)
{
    String out = EO_GetBreakByte(server, EO_BREAK_BYTE);
    out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, 3), out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, Banned::GetBanType(server->banned)),
               out.Length() + 1);
    out.Insert(EO_GetBreakByte(server, Banned::GetBanTime(server->banned)),
               out.Length() + 1);
    out.Insert(EO_EncodeNumber(server, out.Length(), 2), 1);
    return out;
}

void Client_SendRaw(Packets *server, Player *client, String data, int break_byte)
{
    FILE *fp;
    if (data.Length() > 62000)
    {
        String msg = DateToStr(Now());
        msg.Insert(" ", msg.Length() + 1);
        msg.Insert(TimeToStr(Now()), msg.Length() + 1);
        msg.Insert(" EndlServ ", msg.Length() + 1);
        msg.Insert("Too large uncoded packet dropped: " + IntToStr(break_byte),
                   msg.Length() + 1);
        msg.Insert("\n", msg.Length() + 1);
        fp = fopen("error.log", "a");
        fprintf(fp, "%s", msg.c_str());
        fclose(fp);
    }
    if (client->removing)
        return;
    if (!client->connected)
        return;
    String built = String(EO_GetBreakByte(server, EO_BREAK_BYTE));
    built.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), built.Length() + 1);
    built.Insert(EO_GetBreakByte(server, break_byte), built.Length() + 1);
    built.Insert(data, built.Length() + 1);
    built.Insert(EO_EncodeNumber(server, built.Length(), 2), 1);
    client->socket->SendText(built);
}

void Talk_PlayerWhisper(Packets *server, int map_id, String message, int break_byte)
{
    for (Player **player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_id == map_id && (*player_iter)->logged_in &&
            !(*player_iter)->removing)
        {
            String out = EO_GetBreakByte(server, EO_BREAK_BYTE);
            out.Insert(EO_GetBreakByte(server, EO_BREAK_BYTE), out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, break_byte), out.Length() + 1);
            out.Insert(message, out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, out.Length(), 2), 1);
            (*player_iter)->socket->SendText(out);
        }
    }
}

void Server_BroadcastToPartyExceptSelf(Packets *server,
                                       Player *player,
                                       unsigned char action,
                                       unsigned char family,
                                       String data)
{
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        if (player->party_ids[i] != player->player_id)
        {
            Player *member =
                Players::Players_GetById(server->players, player->party_ids[i]);
            if (member != NULL && member->logged_in)
                Client_SendEncoded(server, member, action, family, data);
        }
    }
}

void Server_BroadcastToParty(Packets *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data)
{
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        Player *member = Players::Players_GetById(server->players, player->party_ids[i]);
        if (member != NULL && member->logged_in)
            Client_SendEncoded(server, member, action, family, data);
    }
}

void Server_BroadcastToPartyOnMap(Packets *server,
                                  Player *player,
                                  unsigned char action,
                                  unsigned char family,
                                  String data)
{
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        Player *member = Players::Players_GetById(server->players, player->party_ids[i]);
        if (member != NULL && player->map_id == member->map_id && member->logged_in)
            Client_SendEncoded(server, member, action, family, data);
    }
}

void Guild_BroadcastToAll(Packets *server,
                          Player *player,
                          unsigned char action,
                          unsigned char family,
                          String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->guild_tag == player->guild_tag)
        {
            if ((*player_iter)->player_id != player->player_id)
            {
                if ((*player_iter)->logged_in)
                    Client_SendEncoded(server, *player_iter, action, family, data);
            }
        }
    }
}

void Server_BroadcastAdjacent(Packets *server,
                              Player *player,
                              MapCoord coords,
                              unsigned char action,
                              unsigned char family,
                              String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_id == player->map_id &&
            Coords_IsAdjacent(
                server, coords.x, coords.y, (*player_iter)->x, (*player_iter)->y) &&
            (*player_iter)->logged_in && (*player_iter)->player_id != player->player_id)
        {
            Client_SendEncoded(server, *player_iter, action, family, data);
        }
    }
}

void Server_BroadcastToMapAndAdmins(
    Packets *server, int map_id, unsigned char action, unsigned char family, String data)
{
    for (Player **player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_id == map_id ||
            (*player_iter)->admin_level > AdminLevel_Player)
        {
            if ((*player_iter)->logged_in)
                Client_SendEncoded(server, *player_iter, action, family, data);
        }
    }
}

void Server_BroadcastToMap(
    Packets *server, int map_id, unsigned char action, unsigned char family, String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_id == map_id && (*player_iter)->logged_in)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Server_BroadcastNearTile(Packets *server,
                              int skip_id,
                              int map_id,
                              MapCoord coord,
                              unsigned char action,
                              unsigned char family,
                              String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_id == map_id && (*player_iter)->player_id != skip_id &&
            Server_InViewRange(
                server, coord.x, coord.y, (*player_iter)->x, (*player_iter)->y))
        {
            Client_SendEncoded(server, *player_iter, action, family, data);
        }
    }
}

void Admin_BroadcastToOtherAdmins(Packets *server,
                                  Player *player,
                                  unsigned char action,
                                  unsigned char family,
                                  String data)
{
    for (Player **player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->admin_level > AdminLevel_Player &&
            (*player_iter)->logged_in && (*player_iter)->player_id != player->player_id)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Admin_BroadcastToAll(Packets *server,
                          unsigned char action,
                          unsigned char family,
                          String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->admin_level > AdminLevel_Player && (*player_iter)->logged_in)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Server_BroadcastToAll(Packets *server,
                           unsigned char action,
                           unsigned char family,
                           String data)
{
    for (Player **player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->logged_in)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Admin_ReportToGMs(Packets *server,
                       Player *player,
                       unsigned char action,
                       unsigned char family,
                       String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->global_chat && (*player_iter)->logged_in &&
            (*player_iter)->player_id != player->player_id)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Admin_BroadcastToAdmins(Packets *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->logged_in && (*player_iter)->player_id != player->player_id)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Server_BroadcastNearby(Packets *server,
                            Player *player,
                            unsigned char action,
                            unsigned char family,
                            String data)
{
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_id == player->map_id &&
            Server_InViewRangeReverse(
                server, player->x, player->y, (*player_iter)->x, (*player_iter)->y) &&
            (*player_iter)->logged_in && (*player_iter)->player_id != player->player_id)
        {
            Client_SendEncoded(server, *player_iter, action, family, data);
        }
    }
}

void Client_SendEncoded(Packets *server,
                        Player *player,
                        unsigned char action,
                        unsigned char family,
                        String data)
{
    if (data.Length() > 20000)
    {
        String msg = DateToStr(Now());
        msg.Insert(" ", msg.Length() + 1);
        msg.Insert(TimeToStr(Now()), msg.Length() + 1);
        msg.Insert(" EndlServ ", msg.Length() + 1);
        msg.Insert("Too large encoded packet dropped: " + IntToStr(action) + "," +
                       IntToStr(family),
                   msg.Length() + 1);
        msg.Insert("\n", msg.Length() + 1);
        FILE *fp;
        fp = fopen("error.log", "a");
        fprintf(fp, "%s", msg.c_str());
        fclose(fp);
    }
    if (!player->removing)
    {
        char family_byte = (char)family;
        char action_byte = (char)action;
        String out = String(action_byte);
        out.Insert(family_byte, out.Length() + 1);
        out.Insert(data, out.Length() + 1);
        std::basic_string<char> range(out.c_str());
        out = EO_Encode_Interleave(server,
                                   player->server_encryption_multiple,
                                   (char *)range.begin(),
                                   (char *)range.end());
        for (int i = 1; i <= out.Length(); i++)
        {
            int c = (unsigned char)out[i];
            if (c < 0x80)
                out[i] += (char)0x80;
            if (c > 0x80)
                out[i] += (char)0x80;
        }
        out.Insert(EO_EncodeNumber(server, out.Length(), 2), 1);
        Server_AddSentBytes(server, out.Length());
        player->socket->SendText(out);
    }
}

void FUN_00470584();

void Player_Respawn(Packets *server, Player *player)
{
    MapCoord coords;
    int map_id = InnValues::GetSpawnMap(GUI->inn_values, player->home_id, player->level);
    if (map_id < 0)
    {
        map_id = Settings::GetRescueMap(server->settings);
        coords.x = Settings::GetRescueX(server->settings);
        coords.y = Settings::GetRescueY(server->settings);
    }
    else
    {
        coords.x = InnValues::GetSpawnX(GUI->inn_values, player->home_id, player->level);
        coords.y = InnValues::GetSpawnY(GUI->inn_values, player->home_id, player->level);
    }
    if (player->level <= 3)
    {
        map_id = Settings::GetStartMap(server->settings);
        coords.x = Settings::GetStartX(server->settings);
        coords.y = Settings::GetStartY(server->settings);
    }
    player->flush_queue = 1;
    Player_Warp(server, player, map_id, coords, WarpEffect_None, true);
}

void Player_Warp(Packets *server,
                 Player *player,
                 int target_map,
                 MapCoord coords,
                 int warp_effect,
                 bool do_leave)
{
    if (target_map == 0x50 || target_map == 0x51)
        return;
    if (target_map < 1 && (int)server->map_control->maps.size() < target_map)
        return;
    if (Mapcontrol_GetByIndex(server->map_control, target_map - 1)->width < 1 ||
        Mapcontrol_GetByIndex(server->map_control, target_map - 1)->height < 1)
        return;
    if (player->warp_state < 0)
        player->session_id = RandRange(50000) + 10000;
    int old_map = player->map_id;
    player->warp_state = warp_effect;
    player->warp_pending = true;
    player->dead = false;
    player->warp_map = target_map;
    player->warp_x = coords.x;
    player->warp_y = coords.y;
    if (do_leave)
    {
        Server_BroadcastNearby(server,
                               player,
                               PacketAction_Remove,
                               PacketFamily_Avatar,
                               EO_EncodeNumber(server, player->player_id, 2) +
                                   EO_EncodeNumber(server, player->warp_state, 1));
        if (target_map > 0)
        {
            player->target_map = target_map;
            player->target_x = coords.x;
            player->target_y = coords.y;
        }
        if (player->map_id > 0)
        {
            player->map_has_quakes = false;
            player->map_has_hp_drain = false;
            player->map_has_tp_drain = false;
            player->map_has_spikes = false;
            MapContainer::Mapcontrol_DecPlayerCount(server->map_control, player->map_id);
        }
        player->map_id = 0;
        player->x = 0;
        player->y = 0;
        player->idle_ticks = 0;
        player->read_len = -1;
        player->guild_inviter_id = -1;
        player->session_token = -1;
    }
    if (target_map == old_map)
    {
        String out = EO_EncodeNumber(server, WarpType_Local, 1);
        out.Insert(EO_EncodeNumber(
                       server,
                       Mapcontrol_GetByIndex(server->map_control, target_map - 1)->rid,
                       2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->session_id, 2), out.Length() + 1);
        Client_SendEncoded(server, player, PacketAction_Request, PacketFamily_Warp, out);
    }
    else
    {
        String out = EO_EncodeNumber(server, WarpType_MapSwitch, 1);
        out.Insert(EO_EncodeNumber(
                       server,
                       Mapcontrol_GetByIndex(server->map_control, target_map - 1)->rid,
                       2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server,
                                   (unsigned short)Mapcontrol_GetByIndex(
                                       server->map_control, target_map - 1)
                                       ->rid1,
                                   2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server,
                                   (unsigned short)Mapcontrol_GetByIndex(
                                       server->map_control, target_map - 1)
                                       ->rid2,
                                   2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server,
                                   (unsigned short)Mapcontrol_GetByIndex(
                                       server->map_control, target_map - 1)
                                       ->filesize,
                                   3),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->session_id, 2), out.Length() + 1);
        Client_SendEncoded(server, player, PacketAction_Request, PacketFamily_Warp, out);
        Player_FireQuestTriggers(server, player, 0xc, old_map);
    }
}

bool Player_CheckIdleWarp(Packets *server, Player *player, int x, int y)
{
    if (MapContainer::Mapcontrol_IsTileWalkable(
            server->map_control, player->map_id, x, y))
    {
        MapCoord coords;
        coords.x = x;
        coords.y = y;
        if (MapContainer::Mapcontrol_GetWarpDoorAt(
                server->map_control, player->map_id, coords) < 2)
        {
            MapCoord dest;
            int target_map = MapContainer::Mapcontrol_GetWarpMap(
                server->map_control, player->map_id, x, y);
            int level_req = MapContainer::Mapcontrol_GetWarpLevelReq(
                server->map_control, player->map_id, x, y);
            dest.x = MapContainer::Mapcontrol_GetWarpX(
                server->map_control, player->map_id, x, y);
            dest.y = MapContainer::Mapcontrol_GetWarpY(
                server->map_control, player->map_id, x, y);
            if (player->level < level_req)
                return false;
            player->flush_queue = 1;
            Player_Warp(server, player, target_map, dest, WarpEffect_None, false);
            return true;
        }
    }
    return false;
}

void Player_CalculateStats(Packets *server, Player *player)
{
    if (player->class_id < 1)
        return;
    ClassValue cls = ClassValues::GetByIndex(GUI->class_values, player->class_id - 1);
    player->adj_strength = player->adj_strength + cls.str;
    player->adj_intelligence = player->adj_intelligence + cls.intl;
    player->adj_wisdom = player->adj_wisdom + cls.wis;
    player->adj_agility = player->adj_agility + cls.agi;
    player->adj_constitution = player->adj_constitution + cls.con;
    player->adj_charisma = player->adj_charisma + cls.cha;
    player->min_damage = player->min_damage - player->class_min_damage;
    player->max_damage = player->max_damage - player->class_max_damage;
    player->accuracy = player->accuracy - player->class_accuracy;
    player->evasion = player->evasion - player->class_evasion;
    player->armor = player->armor - player->class_armor;
    player->class_min_damage = 0;
    player->class_max_damage = 0;
    player->class_accuracy = 0;
    player->class_evasion = 0;
    player->class_armor = 0;
    if (cls.stat_group == 0)
    {
        player->class_min_damage =
            player->class_min_damage + (short)(player->adj_strength / 3);
        player->class_max_damage =
            player->class_max_damage + (short)(player->adj_strength / 3);
        player->class_accuracy =
            player->class_accuracy + (short)(player->adj_agility / 3);
        player->class_evasion = player->class_evasion + (short)(player->adj_agility / 5);
        player->class_armor = player->class_armor + (short)(player->adj_constitution / 4);
    }
    if (cls.stat_group == 1)
    {
        player->class_min_damage =
            player->class_min_damage + (short)(player->adj_strength / 5);
        player->class_max_damage =
            player->class_max_damage + (short)(player->adj_strength / 5);
        player->class_accuracy =
            player->class_accuracy + (short)(player->adj_agility / 3);
        player->class_evasion = player->class_evasion + (short)(player->adj_agility / 3);
        player->class_armor = player->class_armor + (short)(player->adj_constitution / 4);
    }
    if (cls.stat_group == 2)
    {
        player->class_min_damage =
            player->class_min_damage + (short)(player->adj_intelligence / 3);
        player->class_max_damage =
            player->class_max_damage + (short)(player->adj_intelligence / 3);
        player->class_accuracy = player->class_accuracy + (short)(player->adj_wisdom / 3);
        player->class_evasion = player->class_evasion + (short)(player->adj_agility / 4);
        player->class_armor = player->class_armor + (short)(player->adj_constitution / 5);
    }
    if (cls.stat_group == 3)
    {
        player->class_min_damage =
            player->class_min_damage + (short)(player->adj_strength / 6);
        player->class_max_damage =
            player->class_max_damage + (short)(player->adj_strength / 6);
        player->class_accuracy =
            player->class_accuracy + (short)(player->adj_agility / 5);
        player->class_evasion = player->class_evasion + (short)(player->adj_agility / 4);
        player->class_armor = player->class_armor + (short)(player->adj_constitution / 5);
    }
    player->min_damage = player->min_damage + player->class_min_damage;
    player->max_damage = player->max_damage + player->class_max_damage;
    player->accuracy = player->accuracy + player->class_accuracy;
    player->evasion = player->evasion + player->class_evasion;
    player->armor = player->armor + player->class_armor;
}

void Player_ApplyEquipmentBonuses(Packets *server, Player *player)
{
    player->equip_bonus_hp = 0;
    player->equip_bonus_tp = 0;
    player->equip_strength_bonus = 0;
    player->equip_wisdom_bonus = 0;
    player->equip_intelligence_bonus = 0;
    player->equip_agility_bonus = 0;
    player->equip_constitution_bonus = 0;
    player->equip_charisma_bonus = 0;
    if (ItemValues::GetType(GUI->item_values, player->boots_item_id) == ItemType_Boots)
    {
        ItemValue *boots =
            ItemValues::GetByIndex(GUI->item_values, player->boots_item_id - 1);
        if (boots->element < 7)
        {
            player->element_resistances[boots->element] =
                player->element_resistances[boots->element] + boots->element_damage;
        }
        player->boots_graphic_id = boots->spec1;
        player->weight_current = player->weight_current + (int)boots->weight;
        player->min_damage = player->min_damage + boots->min_damage;
        player->max_damage = player->max_damage + boots->max_damage;
        player->accuracy = player->accuracy + boots->accuracy;
        player->evasion = player->evasion + boots->evade;
        player->armor = player->armor + boots->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)boots->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)boots->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)boots->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)boots->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)boots->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)boots->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)boots->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)boots->charisma;
    }
    else
    {
        player->boots_item_id = 0;
        player->boots_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->accessory_item_id) ==
        ItemType_Accessory)
    {
        ItemValue *accessory =
            ItemValues::GetByIndex(GUI->item_values, player->accessory_item_id - 1);
        if (accessory->element < 7)
        {
            player->element_resistances[accessory->element] =
                player->element_resistances[accessory->element] +
                accessory->element_damage;
        }
        player->accessory_graphic_id = accessory->spec1;
        player->weight_current = player->weight_current + (int)accessory->weight;
        player->min_damage = player->min_damage + accessory->min_damage;
        player->max_damage = player->max_damage + accessory->max_damage;
        player->accuracy = player->accuracy + accessory->accuracy;
        player->evasion = player->evasion + accessory->evade;
        player->armor = player->armor + accessory->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)accessory->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)accessory->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)accessory->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)accessory->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)accessory->intelligence;
        player->equip_agility_bonus =
            player->equip_agility_bonus + (int)accessory->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)accessory->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)accessory->charisma;
    }
    else
    {
        player->accessory_item_id = 0;
        player->accessory_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->gloves_item_id) == ItemType_Gloves)
    {
        ItemValue *gloves =
            ItemValues::GetByIndex(GUI->item_values, player->gloves_item_id - 1);
        if (gloves->element < 7)
        {
            player->element_resistances[gloves->element] =
                player->element_resistances[gloves->element] + gloves->element_damage;
        }
        player->gloves_graphic_id = gloves->spec1;
        player->weight_current = player->weight_current + (int)gloves->weight;
        player->min_damage = player->min_damage + gloves->min_damage;
        player->max_damage = player->max_damage + gloves->max_damage;
        player->accuracy = player->accuracy + gloves->accuracy;
        player->evasion = player->evasion + gloves->evade;
        player->armor = player->armor + gloves->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)gloves->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)gloves->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)gloves->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)gloves->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)gloves->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)gloves->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)gloves->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)gloves->charisma;
    }
    else
    {
        player->gloves_item_id = 0;
        player->gloves_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->armor_item_id) == ItemType_Armor)
    {
        ItemValue *armor =
            ItemValues::GetByIndex(GUI->item_values, player->armor_item_id - 1);
        if (armor->element < 7)
        {
            player->element_resistances[armor->element] =
                player->element_resistances[armor->element] + armor->element_damage;
        }
        player->armor_graphic_id = armor->spec1;
        player->weight_current = player->weight_current + (int)armor->weight;
        player->min_damage = player->min_damage + armor->min_damage;
        player->max_damage = player->max_damage + armor->max_damage;
        player->accuracy = player->accuracy + armor->accuracy;
        player->evasion = player->evasion + armor->evade;
        player->armor = player->armor + armor->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)armor->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)armor->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)armor->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)armor->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)armor->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)armor->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)armor->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)armor->charisma;
    }
    else
    {
        player->armor_item_id = 0;
        player->armor_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->belt_item_id) == ItemType_Belt)
    {
        ItemValue *belt =
            ItemValues::GetByIndex(GUI->item_values, player->belt_item_id - 1);
        if (belt->element < 7)
        {
            player->element_resistances[belt->element] =
                player->element_resistances[belt->element] + belt->element_damage;
        }
        player->belt_graphic_id = belt->spec1;
        player->weight_current = player->weight_current + (int)belt->weight;
        player->min_damage = player->min_damage + belt->min_damage;
        player->max_damage = player->max_damage + belt->max_damage;
        player->accuracy = player->accuracy + belt->accuracy;
        player->evasion = player->evasion + belt->evade;
        player->armor = player->armor + belt->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)belt->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)belt->tp;
        player->equip_strength_bonus = player->equip_strength_bonus + (int)belt->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)belt->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)belt->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)belt->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)belt->constitution;
        player->equip_charisma_bonus = player->equip_charisma_bonus + (int)belt->charisma;
    }
    else
    {
        player->belt_item_id = 0;
        player->belt_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->necklace_item_id) ==
        ItemType_Necklace)
    {
        ItemValue *necklace =
            ItemValues::GetByIndex(GUI->item_values, player->necklace_item_id - 1);
        if (necklace->element < 7)
        {
            player->element_resistances[necklace->element] =
                player->element_resistances[necklace->element] + necklace->element_damage;
        }
        player->necklace_graphic_id = necklace->spec1;
        player->weight_current = player->weight_current + (int)necklace->weight;
        player->min_damage = player->min_damage + necklace->min_damage;
        player->max_damage = player->max_damage + necklace->max_damage;
        player->accuracy = player->accuracy + necklace->accuracy;
        player->evasion = player->evasion + necklace->evade;
        player->armor = player->armor + necklace->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)necklace->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)necklace->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)necklace->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)necklace->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)necklace->intelligence;
        player->equip_agility_bonus =
            player->equip_agility_bonus + (int)necklace->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)necklace->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)necklace->charisma;
    }
    else
    {
        player->necklace_item_id = 0;
        player->necklace_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->hat_item_id) == ItemType_Hat)
    {
        ItemValue *hat =
            ItemValues::GetByIndex(GUI->item_values, player->hat_item_id - 1);
        if (hat->element < 7)
        {
            player->element_resistances[hat->element] =
                player->element_resistances[hat->element] + hat->element_damage;
        }
        player->hat_graphic_id = hat->spec1;
        player->weight_current = player->weight_current + (int)hat->weight;
        player->min_damage = player->min_damage + hat->min_damage;
        player->max_damage = player->max_damage + hat->max_damage;
        player->accuracy = player->accuracy + hat->accuracy;
        player->evasion = player->evasion + hat->evade;
        player->armor = player->armor + hat->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)hat->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)hat->tp;
        player->equip_strength_bonus = player->equip_strength_bonus + (int)hat->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)hat->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)hat->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)hat->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)hat->constitution;
        player->equip_charisma_bonus = player->equip_charisma_bonus + (int)hat->charisma;
    }
    else
    {
        player->hat_item_id = 0;
        player->hat_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->shield_item_id) == ItemType_Shield)
    {
        ItemValue *shield =
            ItemValues::GetByIndex(GUI->item_values, player->shield_item_id - 1);
        if (shield->element < 7)
        {
            player->element_resistances[shield->element] =
                player->element_resistances[shield->element] + shield->element_damage;
        }
        player->shield_graphic_id = shield->spec1;
        player->weight_current = player->weight_current + (int)shield->weight;
        player->min_damage = player->min_damage + shield->min_damage;
        player->max_damage = player->max_damage + shield->max_damage;
        player->accuracy = player->accuracy + shield->accuracy;
        player->evasion = player->evasion + shield->evade;
        player->armor = player->armor + shield->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)shield->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)shield->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)shield->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)shield->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)shield->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)shield->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)shield->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)shield->charisma;
    }
    else
    {
        player->shield_item_id = 0;
        player->shield_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->weapon_item_id) == ItemType_Weapon)
    {
        ItemValue *weapon =
            ItemValues::GetByIndex(GUI->item_values, player->weapon_item_id - 1);
        if (weapon->element < 7)
        {
            player->element_resistances[weapon->element] =
                player->element_resistances[weapon->element] + weapon->element_damage;
        }
        player->weapon_graphic_id = weapon->spec1;
        player->weight_current = player->weight_current + (int)weapon->weight;
        player->min_damage = player->min_damage + weapon->min_damage;
        player->max_damage = player->max_damage + weapon->max_damage;
        player->accuracy = player->accuracy + weapon->accuracy;
        player->evasion = player->evasion + weapon->evade;
        player->armor = player->armor + weapon->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)weapon->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)weapon->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)weapon->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)weapon->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)weapon->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)weapon->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)weapon->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)weapon->charisma;
    }
    else
    {
        player->weapon_item_id = 0;
        player->weapon_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->ring1_item_id) == ItemType_Ring)
    {
        ItemValue *ring1 =
            ItemValues::GetByIndex(GUI->item_values, player->ring1_item_id - 1);
        if (ring1->element < 7)
        {
            player->element_resistances[ring1->element] =
                player->element_resistances[ring1->element] + ring1->element_damage;
        }
        player->ring1_graphic_id = ring1->spec1;
        player->weight_current = player->weight_current + (int)ring1->weight;
        player->min_damage = player->min_damage + ring1->min_damage;
        player->max_damage = player->max_damage + ring1->max_damage;
        player->accuracy = player->accuracy + ring1->accuracy;
        player->evasion = player->evasion + ring1->evade;
        player->armor = player->armor + ring1->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)ring1->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)ring1->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)ring1->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)ring1->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)ring1->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)ring1->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)ring1->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)ring1->charisma;
    }
    else
    {
        player->ring1_item_id = 0;
        player->ring1_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->ring2_item_id) == ItemType_Ring)
    {
        ItemValue *ring2 =
            ItemValues::GetByIndex(GUI->item_values, player->ring2_item_id - 1);
        if (ring2->element < 7)
        {
            player->element_resistances[ring2->element] =
                player->element_resistances[ring2->element] + ring2->element_damage;
        }
        player->ring2_graphic_id = ring2->spec1;
        player->weight_current = player->weight_current + (int)ring2->weight;
        player->min_damage = player->min_damage + ring2->min_damage;
        player->max_damage = player->max_damage + ring2->max_damage;
        player->accuracy = player->accuracy + ring2->accuracy;
        player->evasion = player->evasion + ring2->evade;
        player->armor = player->armor + ring2->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)ring2->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)ring2->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)ring2->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)ring2->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)ring2->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)ring2->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)ring2->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)ring2->charisma;
    }
    else
    {
        player->ring2_item_id = 0;
        player->ring2_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->armlet1_item_id) == ItemType_Armlet)
    {
        ItemValue *armlet1 =
            ItemValues::GetByIndex(GUI->item_values, player->armlet1_item_id - 1);
        if (armlet1->element < 7)
        {
            player->element_resistances[armlet1->element] =
                player->element_resistances[armlet1->element] + armlet1->element_damage;
        }
        player->armlet1_graphic_id = armlet1->spec1;
        player->weight_current = player->weight_current + (int)armlet1->weight;
        player->min_damage = player->min_damage + armlet1->min_damage;
        player->max_damage = player->max_damage + armlet1->max_damage;
        player->accuracy = player->accuracy + armlet1->accuracy;
        player->evasion = player->evasion + armlet1->evade;
        player->armor = player->armor + armlet1->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)armlet1->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)armlet1->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)armlet1->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)armlet1->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)armlet1->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)armlet1->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)armlet1->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)armlet1->charisma;
    }
    else
    {
        player->armlet1_item_id = 0;
        player->armlet1_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->armlet2_item_id) == ItemType_Armlet)
    {
        ItemValue *armlet2 =
            ItemValues::GetByIndex(GUI->item_values, player->armlet2_item_id - 1);
        if (armlet2->element < 7)
        {
            player->element_resistances[armlet2->element] =
                player->element_resistances[armlet2->element] + armlet2->element_damage;
        }
        player->armlet2_graphic_id = armlet2->spec1;
        player->weight_current = player->weight_current + (int)armlet2->weight;
        player->min_damage = player->min_damage + armlet2->min_damage;
        player->max_damage = player->max_damage + armlet2->max_damage;
        player->accuracy = player->accuracy + armlet2->accuracy;
        player->evasion = player->evasion + armlet2->evade;
        player->armor = player->armor + armlet2->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)armlet2->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)armlet2->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)armlet2->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)armlet2->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)armlet2->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)armlet2->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)armlet2->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)armlet2->charisma;
    }
    else
    {
        player->armlet2_item_id = 0;
        player->armlet2_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->bracer1_item_id) == ItemType_Bracer)
    {
        ItemValue *bracer1 =
            ItemValues::GetByIndex(GUI->item_values, player->bracer1_item_id - 1);
        if (bracer1->element < 7)
        {
            player->element_resistances[bracer1->element] =
                player->element_resistances[bracer1->element] + bracer1->element_damage;
        }
        player->bracer1_graphic_id = bracer1->spec1;
        player->weight_current = player->weight_current + (int)bracer1->weight;
        player->min_damage = player->min_damage + bracer1->min_damage;
        player->min_damage = player->min_damage + bracer1->min_damage;
        player->max_damage = player->max_damage + bracer1->max_damage;
        player->accuracy = player->accuracy + bracer1->accuracy;
        player->evasion = player->evasion + bracer1->evade;
        player->armor = player->armor + bracer1->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)bracer1->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)bracer1->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)bracer1->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)bracer1->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)bracer1->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)bracer1->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)bracer1->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)bracer1->charisma;
    }
    else
    {
        player->bracer1_item_id = 0;
        player->bracer1_graphic_id = 0;
    }
    if (ItemValues::GetType(GUI->item_values, player->bracer2_item_id) == ItemType_Bracer)
    {
        ItemValue *bracer2 =
            ItemValues::GetByIndex(GUI->item_values, player->bracer2_item_id - 1);
        if (bracer2->element < 7)
        {
            player->element_resistances[bracer2->element] =
                player->element_resistances[bracer2->element] + bracer2->element_damage;
        }
        player->bracer2_graphic_id = bracer2->spec1;
        player->weight_current = player->weight_current + (int)bracer2->weight;
        player->min_damage = player->min_damage + bracer2->min_damage;
        player->max_damage = player->max_damage + bracer2->max_damage;
        player->accuracy = player->accuracy + bracer2->accuracy;
        player->evasion = player->evasion + bracer2->evade;
        player->armor = player->armor + bracer2->armor;
        player->equip_bonus_hp = player->equip_bonus_hp + (int)bracer2->hp;
        player->equip_bonus_tp = player->equip_bonus_tp + (int)bracer2->tp;
        player->equip_strength_bonus =
            player->equip_strength_bonus + (int)bracer2->strength;
        player->equip_wisdom_bonus = player->equip_wisdom_bonus + (int)bracer2->wisdom;
        player->equip_intelligence_bonus =
            player->equip_intelligence_bonus + (int)bracer2->intelligence;
        player->equip_agility_bonus = player->equip_agility_bonus + (int)bracer2->agility;
        player->equip_constitution_bonus =
            player->equip_constitution_bonus + (int)bracer2->constitution;
        player->equip_charisma_bonus =
            player->equip_charisma_bonus + (int)bracer2->charisma;
    }
    else
    {
        player->bracer2_item_id = 0;
        player->bracer2_graphic_id = 0;
    }
    return;
}

void Server_SyncMapHazardFlags(Packets *server, int map_id)
{
    for (Player **iter = server->players->players.begin();
         iter != server->players->players.end();
         iter++)
    {
        if ((*iter)->map_id == map_id)
        {
            (*iter)->map_has_quakes =
                Mapcontrol_GetByIndex(server->map_control, map_id - 1)->has_quakes;
            (*iter)->map_has_hp_drain =
                Mapcontrol_GetByIndex(server->map_control, map_id - 1)->has_hp_drain;
            (*iter)->map_has_tp_drain =
                Mapcontrol_GetByIndex(server->map_control, map_id - 1)->has_tp_drain;
            (*iter)->map_has_spikes =
                Mapcontrol_GetByIndex(server->map_control, map_id - 1)->has_spikes;
        }
    }
}

int Party_ShareExp(Packets *server, Player *player, int exp)
{
    if (!player->in_party)
        return exp;
    if (player->CountPartyMembers() < 2)
        return exp;
    int members = 0;
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        Player *member = Players::Players_GetById(server->players, player->party_ids[i]);
        if (member != NULL && member->map_id == player->map_id)
            members++;
    }
    if (members < 2)
        return exp;
    if (exp > 3 && members > 2)
        exp = exp + exp / members;
    exp = exp / members;
    if (exp < 1)
        exp = 1;
    String pkt = "";
    bool leveled = false;
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        Player *member = Players::Players_GetById(server->players, player->party_ids[i]);
        if (member == NULL || member->player_id == player->player_id ||
            member->map_id != player->map_id)
            continue;
        if (Settings::GetMaxKills(server->settings) != 0)
        {
            if (KillCounters::IncrementAndGet(server->kill_counters, member->name) >
                Settings::GetMaxKills(server->settings))
                exp = 0;
        }
        member->experience = member->experience + exp;
        int levelup = Players::Player_TryLevelUp(server->players, member);
        if (levelup > 0)
        {
            String stats = EO_EncodeNumber(server, member->stat_points, 2);
            stats.Insert(EO_EncodeNumber(server, member->skill_points, 2),
                         stats.Length() + 1);
            stats.Insert(EO_EncodeNumber(server, member->max_hp, 2), stats.Length() + 1);
            stats.Insert(EO_EncodeNumber(server, member->max_tp, 2), stats.Length() + 1);
            stats.Insert(EO_EncodeNumber(server, member->max_sp, 2), stats.Length() + 1);
            Client_SendEncoded(
                server, member, PacketAction_TargetGroup, PacketFamily_Recover, stats);
            leveled = true;
        }
        pkt.Insert(EO_EncodeNumber(server, member->player_id, 2), pkt.Length() + 1);
        pkt.Insert(EO_EncodeNumber(server, exp, 4), pkt.Length() + 1);
        pkt.Insert(EO_EncodeNumber(server, levelup, 1), pkt.Length() + 1);
    }
    if (leveled)
        Server_BroadcastToMap(
            server, player->map_id, PacketAction_TargetGroup, PacketFamily_Party, pkt);
    else
        Server_BroadcastToPartyOnMap(
            server, player, PacketAction_TargetGroup, PacketFamily_Party, pkt);
    return exp;
}

bool Face_Execute(Packets *server, Player *player, int action, String *data)
{
    *(TTimeStamp *)&player->walk_tick = DateTimeToTimeStamp(Now());
    if (action == PacketAction_Player)
    {
        if (!player->logged_in)
            return false;
        if (player->on_chair || player->sitting)
            return true;
        if (data->Length() < 1)
            return false;
        if ((unsigned)EO_DecodeNumber(server, (*data)[1]) > Direction_Right)
            return false;
        player->direction = EO_DecodeNumber(server, (*data)[1]);
        String out = EO_EncodeNumber(server, player->player_id, 2);
        out = out + (*data)[1];
        Server_BroadcastNearby(
            server, player, PacketAction_Player, PacketFamily_Face, out);
        return true;
    }
    return false;
}

bool Chair_Execute(Packets *server, Player *player, int action, String *data)
{
    *(TTimeStamp *)&player->walk_tick = DateTimeToTimeStamp(Now());
    if (action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        if (data->Length() < 1)
            return false;
        int sit_action = EO_DecodeNumber(server, (*data)[1]);
        if (sit_action == SitAction_Sit)
        {
            if (data->Length() < 3)
                return false;
            int x = EO_DecodeNumber(server, (*data)[2]);
            int y = EO_DecodeNumber(server, (*data)[3]);
            if (player->on_chair)
                return true;
            if (Players::Players_IsPlayerAt(server->players, player->map_id, x, y))
                return true;
            if (player->map_id < 1 ||
                (int)server->map_control->maps.size() < player->map_id)
                return true;
            int spec = MapContainer::Mapcontrol_GetTileSpec(
                server->map_control, player->map_id, x, y);
            if (spec >= 0 && spec <= 6)
            {
                MapObject tile = Mapcontrol_GetTileSpecObject(
                    server->map_control, player->map_id, x, y);
                String out = EO_EncodeNumber(server, player->player_id, 2);
                out.Insert(EO_EncodeNumber(server, x, 1), out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, y, 1), out.Length() + 1);
                if ((spec == 0 || spec == 4 || spec == 6) &&
                    (unsigned short)tile.x == player->x &&
                    (unsigned short)tile.y == player->y - 1)
                {
                    player->on_chair = true;
                    player->sitting = false;
                    player->x = x;
                    player->y = y;
                    player->direction = Direction_Down;
                    out.Insert(EO_EncodeNumber(server, Direction_Down, 1),
                               out.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Reply, PacketFamily_Chair, out);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Player, PacketFamily_Chair, out);
                    return true;
                }
                if ((spec == 1 || spec == 5 || spec == 6) &&
                    (unsigned short)tile.x == (unsigned)(player->x + 1) &&
                    (unsigned short)tile.y == player->y)
                {
                    player->on_chair = true;
                    player->sitting = false;
                    player->x = x;
                    player->y = y;
                    player->direction = Direction_Left;
                    out.Insert(EO_EncodeNumber(server, Direction_Left, 1),
                               out.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Reply, PacketFamily_Chair, out);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Player, PacketFamily_Chair, out);
                    return true;
                }
                if ((spec == 2 || spec == 4 || spec == 6) &&
                    (unsigned short)tile.x == (unsigned)(player->x - 1) &&
                    (unsigned short)tile.y == player->y)
                {
                    player->on_chair = true;
                    player->sitting = false;
                    player->x = x;
                    player->y = y;
                    player->direction = Direction_Right;
                    out.Insert(EO_EncodeNumber(server, Direction_Right, 1),
                               out.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Reply, PacketFamily_Chair, out);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Player, PacketFamily_Chair, out);
                    return true;
                }
                if ((spec == 3 || spec == 5 || spec == 6) &&
                    (unsigned short)tile.x == player->x &&
                    (unsigned short)tile.y == (unsigned)(player->y + 1))
                {
                    player->on_chair = true;
                    player->sitting = false;
                    player->x = x;
                    player->y = y;
                    player->direction = Direction_Up;
                    out.Insert(EO_EncodeNumber(server, Direction_Up, 1),
                               out.Length() + 1);
                    Client_SendEncoded(
                        server, player, PacketAction_Reply, PacketFamily_Chair, out);
                    Server_BroadcastNearby(
                        server, player, PacketAction_Player, PacketFamily_Chair, out);
                    return true;
                }
            }
        }
        else
        {
            if (!player->on_chair)
                return true;
            if (player->direction == Direction_Down)
                player->y = player->y + 1;
            if (player->direction == Direction_Left)
                player->x = player->x + -1;
            if (player->direction == Direction_Up)
                player->y = player->y + -1;
            if (player->direction == Direction_Right)
                player->x = player->x + 1;
            player->on_chair = false;
            player->sitting = false;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            out.Insert(EO_EncodeNumber(server, player->x, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->y, 1), out.Length() + 1);
            Client_SendEncoded(
                server, player, PacketAction_Close, PacketFamily_Chair, out);
            Server_BroadcastNearby(
                server, player, PacketAction_Remove, PacketFamily_Chair, out);
            return true;
        }
    }
    return false;
}

bool Attack_Execute(Packets *server, Player *caster, int action, String *data)
{
    *(TTimeStamp *)&caster->walk_tick = DateTimeToTimeStamp(Now());
    if (caster->map_id < 1)
        return 1;
    if (caster->weight_max + 2 < caster->weight_current)
        return 1;
    if (action == 10)
    {
        if (!caster->logged_in)
            return 0;
        if (caster->sitting || caster->on_chair)
            return 1;
        if (data->Length() < 4)
            return 0;
        int attack_tick = EO_DecodeNumber(server, data->SubString(2, 3));
        int elapsed = attack_tick - caster->last_client_walk_tick;
        if (elapsed < 0 && caster->last_client_walk_tick > WALK_DELAY_SANITY_MS)
            elapsed = 0x2c;
        caster->last_client_walk_tick = attack_tick;
        if (elapsed < 0x2c)
            return 0;
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int window = stamp.Time / 10 + 0x64;
        int clock_drift;
        if (attack_tick > window)
        {
            clock_drift = attack_tick - window;
            if (caster->sync_base_ahead < 0)
                caster->sync_base_ahead = clock_drift;
            if (Math_Abs(clock_drift - caster->sync_base_ahead) > 0x320)
                return 1;
        }
        else
        {
            clock_drift = window - attack_tick;
            if (caster->sync_base_behind < 0)
                caster->sync_base_behind = clock_drift;
            if (Math_Abs(clock_drift - caster->sync_base_behind) > 0x320)
                return 1;
        }
        caster->direction = EO_DecodeNumber(server, (*data)[1]);
        int offset_x = offset_x = caster->x;
        int offset_y = offset_y = caster->y;
        int reach = 1;
        if (caster->direction > Direction_Right)
            return 1;
        if (Combat_IsRangedWeapon(server->weapon_map, caster->weapon_graphic_id))
        {
            reach = 6;
            if (caster->weapon_graphic_id == 0x31 || caster->weapon_graphic_id == 0x32)
                reach = 0;
        }
        if (ItemValues::GetSubtype(GUI->item_values, caster->weapon_item_id) ==
            ItemSubtype_Ranged)
        {
            if (ItemValues::GetSubtype(GUI->item_values, caster->shield_item_id) !=
                ItemSubtype_Arrows)
                return 1;
        }
        if ((unsigned char)Mapcontrol_GetByIndex(server->map_control, caster->map_id - 1)
                ->map_type == MapType_Pk)
        {
            for (int i = 0; i < reach; i++)
            {
                if (caster->direction == Direction_Down)
                    offset_y++;
                if (caster->direction == Direction_Left)
                    offset_x--;
                if (caster->direction == Direction_Up)
                    offset_y--;
                if (caster->direction == Direction_Right)
                    offset_x++;
                Player **iter;
                for (iter = server->players->players.begin();
                     iter != server->players->players.end();
                     iter++)
                {
                    if ((*iter)->map_id != caster->map_id)
                        continue;
                    if ((*iter)->x != offset_x)
                        continue;
                    if ((*iter)->y != offset_y)
                        continue;
                    if (Player::IsPartyMember(caster, (*iter)->player_id))
                        continue;
                    int damage = 0;
                    int hit_rate = Game::Combat_CalcHitRate(
                        GUI->game_control, caster->accuracy, (*iter)->evasion, 0.9);
                    if (RandRange(100) < hit_rate)
                    {
                        hit_rate = Game::Combat_CalcArmorPen(
                            GUI->game_control,
                            (caster->min_damage + caster->max_damage) / 2,
                            (*iter)->armor,
                            0.8);
                        double scaled = (double)(int)caster->min_damage;
                        if (scaled < 1.0)
                            scaled = 1.0;
                        scaled *= 0.01L;
                        scaled *= (double)hit_rate;
                        scaled += (double)RandRange(caster->max_damage -
                                                    caster->min_damage + 2);
                        damage = (int)scaled;
                        if (damage < 1)
                            damage = 1;
                    }
                    if (caster->weapon_item_id > 0)
                    {
                        ItemElement element = ItemValues::GetElement(
                            GUI->item_values, caster->weapon_item_id);
                        element.element_damage = 0;
                        if (element.element == 1)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[1],
                                               (*iter)->element_resistances[2]));
                        if (element.element == 2)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[2],
                                               (*iter)->element_resistances[1]));
                        if (element.element == 3)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[3],
                                               (*iter)->element_resistances[6]));
                        if (element.element == 4)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[4],
                                               (*iter)->element_resistances[3]));
                        if (element.element == 5)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[5],
                                               (*iter)->element_resistances[4]));
                        if (element.element == 6)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[6],
                                               (*iter)->element_resistances[5]));
                    }
                    if ((*iter)->direction == caster->direction)
                        damage += damage / 2;
                    (*iter)->hp -= damage;
                    if ((*iter)->hp > (*iter)->max_hp)
                        (*iter)->hp = (*iter)->max_hp;
                    if ((*iter)->hp < 1)
                        (*iter)->hp = 0;
                    if (damage > 0 && (*iter)->in_party)
                    {
                        String party_pkt = EO_EncodeNumber(server, (*iter)->player_id, 2);
                        party_pkt.Insert(
                            EO_EncodeNumber(server, Player::HpPercent(*iter), 1),
                            party_pkt.Length() + 1);
                        Server_BroadcastToParty(server,
                                                (*iter),
                                                PacketAction_Agree,
                                                PacketFamily_Party,
                                                party_pkt);
                    }
                    String pkt = EO_EncodeNumber(server, caster->player_id, 2);
                    pkt.Insert(EO_EncodeNumber(server, (*iter)->player_id, 2),
                               pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, damage, 3), pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, caster->direction, 1),
                               pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, Player::HpPercent(*iter), 1),
                               pkt.Length() + 1);
                    if ((*iter)->hp < 1)
                        pkt.Insert(EO_EncodeNumber(server, 1, 1), pkt.Length() + 1);
                    else
                        pkt.Insert(EO_EncodeNumber(server, 0, 1), pkt.Length() + 1);
                    Server_BroadcastNearby(
                        server, (*iter), PacketAction_Reply, PacketFamily_Avatar, pkt);
                    Client_SendEncoded(
                        server, (*iter), PacketAction_Reply, PacketFamily_Avatar, pkt);
                    pkt = EO_EncodeNumber(server, (*iter)->hp, 2);
                    pkt.Insert(EO_EncodeNumber(server, (*iter)->tp, 2), pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, 0, 2), pkt.Length() + 1);
                    Client_SendEncoded(
                        server, (*iter), PacketAction_Player, PacketFamily_Recover, pkt);
                    if ((*iter)->hp < 1)
                    {
                        Player_Respawn(server, (*iter));
                        Player_FireQuestTriggers(server, caster, 9, 1);
                    }
                    return 1;
                }
            }
            offset_x = offset_x = caster->x;
            offset_y = offset_y = caster->y;
        }
        for (int tile_step = 0; tile_step < reach; tile_step++)
        {
            if (caster->direction == Direction_Down)
                offset_y++;
            if (caster->direction == Direction_Left)
                offset_x--;
            if (caster->direction == Direction_Up)
                offset_y--;
            if (caster->direction == Direction_Right)
                offset_x++;
            if (offset_y < 0 || offset_x < 0)
            {
                String pkt = EO_EncodeNumber(server, caster->player_id, 2);
                pkt = pkt + (*data)[1];
                Server_BroadcastNearby(
                    server, caster, PacketAction_Player, PacketFamily_Attack, pkt);
                return 1;
            }
            if (!MapContainer::Mapcontrol_IsTileClear(
                    server->map_control, caster->map_id, offset_x, offset_y))
            {
                String pkt = EO_EncodeNumber(server, caster->player_id, 2);
                pkt = pkt + (*data)[1];
                Server_BroadcastNearby(
                    server, caster, PacketAction_Player, PacketFamily_Attack, pkt);
                return 1;
            }
            Npc **npc_iter;
            for (npc_iter = (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                          caster->map_id - 1)
                                ->npc_list.begin();
                 npc_iter !=
                 (Npc **)Mapcontrol_GetByIndex(server->map_control, caster->map_id - 1)
                     ->npc_list.end();
                 npc_iter++)
            {
                if ((*npc_iter)->x != offset_x)
                    continue;
                if ((*npc_iter)->y != offset_y)
                    continue;
                if (!(*npc_iter)->alive)
                    continue;
                NpcTypeInfo type_info =
                    NpcValues::GetType(GUI->npc_values, (*npc_iter)->id);
                if (type_info.type <= 0 || type_info.type >= 6)
                    continue;
                if ((*npc_iter)->chase_target_id != caster->player_id &&
                    (*npc_iter)->chase_target_id > 0 &&
                    !Player::IsPartyMember(caster, (*npc_iter)->chase_target_id))
                {
                    String reply = EO_EncodeNumber(server, caster->player_id, 2);
                    reply.Insert((*data)[1], reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->index, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, 0, 3), reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->nHp_pct, 2),
                                 reply.Length() + 1);
                    Server_BroadcastNearby(
                        server, caster, PacketAction_Reply, PacketFamily_Npc, reply);
                    reply.Insert(EO_EncodeNumber(server, 2, 1), reply.Length() + 1);
                    Client_SendEncoded(
                        server, caster, PacketAction_Reply, PacketFamily_Npc, reply);
                    return 1;
                }
                int damage = 0;
                int hit_rate = Game::Combat_CalcHitRate(
                    GUI->game_control, caster->accuracy, (*npc_iter)->evade, 0.9);
                if (RandRange(100) < hit_rate)
                {
                    hit_rate = Game::Combat_CalcArmorPen(
                        GUI->game_control,
                        (caster->min_damage + caster->max_damage) / 2,
                        (*npc_iter)->armor,
                        0.8);
                    double scaled = (double)(int)caster->min_damage;
                    if (scaled < 1.0)
                        scaled = 1.0;
                    scaled *= 0.01L;
                    scaled *= (double)hit_rate;
                    scaled +=
                        (double)RandRange(caster->max_damage - caster->min_damage + 2);
                    damage = (int)scaled;
                    if (damage < 1)
                        damage = 1;
                }
                if (caster->weapon_item_id > 0)
                {
                    ItemElement element =
                        ItemValues::GetElement(GUI->item_values, caster->weapon_item_id);
                    element.element_damage = 0;
                    if (element.element == 1)
                        damage =
                            (int)((double)damage *
                                  Game::Combat_CalcElementMult(
                                      GUI->game_control,
                                      *(MapCoord *)&element,
                                      caster->element_resistances[1],
                                      (*npc_iter)->element_weakness_damage_table[1]));
                    if (element.element == 2)
                        damage =
                            (int)((double)damage *
                                  Game::Combat_CalcElementMult(
                                      GUI->game_control,
                                      *(MapCoord *)&element,
                                      caster->element_resistances[2],
                                      (*npc_iter)->element_weakness_damage_table[0]));
                    if (element.element == 3)
                        damage =
                            (int)((double)damage *
                                  Game::Combat_CalcElementMult(
                                      GUI->game_control,
                                      *(MapCoord *)&element,
                                      caster->element_resistances[3],
                                      (*npc_iter)->element_weakness_damage_table[5]));
                    if (element.element == 4)
                        damage =
                            (int)((double)damage *
                                  Game::Combat_CalcElementMult(
                                      GUI->game_control,
                                      *(MapCoord *)&element,
                                      caster->element_resistances[4],
                                      (*npc_iter)->element_weakness_damage_table[2]));
                    if (element.element == 5)
                        damage =
                            (int)((double)damage *
                                  Game::Combat_CalcElementMult(
                                      GUI->game_control,
                                      *(MapCoord *)&element,
                                      caster->element_resistances[5],
                                      (*npc_iter)->element_weakness_damage_table[3]));
                    if (element.element == 6)
                        damage =
                            (int)((double)damage *
                                  Game::Combat_CalcElementMult(
                                      GUI->game_control,
                                      *(MapCoord *)&element,
                                      caster->element_resistances[6],
                                      (*npc_iter)->element_weakness_damage_table[4]));
                }
                if ((unsigned short)(*npc_iter)->nAttack_dir ==
                        (unsigned int)caster->direction ||
                    (*npc_iter)->hp == (*npc_iter)->max_hp)
                    damage += damage / 2;
                if ((unsigned short)(*npc_iter)->boss > 0)
                    MapContainer::Mapcontrol_AggroChildNpcs(server->map_control,
                                                            caster->map_id);
                (*npc_iter)->aggressive = true;
                (*npc_iter)->nLeash_timer = (short)(RandRange(0x32) + 100);
                if (MapContainer::Mapcontrol_CountNpcsChasingPlayer(
                        server->map_control, caster->map_id, caster->player_id) < 2 &&
                    type_info.behavior_id == 0)
                    (*npc_iter)->chase_target_id = caster->player_id;
                (*npc_iter)->hp -= damage;
                (*npc_iter)->nHp_pct =
                    (short)((*npc_iter)->hp * 100 /
                            NpcValues::GetMaxHp(GUI->npc_values, (*npc_iter)->id));
                if ((*npc_iter)->hp < 1)
                {
                    int exp = NpcValues::GetExp(GUI->npc_values, (*npc_iter)->id);
                    int drop_result = 0;
                    exp = Party_ShareExp(server, caster, exp);
                    if ((*npc_iter)->wDrop_item_id > 0 && (*npc_iter)->wDrop_amount > 0)
                        drop_result = MapContainer::Mapcontrol_AddGroundItem(
                            server->map_control,
                            caster->map_id,
                            (*npc_iter)->wDrop_item_id,
                            (*npc_iter)->x,
                            (*npc_iter)->y,
                            (*npc_iter)->wDrop_amount,
                            caster->account_ident,
                            0x3d);
                    TDateTime now = Now();
                    *(TTimeStamp *)&(*npc_iter)->nDeath_ms = DateTimeToTimeStamp(now);
                    (*npc_iter)->chase_target_id = -1;
                    (*npc_iter)->alive = false;
                    if ((unsigned short)(*npc_iter)->boss > 0 &&
                        MapContainer::Mapcontrol_KillChildNpcs(server->map_control,
                                                               caster->map_id))
                        Server_BroadcastToMap(
                            server,
                            caster->map_id,
                            PacketAction_Junk,
                            PacketFamily_Npc,
                            EO_EncodeNumber(server,
                                            (unsigned short)Mapcontrol_GetByIndex(
                                                server->map_control, caster->map_id - 1)
                                                ->child_npc_id,
                                            2));
                    String reply = EO_EncodeNumber(server, caster->player_id, 2);
                    reply.Insert(EO_EncodeNumber(server, caster->direction, 1),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->index, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, drop_result, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->wDrop_item_id, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->x, 1),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->y, 1),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->wDrop_amount, 4),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, damage, 3), reply.Length() + 1);
                    if (Settings::GetMaxKills(server->settings) != 0)
                    {
                        if (KillCounters::IncrementAndGet(server->kill_counters,
                                                          caster->name) >
                            Settings::GetMaxKills(server->settings))
                            exp = 0;
                    }
                    caster->experience += exp;
                    if (Players::Player_TryLevelUp(server->players, caster) > 0)
                    {
                        Server_BroadcastNearby(
                            server, caster, PacketAction_Accept, PacketFamily_Npc, reply);
                        reply.Insert(EO_EncodeNumber(server, caster->experience, 4),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->level, 1),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->stat_points, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->skill_points, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->max_hp, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->max_tp, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->max_sp, 2),
                                     reply.Length() + 1);
                        Client_SendEncoded(
                            server, caster, PacketAction_Accept, PacketFamily_Npc, reply);
                        return 1;
                    }
                    Server_BroadcastNearby(
                        server, caster, PacketAction_Spec, PacketFamily_Npc, reply);
                    reply.Insert(EO_EncodeNumber(server, caster->experience, 4),
                                 reply.Length() + 1);
                    if (!caster->cheater_flag)
                        Client_SendEncoded(
                            server, caster, PacketAction_Spec, PacketFamily_Npc, reply);
                    else
                    {
                        if (RandRange(6) > 2)
                            caster->experience -= exp;
                    }
                    Player_FireQuestTriggers(server, caster, 8, (*npc_iter)->id);
                    return 1;
                }
                String reply = EO_EncodeNumber(server, caster->player_id, 2);
                reply.Insert((*data)[1], reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, (*npc_iter)->index, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, damage, 3), reply.Length() + 1);
                if (!caster->cheater_flag)
                    reply.Insert(EO_EncodeNumber(server, (*npc_iter)->nHp_pct, 2),
                                 reply.Length() + 1);
                else if ((*npc_iter)->nHp_pct < 1)
                    reply.Insert(EO_EncodeNumber(server, RandRange(2) + 1, 2),
                                 reply.Length() + 1);
                Server_BroadcastNearby(
                    server, caster, PacketAction_Reply, PacketFamily_Npc, reply);
                reply.Insert(EO_EncodeNumber(server, 1, 1), reply.Length() + 1);
                Client_SendEncoded(
                    server, caster, PacketAction_Reply, PacketFamily_Npc, reply);
                return 1;
            }
            if (caster->arena_queued)
            {
                if (!(char)Mapcontrol_GetByIndex(server->map_control, caster->map_id - 1)
                         ->arena_enabled)
                    caster->arena_queued = false;
                else if (tile_step == 0)
                {
                    Player *target = Players::Players_GetByMapTile(
                        server->players, caster->map_id, offset_x, offset_y);
                    if (target != 0)
                    {
                        caster->arena_kills++;
                        target->arena_queued = false;
                        MapCoord coords;
                        coords.x =
                            Mapcontrol_GetByIndex(server->map_control, caster->map_id - 1)
                                ->relog_x;
                        coords.y =
                            Mapcontrol_GetByIndex(server->map_control, caster->map_id - 1)
                                ->relog_y;
                        Player_Warp(server,
                                    target,
                                    target->map_id,
                                    coords,
                                    WarpEffect_None,
                                    true);
                        if (Players::Players_CountArenaPlayers(server->players,
                                                               caster->map_id) < 2)
                        {
                            String arena_win_pkt = caster->name;
                            arena_win_pkt.Insert(" ", arena_win_pkt.Length() + 1);
                            if (caster->title.Length() > 0)
                            {
                                arena_win_pkt.Insert("(", arena_win_pkt.Length() + 1);
                                arena_win_pkt.Insert(caster->title,
                                                     arena_win_pkt.Length() + 1);
                                arena_win_pkt.Insert(") ", arena_win_pkt.Length() + 1);
                            }
                            arena_win_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                                 arena_win_pkt.Length() + 1);
                            arena_win_pkt.Insert(
                                EO_EncodeNumber(server, caster->arena_kills, 1),
                                arena_win_pkt.Length() + 1);
                            arena_win_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                                 arena_win_pkt.Length() + 1);
                            arena_win_pkt.Insert(caster->name,
                                                 arena_win_pkt.Length() + 1);
                            arena_win_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                                 arena_win_pkt.Length() + 1);
                            arena_win_pkt.Insert(target->name,
                                                 arena_win_pkt.Length() + 1);
                            Server_BroadcastToMap(server,
                                                  caster->map_id,
                                                  PacketAction_Accept,
                                                  PacketFamily_Arena,
                                                  arena_win_pkt);
                            Client_SendEncoded(server,
                                               target,
                                               PacketAction_Accept,
                                               PacketFamily_Arena,
                                               arena_win_pkt);
                            if (Mapcontrol_GetByIndex(server->map_control,
                                                      caster->map_id - 1)
                                    ->arena_block > 2)
                            {
                                caster->arena_queued = false;
                                Player_Warp(server,
                                            caster,
                                            caster->map_id,
                                            coords,
                                            WarpEffect_None,
                                            true);
                            }
                            return 1;
                        }
                        String arena_elim_pkt =
                            EO_EncodeNumber(server, caster->player_id, 2);
                        arena_elim_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                              arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(
                            EO_EncodeNumber(server, caster->direction, 1),
                            arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                              arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(
                            EO_EncodeNumber(server, caster->arena_kills, 1),
                            arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                              arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(caster->name, arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(EO_GetBreakByte(server, 0xff),
                                              arena_elim_pkt.Length() + 1);
                        arena_elim_pkt.Insert(target->name, arena_elim_pkt.Length() + 1);
                        Server_BroadcastToMap(server,
                                              caster->map_id,
                                              PacketAction_Spec,
                                              PacketFamily_Arena,
                                              arena_elim_pkt);
                        Client_SendEncoded(server,
                                           target,
                                           PacketAction_Spec,
                                           PacketFamily_Arena,
                                           arena_elim_pkt);
                        return 1;
                    }
                }
            }
        }
        String final_pkt = EO_EncodeNumber(server, caster->player_id, 2);
        final_pkt = final_pkt + (*data)[1];
        Server_BroadcastNearby(
            server, caster, PacketAction_Player, PacketFamily_Attack, final_pkt);
        return 1;
    }
    return 0;
}

int Math_Abs(int value)
{
    return __abs__(value);
}

bool Spell_Execute(Packets *server, Player *caster, int action, String *data)
{
    *(TTimeStamp *)&caster->walk_tick = DateTimeToTimeStamp(Now());
    if (caster->map_id < 1)
        return 1;
    if (caster->weight_max + 2 < caster->weight_current)
        return 1;
    if (action == 1)
    {
        if (data->Length() < 5)
            return 0;
        caster->queued_spell_id = EO_DecodeNumber(server, data->SubString(1, 2));
        if (!Players::Player_HasSpellId(server->players, caster, caster->queued_spell_id))
            return 0;
        int cast_time =
            SkillValues::GetCastTime(GUI->skill_values, caster->queued_spell_id) * 30;
        caster->expected_cast_timestamp =
            EO_DecodeNumber(server, data->SubString(3, 3)) + cast_time - 1;
        String pkt = EO_EncodeNumber(server, caster->player_id, 2);
        pkt.Insert(EO_EncodeNumber(server, caster->queued_spell_id, 2), pkt.Length() + 1);
        Server_BroadcastNearby(
            server, caster, PacketAction_Request, PacketFamily_Spell, pkt);
        return 1;
    }
    if (action == 0x1f)
    {
        if (!caster->logged_in)
            return 0;
        if (caster->sitting || caster->on_chair)
            return 1;
        if (data->Length() < 11)
            return 0;
        int client_tick = EO_DecodeNumber(server, data->SubString(2, 3));
        int elapsed = client_tick - caster->last_client_walk_tick;
        if (elapsed < 0 && caster->last_client_walk_tick > WALK_DELAY_SANITY_MS)
            elapsed = 0x2c;
        caster->last_client_walk_tick = client_tick;
        if (elapsed < 0x2c)
            return 0;
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int sync_tick = stamp.Time / 10 + 0x64;
        int clock_drift;
        if (client_tick > sync_tick)
        {
            clock_drift = client_tick - sync_tick;
            if (caster->sync_base_ahead < 0)
                caster->sync_base_ahead = clock_drift;
            if (Math_Abs(clock_drift - caster->sync_base_ahead) > 0x320)
                return 1;
        }
        else
        {
            clock_drift = sync_tick - client_tick;
            if (caster->sync_base_behind < 0)
                caster->sync_base_behind = clock_drift;
            if (Math_Abs(clock_drift - caster->sync_base_behind) > 0x320)
                return 1;
        }
        int spell_target = EO_DecodeNumber(server, (*data)[1]);
        int spell_id = EO_DecodeNumber(server, data->SubString(5, 2));
        int target_id = EO_DecodeNumber(server, data->SubString(7, 2));
        int cast_tick = EO_DecodeNumber(server, data->SubString(9, 3));
        int target_type = SkillValues::GetTargetType(GUI->skill_values, spell_id);
        if (spell_id != caster->queued_spell_id)
            return 0;
        if (cast_tick < caster->expected_cast_timestamp &&
            (cast_tick > 1000 || caster->expected_cast_timestamp < 0x83ce30))
            return 0;
        if (target_type != 0)
            return 1;
        if (!Players::Player_HasSpellId(server->players, caster, spell_id))
            return 1;
        int skill_type = SkillValues::GetSkillType(GUI->skill_values, spell_id);
        int tp_cost = SkillValues::GetTpCost(GUI->skill_values, spell_id);
        if (caster->tp < tp_cost)
        {
            String reply = EO_EncodeNumber(server, spell_id, 2);
            reply.Insert(EO_EncodeNumber(server, caster->hp, 2), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, caster->tp, 2), reply.Length() + 1);
            Client_SendEncoded(
                server, caster, PacketAction_Reply, PacketFamily_Spell, reply);
            return 1;
        }
        caster->tp -= tp_cost;
        if (spell_target == SpellTargetType_Player)
        {
            Player *target = server->players->by_id[target_id];
            if (target == 0)
                return 1;
            if (target->map_id != caster->map_id)
                return 1;
            if (!Server_InViewRange(server, caster->x, caster->y, target->x, target->y))
            {
                Client_SendEncoded(server,
                                   caster,
                                   PacketAction_Reply,
                                   PacketFamily_Refresh,
                                   Refresh_BuildReply(server, caster));
                return 1;
            }
            if (skill_type == 0)
            {
                int hp_heal = SkillValues::GetHpHeal(GUI->skill_values, spell_id);
                target->hp += hp_heal;
                if (target->hp > target->max_hp)
                    target->hp = target->max_hp;
                int hp_percent = target->hp * 100 / target->max_hp;
                String reply = EO_EncodeNumber(server, target->player_id, 2);
                reply.Insert(EO_EncodeNumber(server, caster->player_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, spell_target, 1),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, spell_id, 2), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, hp_heal, 4), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, hp_percent, 1), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, target->hp, 2), reply.Length() + 1);
                Server_BroadcastToMap(server,
                                      target->map_id,
                                      PacketAction_TargetOther,
                                      PacketFamily_Spell,
                                      reply);
                reply = EO_EncodeNumber(server, spell_id, 2);
                reply.Insert(EO_EncodeNumber(server, caster->hp, 2), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, caster->tp, 2), reply.Length() + 1);
                Client_SendEncoded(
                    server, caster, PacketAction_Reply, PacketFamily_Spell, reply);
                if (target->in_party)
                {
                    String pkt = EO_EncodeNumber(server, target->player_id, 2);
                    pkt.Insert(EO_EncodeNumber(server, Player::HpPercent(target), 1),
                               pkt.Length() + 1);
                    Server_BroadcastToParty(
                        server, target, PacketAction_Agree, PacketFamily_Party, pkt);
                }
                return 1;
            }
            if (skill_type == 1)
            {
                if ((unsigned char)Mapcontrol_GetByIndex(server->map_control,
                                                         caster->map_id - 1)
                        ->map_type == 3)
                {
                    if (target->player_id == caster->player_id)
                        return 1;
                    if (target->map_id != caster->map_id)
                        return 1;
                    if (Player::IsPartyMember(caster, target->player_id))
                        return 1;
                    int damage = 0;
                    int hit_rate = Game::Combat_CalcArmorPen(
                        GUI->game_control, caster->accuracy, target->evasion, 0.9);
                    if (RandRange(100) < hit_rate)
                    {
                        SkillDamage dmg =
                            SkillValues::GetDamage(GUI->skill_values, spell_id);
                        int min_total = caster->min_damage + dmg.min_damage;
                        int max_total = caster->max_damage + dmg.max_damage;
                        hit_rate = Game::Combat_CalcArmorPen(GUI->game_control,
                                                             (min_total + max_total) / 2,
                                                             target->armor,
                                                             0.8);
                        double scaled = (double)min_total;
                        if (scaled < 1.0)
                            scaled = 1.0;
                        scaled *= 0.01L;
                        scaled *= (double)hit_rate;
                        scaled += (double)RandRange(max_total - min_total + 2);
                        damage = (int)scaled;
                        if (damage < 1)
                            damage = 1;
                    }
                    if (spell_id > 0)
                    {
                        SkillElement element =
                            SkillValues::GetElement(GUI->skill_values, spell_id);
                        if (element.element == 1)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[1],
                                               target->element_resistances[2]));
                        if (element.element == 2)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[2],
                                               target->element_resistances[1]));
                        if (element.element == 3)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[3],
                                               target->element_resistances[6]));
                        if (element.element == 4)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[4],
                                               target->element_resistances[3]));
                        if (element.element == 5)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[5],
                                               target->element_resistances[4]));
                        if (element.element == 6)
                            damage = (int)((double)damage *
                                           Game::Combat_CalcElementMult(
                                               GUI->game_control,
                                               *(MapCoord *)&element,
                                               caster->element_resistances[6],
                                               target->element_resistances[5]));
                    }
                    target->hp -= damage;
                    if (target->hp > target->max_hp)
                        target->hp = target->max_hp;
                    if (target->hp < 1)
                        target->hp = 0;
                    if (damage > 0 && target->in_party)
                    {
                        String hp_pkt = EO_EncodeNumber(server, target->player_id, 2);
                        hp_pkt.Insert(
                            EO_EncodeNumber(server, Player::HpPercent(target), 1),
                            hp_pkt.Length() + 1);
                        Server_BroadcastToParty(server,
                                                target,
                                                PacketAction_Agree,
                                                PacketFamily_Party,
                                                hp_pkt);
                    }
                    String pkt = EO_EncodeNumber(server, caster->player_id, 2);
                    pkt.Insert(EO_EncodeNumber(server, target->player_id, 2),
                               pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, damage, 3), pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, caster->direction, 1),
                               pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, Player::HpPercent(target), 1),
                               pkt.Length() + 1);
                    if (target->hp < 1)
                        pkt.Insert(EO_EncodeNumber(server, 1, 1), pkt.Length() + 1);
                    else
                        pkt.Insert(EO_EncodeNumber(server, 0, 1), pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, spell_id, 2), pkt.Length() + 1);
                    Server_BroadcastNearby(
                        server, target, PacketAction_Admin, PacketFamily_Avatar, pkt);
                    Client_SendEncoded(
                        server, target, PacketAction_Admin, PacketFamily_Avatar, pkt);
                    pkt = EO_EncodeNumber(server, target->hp, 2);
                    pkt.Insert(EO_EncodeNumber(server, target->tp, 2), pkt.Length() + 1);
                    pkt.Insert(EO_EncodeNumber(server, 0, 2), pkt.Length() + 1);
                    Client_SendEncoded(
                        server, target, PacketAction_Player, PacketFamily_Recover, pkt);
                    if (target->hp < 1)
                        Player_Respawn(server, target);
                }
                return 1;
            }
            return 1;
        }
        if (spell_target == SpellTargetType_Npc)
        {
            Npc **iter;
            for (iter = (Npc **)Mapcontrol_GetByIndex(server->map_control,
                                                      caster->map_id - 1)
                            ->npc_list.begin();
                 iter !=
                 (Npc **)Mapcontrol_GetByIndex(server->map_control, caster->map_id - 1)
                     ->npc_list.end();
                 iter++)
            {
                if ((*iter)->index != target_id)
                    continue;
                NpcTypeInfo type_info = NpcValues::GetType(GUI->npc_values, (*iter)->id);
                if (!(*iter)->alive)
                    return 1;
                if (!Server_InViewRange(
                        server, caster->x, caster->y, (*iter)->x, (*iter)->y))
                {
                    Client_SendEncoded(server,
                                       caster,
                                       PacketAction_Reply,
                                       PacketFamily_Refresh,
                                       Refresh_BuildReply(server, caster));
                    return 1;
                }
                MapCoord coords;
                coords.x = (*iter)->x;
                coords.y = (*iter)->y;
                if (skill_type == 1 && type_info.type > 0 && type_info.type < 6)
                {
                    if ((*iter)->chase_target_id != caster->player_id &&
                        (*iter)->chase_target_id > 0 &&
                        !Player::IsPartyMember(caster, (*iter)->chase_target_id))
                    {
                        String reply = EO_EncodeNumber(server, spell_id, 2);
                        reply.Insert(EO_EncodeNumber(server, caster->player_id, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->direction, 1),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->index, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, 0, 3), reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->nHp_pct, 2),
                                     reply.Length() + 1);
                        Server_BroadcastNearTile(server,
                                                 caster->player_id,
                                                 caster->map_id,
                                                 coords,
                                                 PacketAction_Reply,
                                                 PacketFamily_Cast,
                                                 reply);
                        reply.Insert(EO_EncodeNumber(server, caster->tp, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, 2, 1), reply.Length() + 1);
                        Client_SendEncoded(
                            server, caster, PacketAction_Reply, PacketFamily_Cast, reply);
                        return 1;
                    }
                    int damage = 0;
                    int hit_rate = Game::Combat_CalcArmorPen(
                        GUI->game_control, caster->accuracy, (*iter)->evade, 0.9);
                    if (RandRange(100) < hit_rate)
                    {
                        SkillDamage dmg =
                            SkillValues::GetDamage(GUI->skill_values, spell_id);
                        int min_total = caster->min_damage + dmg.min_damage;
                        int max_total = caster->max_damage + dmg.max_damage;
                        hit_rate = Game::Combat_CalcArmorPen(GUI->game_control,
                                                             (min_total + max_total) / 2,
                                                             (*iter)->armor,
                                                             0.8);
                        double scaled = (double)min_total;
                        if (scaled < 1.0)
                            scaled = 1.0;
                        scaled *= 0.01L;
                        scaled *= (double)hit_rate;
                        scaled += (double)RandRange(max_total - min_total + 2);
                        damage = (int)scaled;
                        if (damage < 1)
                            damage = 1;
                    }
                    if (spell_id > 0)
                    {
                        SkillElement element =
                            SkillValues::GetElement(GUI->skill_values, spell_id);
                        if (element.element == 1)
                            damage =
                                (int)((double)damage *
                                      Game::Combat_CalcElementMult(
                                          GUI->game_control,
                                          *(MapCoord *)&element,
                                          caster->element_resistances[1],
                                          (*iter)->element_weakness_damage_table[1]));
                        if (element.element == 2)
                            damage =
                                (int)((double)damage *
                                      Game::Combat_CalcElementMult(
                                          GUI->game_control,
                                          *(MapCoord *)&element,
                                          caster->element_resistances[2],
                                          (*iter)->element_weakness_damage_table[0]));
                        if (element.element == 3)
                            damage =
                                (int)((double)damage *
                                      Game::Combat_CalcElementMult(
                                          GUI->game_control,
                                          *(MapCoord *)&element,
                                          caster->element_resistances[3],
                                          (*iter)->element_weakness_damage_table[5]));
                        if (element.element == 4)
                            damage =
                                (int)((double)damage *
                                      Game::Combat_CalcElementMult(
                                          GUI->game_control,
                                          *(MapCoord *)&element,
                                          caster->element_resistances[4],
                                          (*iter)->element_weakness_damage_table[2]));
                        if (element.element == 5)
                            damage =
                                (int)((double)damage *
                                      Game::Combat_CalcElementMult(
                                          GUI->game_control,
                                          *(MapCoord *)&element,
                                          caster->element_resistances[5],
                                          (*iter)->element_weakness_damage_table[3]));
                        if (element.element == 6)
                            damage =
                                (int)((double)damage *
                                      Game::Combat_CalcElementMult(
                                          GUI->game_control,
                                          *(MapCoord *)&element,
                                          caster->element_resistances[6],
                                          (*iter)->element_weakness_damage_table[4]));
                    }
                    if ((unsigned short)(*iter)->boss > 0)
                        MapContainer::Mapcontrol_AggroChildNpcs(server->map_control,
                                                                caster->map_id);
                    (*iter)->aggressive = true;
                    (*iter)->nLeash_timer = (short)(RandRange(0x32) + 100);
                    if (MapContainer::Mapcontrol_CountNpcsChasingPlayer(
                            server->map_control, caster->map_id, caster->player_id) < 2 &&
                        type_info.behavior_id == 0)
                        (*iter)->chase_target_id = caster->player_id;
                    (*iter)->hp -= damage;
                    (*iter)->nHp_pct =
                        (short)((*iter)->hp * 100 /
                                NpcValues::GetMaxHp(GUI->npc_values, (*iter)->id));
                    if ((*iter)->hp < 1)
                    {
                        int exp = NpcValues::GetExp(GUI->npc_values, (*iter)->id);
                        int drop_item = 0;
                        exp = Party_ShareExp(server, caster, exp);
                        if ((*iter)->wDrop_item_id > 0 && (*iter)->wDrop_amount > 0)
                            drop_item = MapContainer::Mapcontrol_AddGroundItem(
                                server->map_control,
                                caster->map_id,
                                (*iter)->wDrop_item_id,
                                (*iter)->x,
                                (*iter)->y,
                                (*iter)->wDrop_amount,
                                caster->account_ident,
                                0x3d);
                        TDateTime now = Now();
                        *(TTimeStamp *)&(*iter)->nDeath_ms = DateTimeToTimeStamp(now);
                        (*iter)->chase_target_id = -1;
                        (*iter)->alive = false;
                        if ((unsigned short)(*iter)->boss > 0 &&
                            MapContainer::Mapcontrol_KillChildNpcs(server->map_control,
                                                                   caster->map_id))
                            Server_BroadcastToMap(
                                server,
                                caster->map_id,
                                PacketAction_Junk,
                                PacketFamily_Npc,
                                EO_EncodeNumber(
                                    server,
                                    (unsigned short)Mapcontrol_GetByIndex(
                                        server->map_control, caster->map_id - 1)
                                        ->child_npc_id,
                                    2));
                        String reply = EO_EncodeNumber(server, spell_id, 2);
                        reply.Insert(EO_EncodeNumber(server, caster->player_id, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->direction, 1),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->index, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, drop_item, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->wDrop_item_id, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->x, 1),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->y, 1),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, (*iter)->wDrop_amount, 4),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, damage, 3),
                                     reply.Length() + 1);
                        if (Settings::GetMaxKills(server->settings) != 0)
                        {
                            if (KillCounters::IncrementAndGet(server->kill_counters,
                                                              caster->name) >
                                Settings::GetMaxKills(server->settings))
                                exp = 0;
                        }
                        caster->experience += exp;
                        if (Players::Player_TryLevelUp(server->players, caster) > 0)
                        {
                            Server_BroadcastNearTile(server,
                                                     caster->player_id,
                                                     caster->map_id,
                                                     coords,
                                                     PacketAction_Accept,
                                                     PacketFamily_Cast,
                                                     reply);
                            reply.Insert(EO_EncodeNumber(server, caster->tp, 2),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->experience, 4),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->level, 1),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->stat_points, 2),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->skill_points, 2),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->max_hp, 2),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->max_tp, 2),
                                         reply.Length() + 1);
                            reply.Insert(EO_EncodeNumber(server, caster->max_sp, 2),
                                         reply.Length() + 1);
                            Client_SendEncoded(server,
                                               caster,
                                               PacketAction_Accept,
                                               PacketFamily_Cast,
                                               reply);
                            return 1;
                        }
                        Server_BroadcastNearTile(server,
                                                 caster->player_id,
                                                 caster->map_id,
                                                 coords,
                                                 PacketAction_Spec,
                                                 PacketFamily_Cast,
                                                 reply);
                        reply.Insert(EO_EncodeNumber(server, caster->tp, 2),
                                     reply.Length() + 1);
                        reply.Insert(EO_EncodeNumber(server, caster->experience, 4),
                                     reply.Length() + 1);
                        if (!caster->cheater_flag)
                            Client_SendEncoded(server,
                                               caster,
                                               PacketAction_Spec,
                                               PacketFamily_Cast,
                                               reply);
                        else
                        {
                            if (RandRange(6) > 2)
                                caster->experience -= exp;
                        }
                        Player_FireQuestTriggers(server, caster, 8, (*iter)->id);
                        return 1;
                    }
                    String reply = EO_EncodeNumber(server, spell_id, 2);
                    reply.Insert(EO_EncodeNumber(server, caster->player_id, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, caster->direction, 1),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, (*iter)->index, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, damage, 3), reply.Length() + 1);
                    if (!caster->cheater_flag)
                        reply.Insert(EO_EncodeNumber(server, (*iter)->nHp_pct, 2),
                                     reply.Length() + 1);
                    else if ((*iter)->nHp_pct < 1)
                        reply.Insert(EO_EncodeNumber(server, RandRange(2) + 1, 2),
                                     reply.Length() + 1);
                    Server_BroadcastNearTile(server,
                                             caster->player_id,
                                             caster->map_id,
                                             coords,
                                             PacketAction_Reply,
                                             PacketFamily_Cast,
                                             reply);
                    reply.Insert(EO_EncodeNumber(server, caster->tp, 2),
                                 reply.Length() + 1);
                    reply.Insert(EO_EncodeNumber(server, 1, 1), reply.Length() + 1);
                    Client_SendEncoded(
                        server, caster, PacketAction_Reply, PacketFamily_Cast, reply);
                    return 1;
                }
                return 1;
            }
            return 1;
        }
    }
    if (action == PacketAction_TargetGroup)
    {
        if (!caster->logged_in)
            return 0;
        if (caster->sitting || caster->on_chair)
            return 1;
        if (data->Length() < 5)
            return 0;
        int spell_id = EO_DecodeNumber(server, data->SubString(1, 2));
        int cast_tick = EO_DecodeNumber(server, data->SubString(3, 3));
        int target_type = SkillValues::GetTargetType(GUI->skill_values, spell_id);
        if (spell_id != caster->queued_spell_id)
            return 0;
        if (cast_tick < caster->expected_cast_timestamp &&
            (cast_tick > 1000 || caster->expected_cast_timestamp < 0x83ce30))
            return 0;
        if (target_type != SkillTargetType_Group)
            return 1;
        if (!Players::Player_HasSpellId(server->players, caster, spell_id))
            return 1;
        int skill_type = SkillValues::GetSkillType(GUI->skill_values, spell_id);
        int tp_cost = SkillValues::GetTpCost(GUI->skill_values, spell_id);
        int hp_heal = SkillValues::GetHpHeal(GUI->skill_values, spell_id);
        if (caster->tp < tp_cost)
        {
            String reply = EO_EncodeNumber(server, spell_id, 2);
            reply.Insert(EO_EncodeNumber(server, caster->hp, 2), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, caster->tp, 2), reply.Length() + 1);
            Client_SendEncoded(
                server, caster, PacketAction_Reply, PacketFamily_Spell, reply);
            return 1;
        }
        if (hp_heal < 1)
            return 1;
        if (!caster->in_party)
            return 1;
        caster->tp -= tp_cost;
        String reply = EO_EncodeNumber(server, spell_id, 2);
        reply.Insert(EO_EncodeNumber(server, caster->player_id, 2), reply.Length() + 1);
        reply.Insert(EO_EncodeNumber(server, caster->tp, 2), reply.Length() + 1);
        reply.Insert(EO_EncodeNumber(server, hp_heal, 2), reply.Length() + 1);
        for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
        {
            Player *target =
                Players::Players_GetById(server->players, caster->party_ids[i]);
            if (target != 0 && target->map_id == caster->map_id)
            {
                target->hp += hp_heal;
                if (target->hp > target->max_hp)
                    target->hp = target->max_hp;
                int hp_percent = target->hp * 100 / target->max_hp;
                reply.Insert(EO_EncodeNumber(server, target->player_id, 2),
                             reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, hp_percent, 1), reply.Length() + 1);
                reply.Insert(EO_EncodeNumber(server, target->hp, 2), reply.Length() + 1);
            }
        }
        Server_BroadcastToMap(
            server, caster->map_id, PacketAction_TargetGroup, PacketFamily_Spell, reply);
        return 1;
    }
    if (action == PacketAction_TargetSelf)
    {
        if (!caster->logged_in)
            return 0;
        if (caster->sitting || caster->on_chair)
            return 1;
        if (data->Length() < 6)
            return 0;
        int spell_id = EO_DecodeNumber(server, data->SubString(2, 2));
        int cast_tick = EO_DecodeNumber(server, data->SubString(4, 3));
        int target_type = SkillValues::GetTargetType(GUI->skill_values, spell_id);
        if (spell_id != caster->queued_spell_id)
            return 0;
        if (cast_tick < caster->expected_cast_timestamp &&
            (cast_tick > 1000 || caster->expected_cast_timestamp < 0x83ce30))
            return 0;
        if (target_type != SkillTargetType_Self)
            return 1;
        if (!Players::Player_HasSpellId(server->players, caster, spell_id))
            return 1;
        int skill_type = SkillValues::GetSkillType(GUI->skill_values, spell_id);
        int tp_cost = SkillValues::GetTpCost(GUI->skill_values, spell_id);
        int hp_heal = SkillValues::GetHpHeal(GUI->skill_values, spell_id);
        if (caster->tp < tp_cost)
        {
            String reply = EO_EncodeNumber(server, spell_id, 2);
            reply.Insert(EO_EncodeNumber(server, caster->hp, 2), reply.Length() + 1);
            reply.Insert(EO_EncodeNumber(server, caster->tp, 2), reply.Length() + 1);
            Client_SendEncoded(
                server, caster, PacketAction_Reply, PacketFamily_Spell, reply);
            return 1;
        }
        if (skill_type != 0)
            return 1;
        caster->hp += hp_heal;
        caster->tp -= tp_cost;
        if (caster->hp > caster->max_hp)
            caster->hp = caster->max_hp;
        int hp_percent = caster->hp * 100 / caster->max_hp;
        String reply = EO_EncodeNumber(server, caster->player_id, 2);
        reply.Insert(EO_EncodeNumber(server, spell_id, 2), reply.Length() + 1);
        reply.Insert(EO_EncodeNumber(server, hp_heal, 4), reply.Length() + 1);
        reply.Insert(EO_EncodeNumber(server, hp_percent, 1), reply.Length() + 1);
        Server_BroadcastNearby(
            server, caster, PacketAction_TargetSelf, PacketFamily_Spell, reply);
        reply.Insert(EO_EncodeNumber(server, caster->hp, 2), reply.Length() + 1);
        reply.Insert(EO_EncodeNumber(server, caster->tp, 2), reply.Length() + 1);
        Client_SendEncoded(
            server, caster, PacketAction_TargetSelf, PacketFamily_Spell, reply);
        return 1;
    }
    if (action == PacketAction_Use)
    {
        if (!caster->logged_in)
            return 0;
        if (caster->sitting || caster->on_chair)
            return 1;
        if (data->Length() < 1)
            return 0;
        caster->direction = EO_DecodeNumber(server, (*data)[1]);
        String reply = EO_EncodeNumber(server, caster->player_id, 2);
        reply = reply + (*data)[1];
        Server_BroadcastNearby(
            server, caster, PacketAction_Player, PacketFamily_Spell, reply);
        return 1;
    }
    return 0;
}

bool Walk_Execute(Packets *server, Player *player, int action, String *data)
{
    *(TTimeStamp *)&player->walk_tick = DateTimeToTimeStamp(Now());
    if (player->map_id < 1)
        return true;
    if (action != PacketAction_Player && action != PacketAction_Spec &&
        action != PacketAction_Admin)
        return false;
    if (action == PacketAction_Admin && player->admin_level < AdminLevel_Guardian)
        return false;
    if (action == PacketAction_Spec)
    {
        if (player->ghost_walk_tokens <= 0)
            return true;
        player->ghost_walk_tokens--;
    }
    if (data->Length() < 6)
        return false;
    int client_tick = EO_DecodeNumber(server, data->SubString(2, 3));
    int tick_delta = client_tick - player->last_client_walk_tick;
    if (tick_delta < 0 && player->last_client_walk_tick > 7500000)
        tick_delta = 44;
    player->last_client_walk_tick = client_tick;
    if (tick_delta < 44)
        return false;
    TTimeStamp stamp = DateTimeToTimeStamp(Now());
    int sync_tick = stamp.Time / 10 + 100;
    int clock_drift;
    if (client_tick > sync_tick)
    {
        clock_drift = client_tick - sync_tick;
        if (player->sync_base_ahead < 0)
            player->sync_base_ahead = clock_drift;
        if (Math_Abs(clock_drift - player->sync_base_ahead) > 800)
            return true;
    }
    else
    {
        clock_drift = sync_tick - client_tick;
        if (player->sync_base_behind < 0)
            player->sync_base_behind = clock_drift;
        if (Math_Abs(clock_drift - player->sync_base_behind) > 800)
            return true;
    }
    int direction = EO_DecodeNumber(server, (*data)[1]);
    int target_x = EO_DecodeNumber(server, (*data)[5]);
    int target_y = EO_DecodeNumber(server, (*data)[6]);
    if (direction > 3)
        return true;
    if (player->x == target_x && player->y == target_y)
    {
        if (player->direction != direction && !player->on_chair && !player->sitting)
        {
            player->direction = direction;
            String out = EO_EncodeNumber(server, player->player_id, 2);
            out.Insert(EO_EncodeNumber(server, direction, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->x, 1), out.Length() + 1);
            out.Insert(EO_EncodeNumber(server, player->y, 1), out.Length() + 1);
            Server_BroadcastNearby(
                server, player, PacketAction_Player, PacketFamily_Walk, out);
        }
        return true;
    }
    if (direction > 3)
        direction = Direction_Right;
    int coord_delta = (player->x + player->y) - (target_x + target_y);
    if (coord_delta > 3 || coord_delta < -3 || player->on_chair || player->sitting)
    {
        player->flush_queue = 1;
        if (!player->cheater_flag)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Refresh,
                               Refresh_BuildReply(server, player));
        }
        return true;
    }
    if (direction == Direction_Down)
    {
        target_y = player->y + 1;
        target_x = player->x;
    }
    if (direction == Direction_Left)
    {
        target_x = player->x - 1;
        target_y = player->y;
    }
    if (direction == Direction_Up)
    {
        target_y = player->y - 1;
        target_x = player->x;
    }
    if (direction == Direction_Right)
    {
        target_x = player->x + 1;
        target_y = player->y;
    }
    if (MapContainer::Mapcontrol_IsOccupied(
            server->map_control, player->map_id, target_x, target_y) &&
        action == PacketAction_Player)
        return true;
    if (Players::Players_IsPlayerAt(server->players, player->map_id, target_x, target_y))
    {
        if (action == PacketAction_Player)
        {
            if (!player->cheater_flag)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Refresh,
                                   Refresh_BuildReply(server, player));
            }
            return true;
        }
        if (player->admin_level < AdminLevel_LightGuide)
        {
            TTimeStamp now = DateTimeToTimeStamp(Now());
            int elapsed = now.Date - player->last_pass_ms.Date;
            int ms = now.Time - player->last_pass_ms.Time;
            elapsed = ms / 1000 + elapsed * 86400;
            player->last_pass_ms = now;
            if (elapsed < 7)
                return true;
            if (MapContainer::Mapcontrol_GetTileSpec(
                    server->map_control, player->map_id, target_x, target_y) ==
                MapTileSpec_Reserved31)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Refresh,
                                   Refresh_BuildReply(server, player));
                return true;
            }
        }
    }
    int walkable = MapContainer::Mapcontrol_IsWalkableNPC(
        server->map_control, player->map_id, target_x, target_y, 1);
    if (walkable == 0 || (walkable == 1 && action == PacketAction_Admin))
    {
        player->direction = direction;
        player->x = target_x;
        player->y = target_y;
        player->idle_ticks = 0;
        String reply = Walk_BuildReply(server, player);
        if (reply.Length() > 1)
            Client_SendEncoded(
                server, player, PacketAction_Reply, PacketFamily_Walk, reply);
        String walk = EO_EncodeNumber(server, player->player_id, 2);
        walk.Insert(EO_EncodeNumber(server, direction, 1), walk.Length() + 1);
        walk.Insert(EO_EncodeNumber(server, target_x, 1), walk.Length() + 1);
        walk.Insert(EO_EncodeNumber(server, target_y, 1), walk.Length() + 1);
        Server_BroadcastNearby(
            server, player, PacketAction_Player, PacketFamily_Walk, walk);
        if (player->map_has_spikes)
        {
            unsigned int spec = MapContainer::Mapcontrol_GetTileSpec(
                server->map_control, player->map_id, player->x, player->y);
            if (spec == MapTileSpec_TimedSpikes || spec == MapTileSpec_Spikes)
            {
                int damage = player->max_hp / 5;
                int died = 0;
                if (damage < 1)
                    damage = 1;
                player->hp -= damage;
                if (player->hp <= 0)
                {
                    player->hp = 0;
                    died = 1;
                }
                String dmg = EO_EncodeNumber(server, 2, 1);
                dmg.Insert(EO_EncodeNumber(server, damage, 2), dmg.Length() + 1);
                dmg.Insert(EO_EncodeNumber(server, player->hp, 2), dmg.Length() + 1);
                dmg.Insert(EO_EncodeNumber(server, player->max_hp, 2), dmg.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Spec, PacketFamily_Effect, dmg);
                dmg = EO_EncodeNumber(server, player->player_id, 2);
                dmg.Insert(EO_EncodeNumber(server, Player::HpPercent(player), 1),
                           dmg.Length() + 1);
                dmg.Insert(EO_EncodeNumber(server, died, 1), dmg.Length() + 1);
                dmg.Insert(EO_EncodeNumber(server, damage, 2), dmg.Length() + 1);
                Server_BroadcastNearby(
                    server, player, PacketAction_Admin, PacketFamily_Effect, dmg);
                if (died)
                {
                    Player_Respawn(server, player);
                    return true;
                }
            }
        }
        Player_FireQuestTriggers(server, player, 10, 0);
        return true;
    }
    if (walkable == 2)
    {
        int target_map = MapContainer::Mapcontrol_GetWarpMap(
            server->map_control, player->map_id, target_x, target_y);
        int level_req = MapContainer::Mapcontrol_GetWarpLevelReq(
            server->map_control, player->map_id, target_x, target_y);
        int warp_x = MapContainer::Mapcontrol_GetWarpX(
            server->map_control, player->map_id, target_x, target_y);
        int warp_y = MapContainer::Mapcontrol_GetWarpY(
            server->map_control, player->map_id, target_x, target_y);
        if (player->level < level_req)
            return true;
        if (target_map > 0 && target_map <= (int)server->map_control->maps.size())
        {
            if (Mapcontrol_GetByIndex(server->map_control, target_map - 1)->width < 1 ||
                Mapcontrol_GetByIndex(server->map_control, target_map - 1)->height < 1)
                return true;
            if (player->warp_state < 0)
                player->session_id = RandRange(50000) + 10000;
            player->warp_state = 0;
            player->warp_pending = true;
            player->dead = false;
            player->warp_map = target_map;
            player->warp_x = (short)warp_x;
            player->warp_y = (short)warp_y;
            if (target_map == player->map_id)
            {
                String out = EO_EncodeNumber(server, 1, 1);
                out.Insert(
                    EO_EncodeNumber(
                        server,
                        Mapcontrol_GetByIndex(server->map_control, target_map - 1)->rid,
                        2),
                    out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->session_id, 2),
                           out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Request, PacketFamily_Warp, out);
                return true;
            }
            else
            {
                String out = EO_EncodeNumber(server, 2, 1);
                out.Insert(
                    EO_EncodeNumber(
                        server,
                        Mapcontrol_GetByIndex(server->map_control, target_map - 1)->rid,
                        2),
                    out.Length() + 1);
                out.Insert(EO_EncodeNumber(server,
                                           (unsigned short)Mapcontrol_GetByIndex(
                                               server->map_control, target_map - 1)
                                               ->rid1,
                                           2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server,
                                           (unsigned short)Mapcontrol_GetByIndex(
                                               server->map_control, target_map - 1)
                                               ->rid2,
                                           2),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server,
                                           (unsigned short)Mapcontrol_GetByIndex(
                                               server->map_control, target_map - 1)
                                               ->filesize,
                                           3),
                           out.Length() + 1);
                out.Insert(EO_EncodeNumber(server, player->session_id, 2),
                           out.Length() + 1);
                Client_SendEncoded(
                    server, player, PacketAction_Request, PacketFamily_Warp, out);
                Player_FireQuestTriggers(server, player, 12, player->map_id);
                return true;
            }
        }
    }
    return true;
}

void Connection_Ping(Packets *server)
{
    FUN_00470584();
    bool found = false;
    int value = 0;
    while (!found)
    {
        found = true;
        value = RandRange(0xCA) + 10;
        for (int i = 0; i < 3; i++)
        {
            int recent = server->ping_history[i];
            if (value == recent)
                found = false;
        }
    }
    for (int i = 2; i >= 1; i--)
        server->ping_history[i] = server->ping_history[i - 1];
    server->ping_history[0] = value;
    int value2 = RandRange(0xCA) + 10;
    int sum = server->ping_history[0] + value2;
    String encoded = EO_EncodeNumber(server, sum, 2);
    encoded.Insert(EO_EncodeNumber(server, value2, 1), encoded.Length() + 1);
    Player **player_iter;
    for (player_iter = server->players->players.begin();
         player_iter != server->players->players.end();
         player_iter++)
    {
        if ((unsigned char)(*player_iter)->ping_timeout > 1)
        {
            (*player_iter)->removing = 1;
            Players::Players_MarkDirty(server->players);
        }
        else if ((*player_iter)->connected)
        {
            (*player_iter)->ping_timeout++;
            Client_SendEncoded(server,
                               *player_iter,
                               PacketAction_Player,
                               PacketFamily_Connection,
                               encoded);
        }
    }
}

void FUN_00470584()
{
    srand(time(0));
}

int FUN_00470598(int a0, int value)
{
    value++;
    int a1 = value % 11 + 1;
    int a2 = value % 9 + 1;
    int a3 = value % 0x7d4 + 1;
    value = 0xa94024 - value;
    value = value % (a1 * 0x77);
    value = a2 * value * 0x77;
    value = a3 + value + 0x1b138;
    return value;
}

String Account_DecodePassword(Packets *server, String value)
{
    String reversed = "";
    String result = "";
    try
    {
        for (int i = value.Length(); i >= 1; i--)
            reversed = reversed + value[i];
        for (int j = 1; j <= value.Length(); j++)
        {
            char c = reversed[j];
            char out = c;
            int v = (unsigned char)out;
            bool handled = false;
            if (!handled)
            {
                if (v >= '0' && v <= '9')
                {
                    v = '9' - v + '0';
                    out = (char)v;
                    c = out;
                    result.Insert(String(c), result.Length() + 1);
                    handled = true;
                }
            }
            if (!handled)
            {
                if (v >= 'a' && v <= 'z')
                {
                    v = 'z' - v + 'a';
                    out = (char)v;
                    c = out;
                    result.Insert(String(c), result.Length() + 1);
                    handled = true;
                }
            }
            if (!handled)
                result.Insert(String(c), result.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return result;
}

String Account_EncodePassword(Packets *server, String value)
{
    String reversed = "";
    String result = "";
    try
    {
        for (int i = value.Length(); i >= 1; i--)
            reversed = reversed + value[i];
        for (int j = 1; j <= value.Length(); j++)
        {
            char c = reversed[j];
            char out = c;
            int v = (unsigned char)out;
            bool handled = false;
            if (!handled)
            {
                if (v >= '0' && v <= '9')
                {
                    v = '9' - v + '0';
                    out = (char)v;
                    c = out;
                    result.Insert(String(c), result.Length() + 1);
                    handled = true;
                }
            }
            if (!handled)
            {
                if (v >= 'a' && v <= 'z')
                {
                    v = 'z' - v + 'a';
                    out = (char)v;
                    c = out;
                    result.Insert(String(c), result.Length() + 1);
                    handled = true;
                }
            }
            if (!handled)
                result.Insert(String(c), result.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return result;
}
String EO_EncodeNumber(Packets *server, unsigned int value, int width)
{
    int rem;
    char c;
    try
    {
        unsigned int quotient = 1;
        bool flag = true;
        for (int i = 0; i < width; i++)
        {
            if (flag)
            {
                double d = value / 253.0;
                quotient = d;
                rem = value % EO_NUM_MAX;
                c = rem + 1;
                server->encode_buffer[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = EO_NUM_EMPTY;
                server->encode_buffer[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String result(server->encode_buffer, width);
    return result;
}
String Account_DecodePassword(Packets *server, String value);
String Account_EncodePassword(Packets *server, String value);
void Login_SendCharacterList(Packets *server,
                             Player *player,
                             PacketAction action,
                             PacketFamily family,
                             String data);

unsigned int Server_DecodePacketLength(Packets *self, String data)
{
    int result = 0;
    try
    {
        for (int i = 1; i <= data.Length(); i++)
        {
            char c = data[i];
            unsigned char ch = c;
            if (ch == EO_NUM_EMPTY || ch == 0)
                break;
            int b = ch;
            b -= 1;
            if (i == 1)
                result += b;
            if (i == 2)
                result += b * EO_NUM_MAX;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}
int EO_DecodeNumber(Packets *self, String data)
{
    int result = 0;
    try
    {
        for (int i = 1; i <= data.Length(); i++)
        {
            char c = data[i];
            unsigned char ch = c;
            if (ch == EO_NUM_EMPTY || ch == 0)
                break;
            int b = ch;
            b -= 1;
            if (i == 1)
                result += b;
            if (i == 2)
                result += b * EO_NUM_MAX;
            if (i == 3)
                result += b * EO_NUM_MAX_2;
            if (i == 4)
                result += b * EO_NUM_MAX_3;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}
String EO_Encode_Interleave(Packets *server, int multiple, char *begin, char *end)
{
    int len = 0;
    try
    {
        std::stack<char> pending;
        std::deque<char> woven;
        for (; begin != end; begin++)
        {
            char c = *begin;
            int value = (unsigned char)c;
            if (value % multiple == 0)
                pending.push(*begin);
            else
            {
                while (!pending.empty())
                {
                    woven.push_back(pending.top());
                    pending.pop();
                }
                woven.push_back(*begin);
            }
        }
        while (!pending.empty())
        {
            woven.push_back(pending.top());
            pending.pop();
        }
        while (!woven.empty())
        {
            if (!woven.empty())
            {
                server->packet_buffer[len] = woven.front();
                woven.pop_front();
                len++;
            }
            if (!woven.empty())
            {
                server->packet_buffer[len] = woven.back();
                woven.pop_back();
                len++;
            }
        }
    }
    catch (...)
    {
        len = 0;
    }
    String data(server->packet_buffer, len);
    return data;
}
// Two unidentified element-count helpers (reference `0x44f97c` / `0x44f9bc`).
// The bodies are an `end() - begin()` over a 4-byte element type, emitted as two
// out-of-line accessor calls plus the signed divide-by-4 sequence; the container
// each one counts is NOT identified, so the type here is only a stand-in that
// reproduces the shape. It must be a pointer vector Packets already owns the
// accessors for -- using `MapContainer::maps` instead makes Packets.obj define
// `vector<MapItem>::end()`, which the reference keeps in MapContainer
// (`0x47ecd4`); see PLAN.md, "COMDAT ownership".
int FUN_0044f97c(MapContainer *map_control)
{
    vector<Npc *> *list = (vector<Npc *> *)map_control;
    return list->end() - list->begin();
}

int FUN_0044f9bc(MapContainer *map_control)
{
    vector<ItemObj *> *list = (vector<ItemObj *> *)map_control;
    return list->end() - list->begin();
}
String EO_Decode_Deinterleave(Packets *server, int multiple, char *begin, char *end)
{
    int len = 0;
    if (multiple > 0)
    {
        try
        {
            std::stack<char> pending;
            std::queue<char> woven;
            for (bool toggle = true; begin != end; begin++)
            {
                if (toggle)
                {
                    woven.push(*begin);
                    toggle = false;
                }
                else
                {
                    pending.push(*begin);
                    toggle = true;
                }
            }
            while (!pending.empty())
            {
                woven.push(pending.top());
                pending.pop();
            }
            pending.empty();
            while (!woven.empty())
            {
                char c = woven.front();
                woven.pop();
                unsigned char v = c;
                int value = v;
                if (value % multiple == 0)
                    pending.push(c);
                else
                {
                    while (!pending.empty())
                    {
                        server->packet_buffer[len++] = pending.top();
                        pending.pop();
                    }
                    server->packet_buffer[len++] = c;
                }
            }
            while (!pending.empty())
            {
                server->packet_buffer[len++] = pending.top();
                pending.pop();
            }
        }
        catch (...)
        {
            len = 0;
        }
    }
    String data(server->packet_buffer, len);
    return data;
}
int EO_DecodeByte(Packets *self, char value)
{
    char c = value;
    int result = (unsigned char)c;
    return result;
}
int FUN_00470598(int a0, int value);
char EO_GetBreakByte(Packets *self, int value)
{
    char c = value;
    char result = c;
    return result;
}
void Server_AddSentBytes(Packets *server, int value)
{
    server->sent_bytes += value;
    while (server->sent_bytes > 0x3ff)
    {
        server->sent_kilobytes++;
        server->sent_bytes -= BYTES_PER_KB;
    }
    while (server->sent_kilobytes > 0x3ff)
    {
        server->sent_megabytes++;
        server->sent_kilobytes -= BYTES_PER_KB;
    }
}
void Server_AddReceivedBytes(Packets *server, int value)
{
    server->received_bytes += value;
    while (server->received_bytes > 0x3ff)
    {
        server->received_kilobytes++;
        server->received_bytes -= BYTES_PER_KB;
    }
    while (server->received_kilobytes > 0x3ff)
    {
        server->received_megabytes++;
        server->received_kilobytes -= BYTES_PER_KB;
    }
}
void PacketReader_Init(Packets *reader, String data, unsigned char break_byte)
{
    reader->reader_pos = 1;
    reader->reader_data = data;
    reader->reader_len = data.Length();
    reader->reader_break_byte = break_byte;
}
String PacketReader_GetBreakString(Packets *reader)
{
    String result = "";
    try
    {
        if (reader->reader_len >= 1)
        {
            while (reader->reader_pos <= reader->reader_len)
            {
                if (reader->reader_data[reader->reader_pos] != reader->reader_break_byte)
                {
                    result.Insert(reader->reader_data[reader->reader_pos],
                                  result.Length() + 1);
                }
                else
                {
                    reader->reader_pos++;
                    break;
                }
                reader->reader_pos++;
            }
        }
    }
    catch (...)
    {
        result = "";
    }
    return result;
}
String
PacketReader_GetBreakStringAt(Packets *reader, int end, String break_str, char append)
{
    String result = "";
    try
    {
        if (break_str.Length() >= 1)
        {
            int i = 1;
            int len = break_str.Length();
            int j = 1;
            String x = "";
            while (i <= len)
            {
                if (j == end && break_str[i] != append)
                    result.Insert(break_str[i], result.Length() + 1);
                if (j <= end && break_str[i] == append)
                    j++;
                if (j > end)
                    break;
                i++;
            }
        }
    }
    catch (...)
    {
        result = "";
    }
    return result;
}
bool CharName_CheckUnique(Packets *server, String name)
{
    for (int i = 0; i < server->wordfilter->Count; i++)
    {
        if (server->wordfilter->Strings[i].Length() <= name.Length())
        {
            for (int j = 1; j <= name.Length(); j++)
            {
                if (server->wordfilter->Strings[i][1] == name[j])
                {
                    if (name.SubString(j, server->wordfilter->Strings[i].Length()) ==
                        server->wordfilter->Strings[i])
                        return false;
                }
            }
        }
    }
    return true;
}
bool Login_CheckConnectionThreshold(Packets *server)
{
    if (mySQLdb::Db_GetActiveConnectionCount(server->mysql_controls) > 0x14)
        return true;
    return false;
}
bool Coords_IsAdjacent(Packets *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (dx + dy <= 1)
        result = true;
    return result;
}
bool Server_InViewRange(Packets *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (x2 > x1 && y2 > y1)
    {
        if (dx + dy <= 0x0F)
            result = true;
    }
    else
    {
        if (dx + dy <= 0x0C)
            result = true;
    }
    return result;
}
bool Server_InViewRing(Packets *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (x2 > x1 && y2 > y1)
    {
        if (dx + dy <= 0x0F && dx + dy > 0x0D)
            result = true;
    }
    else
    {
        if (dx + dy <= 0x0C && dx + dy > 0x0A)
            result = true;
    }
    return result;
}

bool Server_InViewRangeReverse(Packets *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (x2 < x1 && y2 < y1)
    {
        if (dx + dy <= 0x0F)
            result = true;
    }
    else
    {
        if (dx + dy <= 0x0C)
            result = true;
    }
    return result;
}
bool Coords_IsWithinTwo(Packets *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (dx + dy <= 2)
        result = true;
    return result;
}
bool Server_InItemViewRing(Packets *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (dx + dy <= 0x0D && dx + dy > 0x0B)
        result = true;
    return result;
}
// Nothing calls this, so ilink32 drops the COMDAT -- but the empty-string
// literals it pools stay in the unit's _DATA. The reference's pool carries
// two more NUL bytes than its referenced `""` uses account for at exactly this
// point (`0x561ca8`-`0x561cb3`), which is that signature; only the count and position are
// observable, not the function they came from (as with NpcValues::ClearDrops).
void Packets_EmptyLiterals()
{
    "";
    "";
}
String Server_FormatSentTraffic(Packets *server)
{
    if (server->sent_megabytes > 0)
        return IntToStr(server->sent_megabytes) + "." +
               IntToStr(server->sent_kilobytes / 0x67).SubString(1, 2) + " Mb";
    if (server->sent_kilobytes > 0)
        return IntToStr(server->sent_kilobytes) + "." +
               IntToStr(server->sent_bytes / 0x67).SubString(1, 2) + " Kb";
    return "n/a";
}
String Server_FormatReceivedTraffic(Packets *server)
{
    if (server->received_megabytes > 0)
        return IntToStr(server->received_megabytes) + "." +
               IntToStr(server->received_kilobytes / 0x67).SubString(1, 2) + " Mb";
    if (server->received_kilobytes > 0)
        return IntToStr(server->received_kilobytes) + "." +
               IntToStr(server->received_bytes / 0x67).SubString(1, 2) + " Kb";
    return "n/a";
}
bool Server_TickOncePerFiveSeconds(Packets *server)
{
    TTimeStamp stamp = DateTimeToTimeStamp(server->start_time);
    TTimeStamp now = DateTimeToTimeStamp(Now());
    int days = now.Date - stamp.Date;
    int millis = now.Time - stamp.Time;
    int elapsed = millis / MS_PER_SECOND + days * SECONDS_PER_DAY;
    if (elapsed > 5)
    {
        server->start_time = Now();
        return true;
    }
    return false;
}
void Server_AppendChatLog(Packets *server, String message)
{
    String line = DateToStr(Now());
    line.Insert(" ", line.Length() + 1);
    line.Insert(TimeToStr(Now()), line.Length() + 1);
    line.Insert(" ", line.Length() + 1);
    line.Insert(message, line.Length() + 1);
    line.Insert("\n", line.Length() + 1);

    String idx = IntToStr(server->state_0x04);
    if (idx.Length() < 2)
        idx = "0" + idx;
    String path = ".\\logs\\chat" + idx + ".log";
    FILE *fp = fopen(path.c_str(), "a");
    fprintf(fp, "%s", line.c_str());
    fclose(fp);

    server->state_0x00++;
    if (server->state_0x00 > 0x7530)
    {
        server->state_0x04++;
        server->state_0x00 = 1;
        if (server->state_0x04 > 0x63)
            server->state_0x04 = 1;
        idx = IntToStr(server->state_0x04);
        if (idx.Length() < 2)
            idx = "0" + idx;
        path = ".\\logs\\chat" + idx + ".log";
        remove(path.c_str());
    }
}
