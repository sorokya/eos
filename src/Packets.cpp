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
#include "Innvalues.h"
#include "Mapcontrol.h"
#include "Mapobject.h"
#include "Protocol.h"

#pragma package(smart_init)

Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);
void Connection_Ping(Server *server);
bool Player_CheckIdleWarp(Server *server, Player *player, int x, int y);
bool Player_HandlePacket(Server *server, Player *player, String data);
bool FUN_00462374(Server *server, Player *player, String data);
void Server_BroadcastToParty(
    Server *server, Player *player, int action, int family, String data);
String Character_BuildSaveQuery(Players *players, Player *player, int flag);
extern TGUI **MAINFORM;
MapObject Map_GetTileSpecObject(Mapcontrol *map_control, int map_id, int x, int y);

void Game_Tick(Server *server)
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
        for (Player **player = Players_Iter_Begin(server->players);
             Players_Iter_End(server->players) != player;
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
                        Mysqlcontrols::Mysql_ExecDirect(
                            server->mysql_controls,
                            (*player)->field_0xc,
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
        if (server->shutting_down != 0 &&
            (unsigned int)Players::Players_ActiveCount(server->players) < 1 &&
            Mysqlcontrols::Database_CanReconnect(server->mysql_controls))
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

Server::Server(Mapcontrol *map_control,
               Questengine *quest_engine,
               Players *players,
               Settings *settings,
               Mysqlcontrols *mysql_controls,
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
    weapon_map = new WeaponmapEntry();
    banned = new Banned(mysql_controls);
    kill_counters = new KillCounters();
    quest_counters = new QuestCounters();
    this->map_control = map_control;
    this->quest_engine = quest_engine;
    this->players = players;
    this->logins = logins;
    this->mysql_controls = mysql_controls;
    this->settings = settings;
    Mysqlcontrols::Query(this->mysql_controls, "SELECT * FROM endl_wordfilter");
    wordfilter = new TStringList;
    while (!Mysqlcontrols::ResultAtEnd(this->mysql_controls))
    {
        wordfilter->Add(Mysqlcontrols::Db_GetString(this->mysql_controls, "word"));
        Mysqlcontrols::NextResultRecord(this->mysql_controls);
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

Server::~Server()
{
}

// The reference helpers are the out-of-line copies of `std::vector<T>::begin`/
// `end`; `-v` keeps them as calls, so the bodies reduce to the vector's start
// and finish pointers. `Mapcontrol::maps` is the vector at +0x00, whose
// `_M_start`/`_M_finish` land at +0x04/+0x08 (pinned by Mapcontrol_GetCount's
// end-minus-begin over 0x160 and by the type's constructor).
MapContainer *MapVector_Begin(Mapcontrol *map_control)
{
    return *(MapContainer **)((char *)map_control + 0x04);
}

MapContainer *MapVector_End(Mapcontrol *map_control)
{
    return *(MapContainer **)((char *)map_control + 0x08);
}

int Mapcontrol_GetCount(Mapcontrol *map_control)
{
    return map_control->maps.end() - map_control->maps.begin();
}

MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index)
{
    return MapVector_Begin(map_control) + index;
}

MapContainer *Mapcontrol_Iter_Front(Mapcontrol *map_control)
{
    return *(MapContainer **)((char *)map_control + 0x04);
}

void **Map_NpcIter_Begin(void *npc_list)
{
    return *(void ***)((char *)npc_list + 0x04);
}

void **Map_NpcIter_End(void *npc_list)
{
    return *(void ***)((char *)npc_list + 0x08);
}

void **GroundItemPtrVector_Begin(void *list)
{
    return *(void ***)((char *)list + 0x04);
}

void **PtrVector_GetEnd(void *list)
{
    return *(void ***)((char *)list + 0x08);
}

int GroundItemPtrVector_Count(void *list)
{
    return PtrVector_GetEnd(list) - GroundItemPtrVector_Begin(list);
}

int Math_Abs(int value)
{
    return __abs__(value);
}

unsigned int Db_GetActiveConnectionCount(Mysqlcontrols *db);

bool Login_CheckConnectionThreshold(Server *server)
{
    if (Db_GetActiveConnectionCount(server->mysql_controls) > 0x14)
        return true;
    return false;
}

Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);

int __fastcall Sock_Send(void *sock, char *data);

void Client_SendRaw(Server *server, Player *client, String data, int break_byte)
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
    String built = String(EO_GetBreakByte(server, 0xff));
    built.Insert(EO_GetBreakByte(server, 0xff), built.Length() + 1);
    built.Insert(EO_GetBreakByte(server, break_byte), built.Length() + 1);
    built.Insert(data, built.Length() + 1);
    built.Insert(EO_EncodeNumber(server, built.Length(), 2), 1);
    Sock_Send(client->socket, *(char **)&built);
}

bool Face_Execute(Server *server, Player *player, int action, String *data)
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

bool Chair_Execute(Server *server, Player *player, int action, String *data)
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
                Mapcontrol_GetCount(server->map_control) < player->map_id)
                return true;
            int spec =
                Mapcontrol::Map_GetTileSpec(server->map_control, player->map_id, x, y);
            if (spec >= 0 && spec <= 6)
            {
                MapObject tile =
                    Map_GetTileSpecObject(server->map_control, player->map_id, x, y);
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

void Player_FireQuestTriggers(Server *server, Player *player, int state_index, int value);

void Player_Warp(Server *server,
                 Player *player,
                 int target_map,
                 MapCoord coords,
                 int warp_anim,
                 bool do_leave)
{
    if (target_map == 0x50 || target_map == 0x51)
        return;
    if (target_map < 1 && Mapcontrol_GetCount(server->map_control) < target_map)
        return;
    if (Mapcontrol_GetByIndex(server->map_control, target_map - 1)->width < 1 ||
        Mapcontrol_GetByIndex(server->map_control, target_map - 1)->height < 1)
        return;
    if (player->warp_state < 0)
        player->session_id = RandRange(50000) + 10000;
    int old_map = player->map_id;
    player->warp_state = warp_anim;
    player->warp_pending = true;
    player->dead = false;
    player->warp_map = target_map;
    player->warp_x = coords.x;
    player->warp_y = coords.y;
    if (do_leave)
    {
        String out = EO_EncodeNumber(server, player->warp_state, 1) +
                     EO_EncodeNumber(server, player->player_id, 2);
        Server_BroadcastNearby(
            server, player, PacketAction_Remove, PacketFamily_Avatar, out);
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
            Mapcontrol::Mapcontrol_dec_player_count(server->map_control, player->map_id);
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
        out.Insert(EO_EncodeNumber(
                       server,
                       Mapcontrol_GetByIndex(server->map_control, target_map - 1)->rid1,
                       2),
                   out.Length() + 1);
        out.Insert(EO_EncodeNumber(
                       server,
                       Mapcontrol_GetByIndex(server->map_control, target_map - 1)->rid2,
                       2),
                   out.Length() + 1);
        out.Insert(
            EO_EncodeNumber(
                server,
                Mapcontrol_GetByIndex(server->map_control, target_map - 1)->filesize,
                3),
            out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->session_id, 2), out.Length() + 1);
        Client_SendEncoded(server, player, PacketAction_Request, PacketFamily_Warp, out);
        Player_FireQuestTriggers(server, player, 0xc, old_map);
    }
}

bool Player_CheckIdleWarp(Server *server, Player *player, int x, int y)
{
    if (Mapcontrol::Map_IsTileWalkable(server->map_control, player->map_id, x, y))
    {
        MapCoord coords;
        coords.x = x;
        coords.y = y;
        if (Mapcontrol::Map_GetWarpDoorAt(server->map_control, player->map_id, coords) <
            2)
        {
            MapCoord dest;
            int target_map =
                Mapcontrol::Map_GetWarpMap(server->map_control, player->map_id, x, y);
            int level_req = Mapcontrol::Map_GetWarpLevelReq(
                server->map_control, player->map_id, x, y);
            dest.x = Mapcontrol::Map_GetWarpX(server->map_control, player->map_id, x, y);
            dest.y = Mapcontrol::Map_GetWarpY(server->map_control, player->map_id, x, y);
            if (player->level < level_req)
                return false;
            player->flush_queue = 1;
            Player_Warp(server, player, target_map, dest, 0, false);
            return true;
        }
    }
    return false;
}

void Player_Respawn(Server *server, Player *player)
{
    MapCoord coords;
    int map_id =
        InnValues::GetSpawnMap((*MAINFORM)->inn_values, player->home_id, player->level);
    if (map_id < 0)
    {
        map_id = Settings::GetRescueMap(server->settings);
        coords.x = Settings::GetRescueX(server->settings);
        coords.y = Settings::GetRescueY(server->settings);
    }
    else
    {
        coords.x =
            InnValues::GetSpawnX((*MAINFORM)->inn_values, player->home_id, player->level);
        coords.y =
            InnValues::GetSpawnY((*MAINFORM)->inn_values, player->home_id, player->level);
    }
    if (player->level <= 3)
    {
        map_id = Settings::GetStartMap(server->settings);
        coords.x = Settings::GetStartX(server->settings);
        coords.y = Settings::GetStartY(server->settings);
    }
    player->flush_queue = 1;
    Player_Warp(server, player, map_id, coords, 0, true);
}

void Server_BroadcastToPartyExceptSelf(Server *server,
                                       Player *player,
                                       unsigned char action,
                                       unsigned char family,
                                       String data)
{
    for (int i = 0; i < 0xa; i++)
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

void Server_BroadcastToParty(Server *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data)
{
    for (int i = 0; i < 0xa; i++)
    {
        Player *member = Players::Players_GetById(server->players, player->party_ids[i]);
        if (member != NULL && member->logged_in)
            Client_SendEncoded(server, member, action, family, data);
    }
}

void Guild_BroadcastToAll(Server *server,
                          Player *player,
                          unsigned char action,
                          unsigned char family,
                          String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
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

void Server_BroadcastAdjacent(Server *server,
                              Player *player,
                              int x,
                              int y,
                              unsigned char action,
                              unsigned char family,
                              String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
    {
        if ((*player_iter)->map_id == player->map_id &&
            Coords_IsAdjacent(server, x, y, (*player_iter)->x, (*player_iter)->y) &&
            (*player_iter)->logged_in && (*player_iter)->player_id != player->player_id)
        {
            Client_SendEncoded(server, *player_iter, action, family, data);
        }
    }
}

void Server_BroadcastNearTile(Server *server,
                              int skip_id,
                              int map_id,
                              int x,
                              int y,
                              unsigned char action,
                              unsigned char family,
                              String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
    {
        if ((*player_iter)->map_id == map_id && (*player_iter)->player_id != skip_id &&
            Server_InViewRange(server, x, y, (*player_iter)->x, (*player_iter)->y))
        {
            Client_SendEncoded(server, *player_iter, action, family, data);
        }
    }
}

void Admin_BroadcastToAll(Server *server,
                          unsigned char action,
                          unsigned char family,
                          String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
    {
        if ((*player_iter)->admin_level > AdminLevel_Player && (*player_iter)->logged_in)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Admin_ReportToGMs(Server *server,
                       Player *player,
                       unsigned char action,
                       unsigned char family,
                       String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
    {
        if ((*player_iter)->global_chat && (*player_iter)->logged_in &&
            (*player_iter)->player_id != player->player_id)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Admin_BroadcastToAdmins(Server *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
    {
        if ((*player_iter)->logged_in && (*player_iter)->player_id != player->player_id)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void Server_BroadcastNearby(Server *server,
                            Player *player,
                            unsigned char action,
                            unsigned char family,
                            String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
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

void Server_BroadcastToMap(
    Server *server, int map_id, unsigned char action, unsigned char family, String data)
{
    Player **player_iter;
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
    {
        if ((*player_iter)->map_id == map_id && (*player_iter)->logged_in)
            Client_SendEncoded(server, *player_iter, action, family, data);
    }
}

void FUN_00463d40(Server *server, int action, int family, String data);
void Server_Shutdown(Server *server)
{
    FUN_00463d40(server, PacketAction_Close, PacketFamily_Message, "r");
    Players::Players_MarkDirty(server->players);
    for (Player **player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
         player_iter++)
        (*player_iter)->removing = 1;
    server->shutting_down = 1;
}

void Connection_Ping(Server *server)
{
    Randomize();
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
    for (player_iter = Players_Iter_Begin(server->players);
         player_iter != Players_Iter_End(server->players);
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

void Server_ClientRead(Server *server, TCustomWinSocket *socket, String data)
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

String EO_EncodeNumber(Server *server, unsigned int value, int width)
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
                rem = value % 0xfd;
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
                char pad = 0xfe;
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

int EO_DecodeNumber(void *self, String data)
{
    int result = 0;
    try
    {
        for (int i = 1; i <= data.Length(); i++)
        {
            char c = data[i];
            unsigned char ch = c;
            if (ch == 0xFE || ch == 0)
                break;
            int b = ch;
            b -= 1;
            if (i == 1)
                result += b;
            if (i == 2)
                result += b * 0xFD;
            if (i == 3)
                result += b * 0xFA09;
            if (i == 4)
                result += b * 0xF71AE5;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}

int EO_DecodeByte(void *self, char value)
{
    char c = value;
    int result = (unsigned char)c;
    return result;
}

char EO_GetBreakByte(void *self, int value)
{
    char c = value;
    char result = c;
    return result;
}

void PacketReader_Init(Server *reader, String data, unsigned char break_byte)
{
    reader->reader_pos = 1;
    reader->reader_data = data;
    reader->reader_len = data.Length();
    reader->reader_break_byte = break_byte;
}

String PacketReader_GetBreakString(Server *reader)
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

String PacketReader_GetBreakStringAt(void *reader, int end, String break_str, char append)
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

bool CharName_CheckUnique(Server *server, String name)
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

unsigned int Server_DecodePacketLength(void *self, String data)
{
    int result = 0;
    try
    {
        for (int i = 1; i <= data.Length(); i++)
        {
            char c = data[i];
            unsigned char ch = c;
            if (ch == 0xFE || ch == 0)
                break;
            int b = ch;
            b -= 1;
            if (i == 1)
                result += b;
            if (i == 2)
                result += b * 0xFD;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}

bool Coords_IsAdjacent(void *self, int x1, int y1, int x2, int y2)
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

bool Server_InViewRange(void *self, int x1, int y1, int x2, int y2)
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

bool Server_InViewRing(void *self, int x1, int y1, int x2, int y2)
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

bool Server_InViewRangeReverse(void *self, int x1, int y1, int x2, int y2)
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

bool Server_InItemViewRing(void *self, int x1, int y1, int x2, int y2)
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
