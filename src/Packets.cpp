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
bool Player_HandlePacket(Server *server, Player *player, String data);
bool FUN_00462374(Server *server, Player *player, String data);
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

bool Login_CheckConnectionThreshold(Server *server)
{
    if (Mysqlcontrols::Db_GetActiveConnectionCount(server->mysql_controls) > 0x14)
        return true;
    return false;
}

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

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
// STUB(0x0041728c, 905 bytes) FUN_0041728c - ref: undefined4 FUN_0041728c(Server *
// server, undefined * param2)
int FUN_0041728c_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x0044f58c, 33 bytes) Exception_InstallFrame - ref: undefined4
// Exception_InstallFrame(void * passthrough_value)
int Exception_InstallFrame_Stub(void *a0)
{
    return 0;
}
// STUB(0x0044f6c8, 35 bytes) FUN_0044f6c8 - ref: undefined FUN_0044f6c8(int param_1, byte
// param_2)
void FUN_0044f6c8_Stub(int a0, unsigned char a1)
{
}
// STUB(0x0044f710, 43 bytes) FUN_0044f710 - ref: undefined FUN_0044f710(int param_1)
void FUN_0044f710_Stub(int a0)
{
}
// STUB(0x0044f73c, 60 bytes) FUN_0044f73c - ref: int FUN_0044f73c(int param_1)
int FUN_0044f73c_Stub(int a0)
{
    return 0;
}
// STUB(0x0044f778, 155 bytes) FUN_0044f778 - ref: int FUN_0044f778(int param_1,
// undefined4 * param_2, undefined4 * param_3)
int FUN_0044f778_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0044f814, 155 bytes) FUN_0044f814 - ref: int FUN_0044f814(int param_1,
// undefined4 * param_2, undefined4 * param_3)
int FUN_0044f814_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0044f8d8, 36 bytes) FUN_0044f8d8 - ref: int FUN_0044f8d8(int param_1)
int FUN_0044f8d8_Stub(int a0)
{
    return 0;
}
// STUB(0x0044f8fc, 22 bytes) FUN_0044f8fc - ref: int FUN_0044f8fc(int param_1, int
// param_2)
int FUN_0044f8fc_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0044f914, 101 bytes) FUN_0044f914 - ref: undefined4 * FUN_0044f914(int param_1,
// undefined4 * param_2)
void *FUN_0044f914_Stub(int a0, void *a1)
{
    return 0;
}
// STUB(0x0044f97c, 38 bytes) FUN_0044f97c - ref: int FUN_0044f97c(int param_1)
int FUN_0044f97c_Stub(int a0)
{
    return 0;
}
// STUB(0x0044f9bc, 38 bytes) FUN_0044f9bc - ref: int FUN_0044f9bc(int param_1)
int FUN_0044f9bc_Stub(int a0)
{
    return 0;
}
// STUB(0x0044f9fc, 169 bytes) FUN_0044f9fc - ref: undefined FUN_0044f9fc(int param_1)
void FUN_0044f9fc_Stub(int a0)
{
}
// STUB(0x0044faa8, 33 bytes) FUN_0044faa8 - ref: undefined FUN_0044faa8(int param_1)
void FUN_0044faa8_Stub(int a0)
{
}
// STUB(0x0044facc, 17 bytes) FUN_0044facc - ref: int FUN_0044facc(int param_1)
int FUN_0044facc_Stub(int a0)
{
    return 0;
}
// STUB(0x0044fae0, 28 bytes) FUN_0044fae0 - ref: undefined FUN_0044fae0(LONG * param_1,
// int param_2)
void FUN_0044fae0_Stub(void *a0, int a1)
{
}
// STUB(0x0044fb08, 17 bytes) FUN_0044fb08 - ref: undefined4 FUN_0044fb08(int param_1)
int FUN_0044fb08_Stub(int a0)
{
    return 0;
}
// STUB(0x0044fb1c, 19 bytes) FUN_0044fb1c - ref: undefined FUN_0044fb1c(undefined4
// param_1, undefined4 param_2, undefined4 * param_3)
void FUN_0044fb1c_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x0044fd94, 19 bytes) FUN_0044fd94 - ref: undefined FUN_0044fd94(undefined4
// param_1, undefined4 param_2, undefined4 * param_3)
void FUN_0044fd94_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x00450010, 11 bytes) FUN_00450010 - ref: undefined4 FUN_00450010(int param_1)
int FUN_00450010_Stub(int a0)
{
    return 0;
}
// STUB(0x0045001c, 11 bytes) FUN_0045001c - ref: undefined4 FUN_0045001c(int param_1)
int FUN_0045001c_Stub(int a0)
{
    return 0;
}
// STUB(0x00450034, 11 bytes) FUN_00450034 - ref: undefined4 FUN_00450034(int param_1)
int FUN_00450034_Stub(int a0)
{
    return 0;
}
// STUB(0x00450040, 11 bytes) FUN_00450040 - ref: undefined4 FUN_00450040(int param_1)
int FUN_00450040_Stub(int a0)
{
    return 0;
}
// STUB(0x0045004c, 11 bytes) FUN_0045004c - ref: undefined4 FUN_0045004c(int param_1)
int FUN_0045004c_Stub(int a0)
{
    return 0;
}
// STUB(0x00450058, 11 bytes) FUN_00450058 - ref: int FUN_00450058(int * param_1)
int FUN_00450058_Stub(void *a0)
{
    return 0;
}
// STUB(0x00450064, 31 bytes) FUN_00450064 - ref: int FUN_00450064(LONG * param_1)
int FUN_00450064_Stub(void *a0)
{
    return 0;
}
// STUB(0x004500ec, 33 bytes) FUN_004500ec - ref: int FUN_004500ec(int param_1, int
// param_2, undefined4 * param_3)
int FUN_004500ec_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x00450110, 10 bytes) FUN_00450110 - ref: undefined * FUN_00450110(void)
void *FUN_00450110_Stub()
{
    return 0;
}
// STUB(0x0045011c, 113 bytes) FUN_0045011c - ref: undefined FUN_0045011c(undefined4
// param_1, undefined4 * param_2)
void FUN_0045011c_Stub(int a0, void *a1)
{
}
// STUB(0x00450190, 88 bytes) FUN_00450190 - ref: undefined4 * FUN_00450190(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00450190_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004501e8, 114 bytes) FUN_004501e8 - ref: int FUN_004501e8(undefined4 param_1,
// int param_2)
int FUN_004501e8_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0045025c, 99 bytes) FUN_0045025c - ref: int FUN_0045025c(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_0045025c_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x004502e4, 33 bytes) FUN_004502e4 - ref: int FUN_004502e4(int param_1, int
// param_2, undefined4 * param_3)
int FUN_004502e4_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x00450308, 144 bytes) FUN_00450308 - ref: undefined FUN_00450308(undefined4
// param_1, undefined4 * param_2)
void FUN_00450308_Stub(int a0, void *a1)
{
}
// STUB(0x004503b4, 53 bytes) FUN_004503b4 - ref: undefined4 * FUN_004503b4(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_004503b4_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004503ec, 114 bytes) FUN_004503ec - ref: int FUN_004503ec(undefined4 param_1,
// int param_2)
int FUN_004503ec_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00450460, 99 bytes) FUN_00450460 - ref: int FUN_00450460(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_00450460_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x004504e8, 33 bytes) FUN_004504e8 - ref: int FUN_004504e8(int param_1, int
// param_2, undefined4 * param_3)
int FUN_004504e8_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x0045050c, 36 bytes) FUN_0045050c - ref: int FUN_0045050c(LONG * param_1)
int FUN_0045050c_Stub(void *a0)
{
    return 0;
}
// STUB(0x00450530, 5 bytes) FUN_00450530 - ref: undefined FUN_00450530(void)
void FUN_00450530_Stub()
{
}
// STUB(0x00450538, 54 bytes) FUN_00450538 - ref: int FUN_00450538(int param_1, int
// param_2)
int FUN_00450538_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00450570, 25 bytes) FUN_00450570 - ref: undefined FUN_00450570(int param_1, int
// param_2)
void FUN_00450570_Stub(int a0, int a1)
{
}
// STUB(0x0045058c, 54 bytes) FUN_0045058c - ref: int FUN_0045058c(int param_1, int
// param_2)
int FUN_0045058c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004505c4, 25 bytes) FUN_004505c4 - ref: undefined FUN_004505c4(int param_1, int
// param_2)
void FUN_004505c4_Stub(int a0, int a1)
{
}
// STUB(0x004505e0, 54 bytes) FUN_004505e0 - ref: int FUN_004505e0(int param_1, int
// param_2)
int FUN_004505e0_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00450618, 36735 bytes) MysqlCallback_Dispatch - ref: void
// MysqlCallback_Dispatch(Server * server, int * query_result)
void MysqlCallback_Dispatch_Stub(void *a0, void *a1)
{
}
// STUB(0x00459638, 101 bytes) FUN_00459638 - ref: undefined4 * FUN_00459638(int param_1,
// undefined4 * param_2)
void *FUN_00459638_Stub(int a0, void *a1)
{
    return 0;
}
// STUB(0x004596a0, 42 bytes) FUN_004596a0 - ref: undefined4 * FUN_004596a0(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_004596a0_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00459700, 5 bytes) FUN_00459700 - ref: undefined FUN_00459700(void)
void FUN_00459700_Stub()
{
}
// STUB(0x00459708, 182 bytes) Player_FireQuestTriggers - ref: void
// Player_FireQuestTriggers(Server * server, Player * player, int event, int arg)
void Player_FireQuestTriggers_Stub(void *a0, void *a1, int a2, int a3)
{
}
// STUB(0x004597c0, 930 bytes) Player_EvaluateQuestRules - ref: void
// Player_EvaluateQuestRules(Server * server, Player * player, Questtracker * tracker,
// Queststate * state, int event, int arg)
void Player_EvaluateQuestRules_Stub(
    void *a0, void *a1, void *a2, void *a3, int a4, int a5)
{
}
// STUB(0x00459b64, 11 bytes) FUN_00459b64 - ref: undefined4 FUN_00459b64(int param_1)
int FUN_00459b64_Stub(int a0)
{
    return 0;
}
// STUB(0x00459b70, 5458 bytes) Player_ApplyQuestActions - ref: void
// Player_ApplyQuestActions(Server * server, Player * player, Questtracker * tracker, bool
// repeat)
void Player_ApplyQuestActions_Stub(void *a0, void *a1, void *a2, int a3)
{
}
// STUB(0x0045b0c4, 11 bytes) FUN_0045b0c4 - ref: undefined4 FUN_0045b0c4(int param_1)
int FUN_0045b0c4_Stub(int a0)
{
    return 0;
}
// STUB(0x0045b0d0, 11 bytes) FUN_0045b0d0 - ref: undefined4 FUN_0045b0d0(int param_1)
int FUN_0045b0d0_Stub(int a0)
{
    return 0;
}
// STUB(0x0045b0dc, 9419 bytes) Login_SendCharacterList - ref: void
// Login_SendCharacterList(Server * server, Player * player, PacketAction action,
// PacketFamily family, AnsiString * data)
void Login_SendCharacterList_Stub(void *a0, void *a1, int a2, int a3, void *a4)
{
}
// STUB(0x0045d5a8, 715 bytes) Party_EncodeMemberList - ref: AnsiString *
// Party_EncodeMemberList(AnsiString * out_str, Server * server, Player * player)
void *Party_EncodeMemberList_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0045d874, 1342 bytes) Walk_BuildReply - ref: AnsiString *
// Walk_BuildReply(AnsiString * out, Server * server, Player * player)
void *Walk_BuildReply_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0045de20, 5386 bytes) Refresh_BuildReply - ref: AnsiString *
// Refresh_BuildReply(AnsiString * out, Server * server, Player * player)
void *Refresh_BuildReply_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0045f37c, 3291 bytes) Player_SerializeAvatar - ref: int *
// Player_SerializeAvatar(int * param_1, int param_2, int param_3, uint param_4)
void *Player_SerializeAvatar_Stub(void *a0, int a1, int a2, unsigned int a3)
{
    return 0;
}
// STUB(0x00460068, 1875 bytes) Player_SerializePaperdoll - ref: int *
// Player_SerializePaperdoll(int * param_1, int param_2, int param_3)
void *Player_SerializePaperdoll_Stub(void *a0, int a1, int a2)
{
    return 0;
}
// STUB(0x004607c8, 3110 bytes) Paperdoll_BuildReply - ref: int *
// Paperdoll_BuildReply(AnsiString * data, Server * server, Player * player)
void *Paperdoll_BuildReply_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004613fc, 459 bytes) Server_BuildOnlineNames - ref: AnsiString *
// Server_BuildOnlineNames(AnsiString * out_str, Server * server)
void *Server_BuildOnlineNames_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x004615d0, 889 bytes) Message_BuildServerStatus - ref: int *
// Message_BuildServerStatus(int * param_1, int param_2)
void *Message_BuildServerStatus_Stub(void *a0, int a1)
{
    return 0;
}
// STUB(0x0046194c, 1629 bytes) Server_BuildOnlineList - ref: AnsiString *
// Server_BuildOnlineList(AnsiString * out_str, Server * server)
void *Server_BuildOnlineList_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x00461fb4, 948 bytes) NpcRange_Lookup - ref: int * NpcRange_Lookup(int * out,
// Server * server, Player * player, uint npc_index)
void *NpcRange_Lookup_Stub(void *a0, void *a1, void *a2, int a3)
{
    return 0;
}
// STUB(0x00462374, 1339 bytes) FUN_00462374 - ref: undefined4 FUN_00462374(int param_1,
// int param_2)
int FUN_00462374_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004628b0, 838 bytes) FUN_004628b0 - ref: int * FUN_004628b0(int * param_1, int
// param_2, int param_3)
void *FUN_004628b0_Stub(void *a0, int a1, int a2)
{
    return 0;
}
// STUB(0x00462bf8, 574 bytes) FUN_00462bf8 - ref: int * FUN_00462bf8(int * param_1, int
// param_2)
void *FUN_00462bf8_Stub(void *a0, int a1)
{
    return 0;
}
// STUB(0x00462e38, 509 bytes) FUN_00462e38 - ref: int * FUN_00462e38(int * param_1, int
// param_2)
void *FUN_00462e38_Stub(void *a0, int a1)
{
    return 0;
}
// STUB(0x00463408, 492 bytes) Talk_PlayerWhisper - ref: undefined Talk_PlayerWhisper(int
// param_1, int param_2, undefined4 param_3, undefined4 param_4)
void Talk_PlayerWhisper_Stub(int a0, int a1, int a2, int a3)
{
}
// STUB(0x00463750, 183 bytes) FUN_00463750 - ref: undefined FUN_00463750(int param_1, int
// param_2, byte param_3, byte param_4, int param_5)
void FUN_00463750_Stub(int a0, int a1, unsigned char a2, unsigned char a3, int a4)
{
}
// STUB(0x004639b8, 179 bytes) FUN_004639b8 - ref: undefined FUN_004639b8(int param_1, int
// param_2, byte param_3, byte param_4, int param_5)
void FUN_004639b8_Stub(int a0, int a1, unsigned char a2, unsigned char a3, int a4)
{
}
// STUB(0x00463be8, 179 bytes) FUN_00463be8 - ref: undefined FUN_00463be8(int param_1, int
// param_2, byte param_3, byte param_4, int param_5)
void FUN_00463be8_Stub(int a0, int a1, unsigned char a2, unsigned char a3, int a4)
{
}
// STUB(0x00463d40, 149 bytes) FUN_00463d40 - ref: undefined FUN_00463d40(int param_1,
// byte param_2, byte param_3, int param_4)
void FUN_00463d40_Stub(int a0, unsigned char a1, unsigned char a2, int a3)
{
}
// STUB(0x00464030, 1286 bytes) Client_SendEncoded - ref: undefined
// Client_SendEncoded(Server * server, Player * player, PacketAction action, PacketFamily
// family)
void Client_SendEncoded_Stub(void *a0, void *a1, int a2, int a3)
{
}
// STUB(0x0046466c, 1487 bytes) Player_Warp - ref: void Player_Warp(Server * server,
// Player * player, int target_map, short warp_x, short warp_y, int warp_anim, bool
// do_leave)
void Player_Warp_Stub(void *a0, void *a1, int a2, int a3, int a4, int a5, int a6)
{
}
// STUB(0x00464d84, 1054 bytes) Player_CalculateStats - ref: undefined4
// Player_CalculateStats(Server * server, Player * player)
int Player_CalculateStats_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x004651a4, 5786 bytes) Player_ApplyEquipmentBonuses - ref: undefined
// Player_ApplyEquipmentBonuses(Server * server, Player * player)
void Player_ApplyEquipmentBonuses_Stub(void *a0, void *a1)
{
}
// STUB(0x00466840, 204 bytes) FUN_00466840 - ref: undefined FUN_00466840(Server * server,
// int map_id)
void FUN_00466840_Stub(void *a0, int a1)
{
}
// STUB(0x0046690c, 1198 bytes) Party_ShareExp - ref: uint Party_ShareExp(Server * server,
// Player * player, uint exp)
unsigned int Party_ShareExp_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00467980, 12223 bytes) Attack_Execute - ref: int Attack_Execute(Server * server,
// Player * attacker, PacketAction action, AnsiString * packet_data)
int Attack_Execute_Stub(void *a0, void *a1, int a2, void *a3)
{
    return 0;
}
// STUB(0x0046a9b0, 17449 bytes) Spell_Execute - ref: int Spell_Execute(Server * server,
// Player * caster, int action, AnsiString * packet_data)
int Spell_Execute_Stub(void *a0, void *a1, int a2, void *a3)
{
    return 0;
}
// STUB(0x0046ee4c, 5466 bytes) Walk_Execute - ref: undefined4 Walk_Execute(Server *
// server, Player * player, PacketAction action, AnsiString * data)
int Walk_Execute_Stub(void *a0, void *a1, int a2, void *a3)
{
    return 0;
}
// STUB(0x00470584, 20 bytes) FUN_00470584 - ref: undefined FUN_00470584(void)
void FUN_00470584_Stub()
{
}
// STUB(0x00470598, 114 bytes) FUN_00470598 - ref: int FUN_00470598(undefined4 param_1,
// int param_2)
int FUN_00470598_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047060c, 698 bytes) FUN_0047060c - ref: int * FUN_0047060c(int * param_1)
void *FUN_0047060c_Stub(void *a0)
{
    return 0;
}
// STUB(0x004708d4, 698 bytes) FUN_004708d4 - ref: int * FUN_004708d4(int * param_1)
void *FUN_004708d4_Stub(void *a0)
{
    return 0;
}
// STUB(0x00470d06, 207 bytes) FUN_00470d06 - ref: undefined4 FUN_00470d06(void)
int FUN_00470d06_Stub()
{
    return 0;
}
// STUB(0x00470dee, 239 bytes) FUN_00470dee - ref: undefined4 FUN_00470dee(void)
int FUN_00470dee_Stub()
{
    return 0;
}
// STUB(0x00470ef0, 759 bytes) EO_Encode_Interleave - ref: void EO_Encode_Interleave(char
// * data, int len, char * out)
void EO_Encode_Interleave_Stub(void *a0, int a1, void *a2)
{
}
// STUB(0x004712e4, 50 bytes) FUN_004712e4 - ref: undefined FUN_004712e4(undefined4 *
// param_1, byte param_2)
void FUN_004712e4_Stub(void *a0, unsigned char a1)
{
}
// STUB(0x00471318, 56 bytes) FUN_00471318 - ref: int FUN_00471318(int param_1, int
// param_2)
int FUN_00471318_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471388, 97 bytes) FUN_00471388 - ref: int FUN_00471388(int param_1)
int FUN_00471388_Stub(int a0)
{
    return 0;
}
// STUB(0x00471418, 19 bytes) FUN_00471418 - ref: undefined FUN_00471418(undefined4 *
// param_1, undefined1 * param_2)
void FUN_00471418_Stub(void *a0, void *a1)
{
}
// STUB(0x0047142c, 14 bytes) FUN_0047142c - ref: undefined FUN_0047142c(int param_1)
void FUN_0047142c_Stub(int a0)
{
}
// STUB(0x0047143c, 14 bytes) FUN_0047143c - ref: undefined FUN_0047143c(int param_1)
void FUN_0047143c_Stub(int a0)
{
}
// STUB(0x0047144c, 127 bytes) FUN_0047144c - ref: undefined FUN_0047144c(undefined4 *
// param_1, undefined1 * param_2)
void FUN_0047144c_Stub(void *a0, void *a1)
{
}
// STUB(0x004714cc, 14 bytes) FUN_004714cc - ref: undefined FUN_004714cc(undefined4 *
// param_1)
void FUN_004714cc_Stub(void *a0)
{
}
// STUB(0x004714dc, 18 bytes) FUN_004714dc - ref: bool FUN_004714dc(int param_1)
bool FUN_004714dc_Stub(int a0)
{
    return 0;
}
// STUB(0x004714f0, 35 bytes) FUN_004714f0 - ref: undefined FUN_004714f0(int param_1)
void FUN_004714f0_Stub(int a0)
{
}
// STUB(0x00471514, 136 bytes) FUN_00471514 - ref: undefined FUN_00471514(undefined4 *
// param_1)
void FUN_00471514_Stub(void *a0)
{
}
// STUB(0x0047159c, 53 bytes) FUN_0047159c - ref: undefined FUN_0047159c(int param_1)
void FUN_0047159c_Stub(int a0)
{
}
// STUB(0x004715d4, 124 bytes) FUN_004715d4 - ref: undefined FUN_004715d4(undefined4 *
// param_1)
void FUN_004715d4_Stub(void *a0)
{
}
// STUB(0x00471650, 182 bytes) FUN_00471650 - ref: int FUN_00471650(int param_1, int
// param_2)
int FUN_00471650_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471708, 64 bytes) FUN_00471708 - ref: int FUN_00471708(int param_1)
int FUN_00471708_Stub(int a0)
{
    return 0;
}
// STUB(0x00471748, 55 bytes) FUN_00471748 - ref: int FUN_00471748(int param_1, undefined4
// param_2)
int FUN_00471748_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471780, 26 bytes) FUN_00471780 - ref: int FUN_00471780(int param_1, int
// param_2)
int FUN_00471780_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047179c, 740 bytes) FUN_0047179c - ref: undefined FUN_0047179c(undefined4 *
// param_1)
void FUN_0047179c_Stub(void *a0)
{
}
// STUB(0x00471af8, 19 bytes) FUN_00471af8 - ref: undefined FUN_00471af8(undefined4
// param_1, undefined4 param_2, undefined1 * param_3)
void FUN_00471af8_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x00471b0c, 22 bytes) FUN_00471b0c - ref: int FUN_00471b0c(int param_1, int
// param_2)
int FUN_00471b0c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471b24, 11 bytes) FUN_00471b24 - ref: undefined4 FUN_00471b24(int param_1)
int FUN_00471b24_Stub(int a0)
{
    return 0;
}
// STUB(0x00471b30, 81 bytes) FUN_00471b30 - ref: int FUN_00471b30(int param_1, int
// param_2)
int FUN_00471b30_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471b94, 205 bytes) FUN_00471b94 - ref: undefined FUN_00471b94(undefined4 *
// param_1)
void FUN_00471b94_Stub(void *a0)
{
}
// STUB(0x00471c64, 55 bytes) FUN_00471c64 - ref: int FUN_00471c64(int param_1, int
// param_2, int param_3)
int FUN_00471c64_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x00471c9c, 212 bytes) FUN_00471c9c - ref: undefined FUN_00471c9c(undefined4 *
// param_1)
void FUN_00471c9c_Stub(void *a0)
{
}
// STUB(0x00471d70, 26 bytes) FUN_00471d70 - ref: undefined4 FUN_00471d70(undefined4
// param_1)
int FUN_00471d70_Stub(int a0)
{
    return 0;
}
// STUB(0x00471d8c, 22 bytes) FUN_00471d8c - ref: int FUN_00471d8c(int param_1, int
// param_2)
int FUN_00471d8c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471da4, 26 bytes) FUN_00471da4 - ref: int FUN_00471da4(int param_1, int
// param_2)
int FUN_00471da4_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471dc0, 22 bytes) FUN_00471dc0 - ref: int FUN_00471dc0(int param_1, undefined4
// param_2)
int FUN_00471dc0_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471dd8, 124 bytes) FUN_00471dd8 - ref: undefined4 * FUN_00471dd8(undefined4 *
// param_1)
void *FUN_00471dd8_Stub(void *a0)
{
    return 0;
}
// STUB(0x00471e54, 78 bytes) FUN_00471e54 - ref: uint FUN_00471e54(void)
unsigned int FUN_00471e54_Stub()
{
    return 0;
}
// STUB(0x00471ea4, 107 bytes) FUN_00471ea4 - ref: int FUN_00471ea4(undefined4 param_1,
// int param_2)
int FUN_00471ea4_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471f10, 11 bytes) FUN_00471f10 - ref: undefined4 FUN_00471f10(int param_1)
int FUN_00471f10_Stub(int a0)
{
    return 0;
}
// STUB(0x00471f1c, 33 bytes) FUN_00471f1c - ref: undefined4 FUN_00471f1c(undefined4
// param_1)
int FUN_00471f1c_Stub(int a0)
{
    return 0;
}
// STUB(0x00471f40, 111 bytes) FUN_00471f40 - ref: int FUN_00471f40(undefined4 param_1,
// int param_2)
int FUN_00471f40_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00471fb0, 42 bytes) FUN_00471fb0 - ref: undefined4 * FUN_00471fb0(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00471fb0_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00471fec, 77 bytes) FUN_00471fec - ref: int FUN_00471fec(int param_1, undefined4
// param_2, int * param_3)
int FUN_00471fec_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x0047203c, 33 bytes) FUN_0047203c - ref: int FUN_0047203c(int param_1, int
// param_2, undefined4 * param_3)
int FUN_0047203c_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x00472060, 33 bytes) FUN_00472060 - ref: undefined FUN_00472060(undefined4
// param_1, undefined1 * param_2)
void FUN_00472060_Stub(int a0, void *a1)
{
}
// STUB(0x00472084, 8 bytes) FUN_00472084 - ref: undefined4 FUN_00472084(undefined4
// param_1, undefined4 param_2)
int FUN_00472084_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047208c, 5 bytes) FUN_0047208c - ref: undefined FUN_0047208c(void)
void FUN_0047208c_Stub()
{
}
// STUB(0x00472094, 22 bytes) FUN_00472094 - ref: undefined FUN_00472094(int param_1, int
// param_2)
void FUN_00472094_Stub(int a0, int a1)
{
}
// STUB(0x004720ac, 81 bytes) FUN_004720ac - ref: int FUN_004720ac(int param_1, int
// param_2)
int FUN_004720ac_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472100, 41 bytes) FUN_00472100 - ref: int FUN_00472100(int param_1, undefined4
// param_2)
int FUN_00472100_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047212c, 31 bytes) FUN_0047212c - ref: bool FUN_0047212c(int param_1, int
// param_2)
bool FUN_0047212c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047214c, 44 bytes) FUN_0047214c - ref: undefined4 * FUN_0047214c(undefined4 *
// param_1, undefined4 * param_2)
void *FUN_0047214c_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x00472178, 8 bytes) FUN_00472178 - ref: undefined4 FUN_00472178(undefined4
// param_1)
int FUN_00472178_Stub(int a0)
{
    return 0;
}
// STUB(0x00472180, 128 bytes) FUN_00472180 - ref: undefined4 * FUN_00472180(undefined4 *
// param_1, undefined4 * param_2)
void *FUN_00472180_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x00472200, 11 bytes) FUN_00472200 - ref: undefined4 FUN_00472200(int param_1)
int FUN_00472200_Stub(int a0)
{
    return 0;
}
// STUB(0x0047220c, 25 bytes) FUN_0047220c - ref: int FUN_0047220c(int param_1, undefined1
// * param_2)
int FUN_0047220c_Stub(int a0, void *a1)
{
    return 0;
}
// STUB(0x00472228, 23 bytes) FUN_00472228 - ref: int FUN_00472228(undefined4 param_1, int
// param_2)
int FUN_00472228_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472240, 54 bytes) FUN_00472240 - ref: int FUN_00472240(int param_1, int
// param_2)
int FUN_00472240_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472278, 170 bytes) FUN_00472278 - ref: int FUN_00472278(int param_1, int
// param_2)
int FUN_00472278_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472324, 74 bytes) FUN_00472324 - ref: undefined4 FUN_00472324(int param_1, int
// param_2)
int FUN_00472324_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472370, 72 bytes) FUN_00472370 - ref: int FUN_00472370(int param_1)
int FUN_00472370_Stub(int a0)
{
    return 0;
}
// STUB(0x004723b8, 91 bytes) FUN_004723b8 - ref: int FUN_004723b8(int param_1, int
// param_2)
int FUN_004723b8_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472414, 870 bytes) EO_Decode_Deinterleave - ref: void
// EO_Decode_Deinterleave(char * data, int len, char * out)
void EO_Decode_Deinterleave_Stub(void *a0, int a1, void *a2)
{
}
// STUB(0x00472810, 56 bytes) FUN_00472810 - ref: int FUN_00472810(int param_1, int
// param_2)
int FUN_00472810_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00472880, 19 bytes) FUN_00472880 - ref: undefined FUN_00472880(undefined4 *
// param_1, undefined1 * param_2)
void FUN_00472880_Stub(void *a0, void *a1)
{
}
// STUB(0x00472894, 14 bytes) FUN_00472894 - ref: undefined FUN_00472894(int param_1)
void FUN_00472894_Stub(int a0)
{
}
// STUB(0x004728a4, 14 bytes) FUN_004728a4 - ref: undefined FUN_004728a4(int param_1)
void FUN_004728a4_Stub(int a0)
{
}
// STUB(0x004728b4, 14 bytes) FUN_004728b4 - ref: undefined FUN_004728b4(undefined4 *
// param_1)
void FUN_004728b4_Stub(void *a0)
{
}
// STUB(0x004728f8, 74 bytes) FUN_004728f8 - ref: undefined FUN_004728f8(int param_1, int
// param_2)
void FUN_004728f8_Stub(int a0, int a1)
{
}
// STUB(0x00472944, 74 bytes) FUN_00472944 - ref: undefined FUN_00472944(int param_1, int
// param_2)
void FUN_00472944_Stub(int a0, int a1)
{
}
// STUB(0x00473124, 78 bytes) FUN_00473124 - ref: undefined4 FUN_00473124(undefined4
// param_1, int param_2, int param_3, int param_4, int param_5)
int FUN_00473124_Stub(int a0, int a1, int a2, int a3, int a4)
{
    return 0;
}
// STUB(0x004731d0, 878 bytes) FUN_004731d0 - ref: int * FUN_004731d0(AnsiString * out,
// Server * server)
void *FUN_004731d0_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x00473540, 878 bytes) FUN_00473540 - ref: int * FUN_00473540(AnsiString *
// param_1, Server * server)
void *FUN_00473540_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x004738b0, 110 bytes) FUN_004738b0 - ref: bool FUN_004738b0(undefined4 param_1,
// undefined4 param_2, undefined4 param_3, int param_4)
bool FUN_004738b0_Stub(int a0, int a1, int a2, int a3)
{
    return 0;
}
// STUB(0x00473920, 1051 bytes) FUN_00473920 - ref: undefined FUN_00473920(int * param_1)
void FUN_00473920_Stub(void *a0)
{
}
// STUB(0x00473f18, 67 bytes) FUN_00473f18 - ref: undefined FUN_00473f18(undefined4 *
// param_1, byte param_2)
void FUN_00473f18_Stub(void *a0, unsigned char a1)
{
}
// STUB(0x00474138, 33 bytes) FUN_00474138 - ref: undefined4 FUN_00474138(undefined4
// param_1)
int FUN_00474138_Stub(int a0)
{
    return 0;
}
// STUB(0x0047415c, 11 bytes) FUN_0047415c - ref: undefined4 FUN_0047415c(int param_1)
int FUN_0047415c_Stub(int a0)
{
    return 0;
}
// STUB(0x00474178, 14 bytes) FUN_00474178 - ref: undefined FUN_00474178(undefined4
// param_1, int param_2)
void FUN_00474178_Stub(int a0, int a1)
{
}
// STUB(0x00474300, 33 bytes) FUN_00474300 - ref: undefined4 FUN_00474300(undefined4
// param_1)
int FUN_00474300_Stub(int a0)
{
    return 0;
}
// STUB(0x00474324, 11 bytes) FUN_00474324 - ref: undefined4 FUN_00474324(int param_1)
int FUN_00474324_Stub(int a0)
{
    return 0;
}
// STUB(0x00474330, 14 bytes) FUN_00474330 - ref: undefined FUN_00474330(void)
void FUN_00474330_Stub()
{
}
// STUB(0x00474340, 14 bytes) FUN_00474340 - ref: undefined FUN_00474340(undefined4
// param_1, int * param_2)
void FUN_00474340_Stub(int a0, void *a1)
{
}
// STUB(0x00474568, 14 bytes) FUN_00474568 - ref: undefined FUN_00474568(void)
void FUN_00474568_Stub()
{
}
// STUB(0x00474578, 14 bytes) FUN_00474578 - ref: undefined FUN_00474578(undefined4
// param_1, int * param_2)
void FUN_00474578_Stub(int a0, void *a1)
{
}
#pragma warn.8057
// END GENERATED STUBS
