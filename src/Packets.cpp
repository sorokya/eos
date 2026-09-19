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
#include "Jukeboxcontrol.h"
#include "Msgboardcontrol.h"
#include "Protocol.h"

#pragma package(smart_init)

Player **Players_Iter_Begin(Players *players);
bool Player_IsPartyMember(Player *player, int player_id);
int RandRange(int range);
int Combat_CalcHitRate(void *game_control, int accuracy, int evasion, double, double);
int Combat_CalcArmorPen(void *game_control, int damage, int armor, double, double);
int Eif_GetElement(void *item_values, int item_id, int *out);
double
Combat_CalcElementMult(void *game_control, short a, short b, int element, int element2);
int Player_HpPercent(Player *player, int mode);
void Server_BroadcastToParty(Server *server,
                             Player *player,
                             unsigned char action,
                             unsigned char family,
                             String data);
Player **Players_Iter_End(Players *players);
bool Player_HandlePacket(Server *server, Player *player, String data);
void FUN_00472944(Server *server, int value);
void FUN_004728f8(Server *server, int value);
String Player_SerializePaperdoll(Server *server, Player *player);
String NpcRange_Lookup(Server *server, Player *player, unsigned int npc_index);
void Player_FireQuestTriggers(Server *server, Player *player, int state_index, int value);
MapCoord FUN_0047c6c0(int map_control, int map_id, unsigned int npc_index);
unsigned int FUN_0047c634(int map_control, int map_id, unsigned int npc_index);
bool Attack_Execute(Server *server, Player *caster, int action, String *reader);
bool Spell_Execute(Server *server, Player *caster, int action, String *packet_data);
String Player_SerializeAvatar(Server *server, Player *player, int arg);
String Server_BuildOnlineNames(Server *server);
String Server_BuildOnlineList(Server *server);
String Refresh_BuildReply(Server *server, Player *player);
String Map_ReadRawFile(Mapcontrol *map_control, int map_id);
bool Walk_Execute(Server *server, Player *player, int action, String *data);

bool Player_HandlePacket(Server *server, Player *player, String data)
{
    FUN_00472944(server, data.Length());
    if (data.Length() < 4)
        return false;
    player->packet_count++;
    player->sequence++;
    if (player->sequence > 9)
        player->sequence = 0;
    for (int i = 1; i < data.Length(); i++)
    {
        int c = (unsigned char)data[i];
        if (c >= 0x80)
            data[i] += (char)0x80;
        if (c > 0x80)
            data[i] += (char)0x80;
    }
    std::basic_string<char> range(data.c_str());
    data = EO_Decode_Deinterleave(server,
                                  player->client_encryption_multiple,
                                  (char *)range.end(),
                                  (char *)range.begin());
    int action = EO_DecodeByte((void *)server, data[1]);
    int family = EO_DecodeByte((void *)server, data[2]);
    int size = EO_DecodeNumber(server, String(data[3]));
    size -= player->sequence;
    data.Delete(1, 3);
    bool found = false;
    for (int i = 0; i < 3; i++)
        if (server->ping_history[i] == size)
            found = true;
    if (!found)
        return false;
    (*MAINFORM)->field_370 = family;
    (*MAINFORM)->field_36c = action;
    if (family == PacketFamily_Walk)
    {
        if (!player->logged_in)
            return false;
        TTimeStamp stamp = DateTimeToTimeStamp(Now());
        int delay = stamp.Time - player->walk_tick;
        if (delay > 0x7270e0)
            delay = 0x15e;
        if (delay < 0x15e)
        {
            if (player->action_queue.size() > 10)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        if (player->action_queue.size() > 0)
        {
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        return Walk_Execute(server, player, action, &data);
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
        if (delay > 0x7270e0)
            delay = 0x15e;
        if (delay < 0x15e)
        {
            if (player->action_queue.size() > 10)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        if (player->action_queue.size() > 0)
        {
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        return Attack_Execute(server, player, action, &data);
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
        if (delay > 0x7270e0)
            delay = 0x15e;
        if (delay < 0x15e)
        {
            if (player->action_queue.size() > 10)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        if (player->action_queue.size() > 0)
        {
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        return Spell_Execute(server, player, action, &data);
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
                               EO_EncodeNumber(server, 6, 2) + "NO");
            return false;
        }
        if (Login_CheckConnectionThreshold(server) &&
            !server->logins->ConnectionLog_CheckIP(player->socket->RemoteAddress))
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Login,
                               EO_EncodeNumber(server, 6, 2) + "NO");
            return false;
        }
        PacketReader_Init(server, data, EO_GetBreakByte(server, 0xff));
        String account = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String password = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        Mysqlcontrols::Mysql_SubmitQuery(
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
            Mysqlcontrols::Mysql_SubmitQuery(
                server->mysql_controls,
                0x42,
                player->player_id,
                player->query_id,
                data,
                "SELECT ident, account, DECODE(password,'eoeokeyendl') as password, "
                "type, signup, serial_c, serial_h, ipaddress, banned FROM "
                "endl_accounts WHERE ident = '" +
                    IntToStr(player->field_0xc) + "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Request)
        {
            if (!player->connected)
                return false;
            if (Settings::GetAccountLock(server->settings))
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Account,
                                   EO_EncodeNumber(server, 7, 2) + "NO");
                return false;
            }
            if (player->remove_timer > 0)
            {
                Client_SendEncoded(server,
                                   player,
                                   PacketAction_Reply,
                                   PacketFamily_Account,
                                   EO_EncodeNumber(server, 7, 2) + "NO");
                return false;
            }
            if (data.Length() < 4)
                return false;
            if (player->account_create_cooldown > 4)
                return true;
            player->account_create_cooldown = 6;
            Mysqlcontrols::Mysql_SubmitQuery(
                server->mysql_controls,
                0x43,
                player->player_id,
                player->query_id,
                data,
                "SELECT ident, account, DECODE(password,'eoeokeyendl') as password, "
                "type, signup, serial_c, serial_h, ipaddress, banned FROM "
                "endl_accounts WHERE account = '" +
                    Mysqlcontrols::Db_SanitizeString(server->mysql_controls, data) +
                    "' LIMIT 1");
            return true;
        }
        if (action == PacketAction_Create)
        {
            if (Settings::GetAccountLock(server->settings))
                return false;
            if (data.Length() < 2)
                return false;
            PacketReader_Init(server, data, EO_GetBreakByte(server, 0xff));
            PacketReader_GetBreakString(server);
            String account = Mysqlcontrols::Db_SanitizeString(
                server->mysql_controls, PacketReader_GetBreakString(server));
            Mysqlcontrols::Mysql_SubmitQuery(
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
    if (family == PacketFamily_Range && action == PacketAction_Request)
    {
        if (!player->logged_in)
            return false;
        if (data.Length() < 1)
            return false;
        PacketReader_Init(server, data, EO_GetBreakByte(server, 0xff));
        String players = PacketReader_GetBreakString(server);
        String npcs = PacketReader_GetBreakString(server);
        String out = EO_GetBreakByte(server, 0xff);
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
        String reply = EO_GetBreakByte(server, 0xff);
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
        if (delay > 0x7270e0)
            delay = 0x15e;
        if (delay < 0x15e)
        {
            if (player->action_queue.size() > 10)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        if (player->action_queue.size() > 0)
        {
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        return Face_Execute(server, player, action, &data);
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
        if ((unsigned int)EO_DecodeNumber(server, String(data[1])) > 10 &&
            EO_DecodeNumber(server, String(data[1])) != 14)
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
            int type = EO_DecodeNumber(server, String(data[1]));
            if (type == 1)
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
                if (!player->on_chair)
                {
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
                family = PacketFamily_Chair;
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
        if (delay > 0x7270e0)
            delay = 0x15e;
        if (delay < 0x15e)
        {
            if (player->action_queue.size() > 10)
                return true;
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        if (player->action_queue.size() > 0)
        {
            PlayerCommand command(family, action, data);
            player->action_queue.insert(player->action_queue.end(), command);
            return true;
        }
        return Chair_Execute(server, player, action, &data);
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
            int warp =
                Mapcontrol::Map_GetWarpDoorAt(server->map_control, player->map_id, coord);
            if (warp > 0)
            {
                if (Players::Player_HasKeyItem(server->players, player, warp))
                {
                    if (Mapcontrol::Mapcontrol_ToggleDoor(
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
            if (Mapcontrol::Mapcontrol_ToggleDoor(
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
                Mapcontrol::Mapcontrol_dec_player_count(server->map_control,
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
                server, player, Map_ReadRawFile(server->map_control, read_map), 4);
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
            if ((short)player->warp_x > 0xf0 || (short)player->warp_x < 0)
                return true;
            if ((short)player->warp_y > 0xf0 || (short)player->warp_y < 0)
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
                Mapcontrol::Mapcontrol_dec_player_count(server->map_control,
                                                        player->map_id);
            }
            int saved_state = player->warp_state;
            player->map_id = player->warp_map;
            player->x = (short)player->warp_x;
            player->y = (short)player->warp_y;
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
            Mapcontrol::Mapcontrol_inc_player_count(server->map_control, player->map_id);
            String out = EO_GetBreakByte(server, 0xff);
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
            ShopCraftIngredient ingredient1 = ShopValues::GetCraftIngredient1(
                (*MAINFORM)->shop_values, shop_id, craft_id);
            ShopCraftIngredient ingredient2 = ShopValues::GetCraftIngredient2(
                (*MAINFORM)->shop_values, shop_id, craft_id);
            ShopCraftIngredient ingredient3 = ShopValues::GetCraftIngredient3(
                (*MAINFORM)->shop_values, shop_id, craft_id);
            ShopCraftIngredient ingredient4 = ShopValues::GetCraftIngredient4(
                (*MAINFORM)->shop_values, shop_id, craft_id);
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
            player->weight_current +=
                ItemValues::Eif_GetWeight((*MAINFORM)->item_values, craft_id);
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
            int price = ShopValues::GetBuyPrice(
                (*MAINFORM)->shop_values, shop_id, item_id, amount);
            if (price < 0)
                return true;
            if (amount < 1)
                return true;
            if (!Players::Player_RemoveItem(server->players, player, 1, price))
                return true;
            Players::Player_AddItem(server->players, player, item_id, amount);
            player->weight_current +=
                ItemValues::Eif_GetWeight((*MAINFORM)->item_values, item_id) * amount;
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
                (*MAINFORM)->shop_values, shop_id, item_id, player->item_change_count);
            if (price < 0)
                return true;
            Players::Player_AddItem(server->players, player, 1, price);
            player->weight_current -=
                ItemValues::Eif_GetWeight((*MAINFORM)->item_values, item_id) *
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
            MapCoord coords =
                FUN_0047c6c0((int)server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return true;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id =
                (int)FUN_0047c634((int)server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType((*MAINFORM)->npc_values, npc_id);
            if (type_info.type != NpcType_Shop)
                return true;
            player->session_token = type_info.behavior_id;
            String open_data = ShopValues::BuildOpenData((*MAINFORM)->shop_values,
                                                         type_info.behavior_id);
            Client_SendEncoded(
                server, player, PacketAction_Open, PacketFamily_Shop, open_data);
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
            MsgBoardController::DeletePost((*MAINFORM)->msgboard_control, board, post_id);
            return true;
        }
        if (action == PacketAction_Create)
        {
            if (!player->logged_in)
                return false;
            if (data.Length() < 2)
                return false;
            int board = EO_DecodeNumber(server, data.SubString(1, 2));
            PacketReader_Init(server, data, EO_GetBreakByte(server, 0xff));
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
                    (*MAINFORM)->msgboard_control, board, player->name) > 1)
                return true;
            MsgBoardController::AddPost(
                (*MAINFORM)->msgboard_control, board, player->name, subject, message, 0);
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
            String post = MsgBoardController::GetPost(
                (*MAINFORM)->msgboard_control, board, post_id);
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
                MsgBoardController::GetBoard((*MAINFORM)->msgboard_control, board + 1);
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
            MapCoord coords =
                FUN_0047c6c0((int)server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return false;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id =
                (int)FUN_0047c634((int)server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType((*MAINFORM)->npc_values, npc_id);
            if (type_info.type != NpcType_Barber)
                return true;
            player->session_token = RandRange(0x2710) + 0x30d41;
            Client_SendEncoded(server,
                               player,
                               PacketAction_Open,
                               PacketFamily_Barber,
                               EO_EncodeNumber(server, player->session_token, 4));
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
            MapCoord coords =
                FUN_0047c6c0((int)server->map_control, player->map_id, npc_index);
            if (coords.x < 0 || coords.y < 0)
                return false;
            if (!Server_InViewRange(server, coords.x, coords.y, player->x, player->y))
                return true;
            int npc_id =
                (int)FUN_0047c634((int)server->map_control, player->map_id, npc_index);
            NpcTypeInfo type_info = NpcValues::GetType((*MAINFORM)->npc_values, npc_id);
            if (type_info.type != NpcType_Bank)
                return true;
            player->session_token = RandRange(0x2710) + 0x186a1;
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
                (*MAINFORM)->jukebox_control, player->map_id);
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
                    (*MAINFORM)->jukebox_control, player->map_id, player->name))
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
                for (PlayerQuest *iter = player->quest_trackers.begin();
                     iter != player->quest_trackers.end();
                     iter++)
                {
                    QuestState *state = Questengine::GetState(
                        server->quest_engine, iter->quest_id, iter->state_index);
                    if (state != NULL)
                    {
                        out.Insert(Questengine::GetQuestName(server->quest_engine,
                                                             iter->quest_id),
                                   out.Length() + 1);
                        out.Insert(String((char)EO_GetBreakByte(server, 0xff)),
                                   out.Length() + 1);
                        out.Insert(state->description, out.Length() + 1);
                        out.Insert(String((char)EO_GetBreakByte(server, 0xff)),
                                   out.Length() + 1);
                        int cond = state->fast_dispatch_condition_type;
                        int v1 = 0;
                        int v2 = 0;
                        if (state->fast_dispatch_rule_index < 5 &&
                            state->fast_dispatch_condition_type > 0 &&
                            state->fast_dispatch_condition_type < 10)
                        {
                            v2 = iter->counters[state->fast_dispatch_rule_index];
                            QuestRule *rule =
                                state->rules[state->fast_dispatch_rule_index];
                            v1 = rule->args[1];
                            if (state->fast_dispatch_condition_type == 9)
                                v1 = rule->args[0];
                            if (v1 < 1)
                                v1 = 1;
                        }
                        out.Insert(EO_EncodeNumber(server, cond, 2), out.Length() + 1);
                        out.Insert(EO_EncodeNumber(server, v2, 2), out.Length() + 1);
                        out.Insert(EO_EncodeNumber(server, v1, 2), out.Length() + 1);
                        out.Insert(String((char)EO_GetBreakByte(server, 0xff)),
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
                out.Insert(EO_EncodeNumber(server, player->quest_history.size(), 2),
                           out.Length() + 1);
                for (PlayerQuest *iter = player->quest_history.begin();
                     iter != player->quest_history.end();
                     iter++)
                {
                    out.Insert(
                        Questengine::GetQuestName(server->quest_engine, iter->quest_id),
                        out.Length() + 1);
                    out.Insert(String((char)EO_GetBreakByte(server, 0xff)),
                               out.Length() + 1);
                }
                Client_SendEncoded(
                    server, player, PacketAction_List, PacketFamily_Quest, out);
            }
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
            msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
            msg.Insert(player->name, msg.Length() + 1);
            msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
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
            PacketReader_Init(server, data, EO_GetBreakByte(server, 0xff));
            String s1 = PacketReader_GetBreakString(server);
            String s2 = PacketReader_GetBreakString(server);
            String msg = EO_EncodeNumber(server, 2, 2);
            msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
            msg.Insert(player->name, msg.Length() + 1);
            msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
            msg.Insert(s2, msg.Length() + 1);
            msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
            msg.Insert(s1, msg.Length() + 1);
            Admin_BroadcastToAll(
                server, PacketAction_Reply, PacketFamily_AdminInteract, msg);
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

bool FUN_00462374(Server *server, Player *player, String data);
String Character_BuildSaveQuery(Players *players, Player *player, int flag);
MapObject Map_GetTileSpecObject(Mapcontrol *map_control, int map_id, int x, int y);
void Server_BroadcastToMapExceptSelf(Server *server,
                                     Player *player,
                                     unsigned char action,
                                     unsigned char family,
                                     String data);

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

void *Map_NpcIter_Begin(void *npc_list)
{
    return *(void **)((char *)npc_list + 0x04);
}

void *Map_NpcIter_End(void *npc_list)
{
    return *(void **)((char *)npc_list + 0x08);
}

void *GroundItemPtrVector_Begin(void *list)
{
    return *(void **)((char *)list + 0x04);
}

void *PtrVector_GetEnd(void *list)
{
    return *(void **)((char *)list + 0x08);
}

int GroundItemPtrVector_Count(void *list)
{
    return (void **)PtrVector_GetEnd(list) - (void **)GroundItemPtrVector_Begin(list);
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

void Player_ApplyQuestActions(Server *server,
                              Player *player,
                              PlayerQuest *tracker,
                              bool flag);

void Player_EvaluateQuestRules(Server *server,
                               Player *player,
                               PlayerQuest *tracker,
                               QuestState *state,
                               int event,
                               int arg)
{
    std::vector<QuestRule *>::iterator iter;
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
                }
            }
            if ((*iter)->rule == 8)
            {
                if (index <= 4)
                {
                    int a1 = (*iter)->args[0];
                    int a2 = (*iter)->args[1];
                    if (arg == a1)
                    {
                        tracker->counters[index]++;
                        if (tracker->counters[index] >= a2)
                        {
                            tracker->state_index = *(short *)&(*iter)->goto_state_index;
                            Player_ApplyQuestActions(server, player, tracker, true);
                            return;
                        }
                    }
                }
            }
            if ((*iter)->rule == 9)
            {
                if (index <= 4)
                {
                    int a1 = (*iter)->args[0];
                    tracker->counters[index]++;
                    if (tracker->counters[index] >= a1)
                    {
                        tracker->state_index = *(short *)&(*iter)->goto_state_index;
                        Player_ApplyQuestActions(server, player, tracker, true);
                        return;
                    }
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

void Player_CalculateStats(Server *server, Player *player)
{
    if (player->class_id < 1)
        return;
    ClassValue cls =
        ClassValues::GetByIndex((*MAINFORM)->class_values, player->class_id - 1);
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

void Player_ApplyEquipmentBonuses(Server *server, Player *player)
{
    player->equip_bonus_hp = 0;
    player->equip_bonus_tp = 0;
    player->equip_strength_bonus = 0;
    player->equip_wisdom_bonus = 0;
    player->equip_intelligence_bonus = 0;
    player->equip_agility_bonus = 0;
    player->equip_constitution_bonus = 0;
    player->equip_charisma_bonus = 0;
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->boots_item_id) ==
        ItemType_Boots)
    {
        ItemValue *boots =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->boots_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->accessory_item_id) ==
        ItemType_Accessory)
    {
        ItemValue *accessory = ItemValues::GetByIndex((*MAINFORM)->item_values,
                                                      player->accessory_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->gloves_item_id) ==
        ItemType_Gloves)
    {
        ItemValue *gloves =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->gloves_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->armor_item_id) ==
        ItemType_Armor)
    {
        ItemValue *armor =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->armor_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->belt_item_id) ==
        ItemType_Belt)
    {
        ItemValue *belt =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->belt_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->necklace_item_id) ==
        ItemType_Necklace)
    {
        ItemValue *necklace = ItemValues::GetByIndex((*MAINFORM)->item_values,
                                                     player->necklace_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->hat_item_id) ==
        ItemType_Hat)
    {
        ItemValue *hat =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->hat_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->shield_item_id) ==
        ItemType_Shield)
    {
        ItemValue *shield =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->shield_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->weapon_item_id) ==
        ItemType_Weapon)
    {
        ItemValue *weapon =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->weapon_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->ring1_item_id) ==
        ItemType_Ring)
    {
        ItemValue *ring1 =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->ring1_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->ring2_item_id) ==
        ItemType_Ring)
    {
        ItemValue *ring2 =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->ring2_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->armlet1_item_id) ==
        ItemType_Armlet)
    {
        ItemValue *armlet1 =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->armlet1_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->armlet2_item_id) ==
        ItemType_Armlet)
    {
        ItemValue *armlet2 =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->armlet2_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->bracer1_item_id) ==
        ItemType_Bracer)
    {
        ItemValue *bracer1 =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->bracer1_item_id - 1);
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
    if (ItemValues::Eif_GetType((*MAINFORM)->item_values, player->bracer2_item_id) ==
        ItemType_Bracer)
    {
        ItemValue *bracer2 =
            ItemValues::GetByIndex((*MAINFORM)->item_values, player->bracer2_item_id - 1);
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

String Server_BuildOnlineList(Server *server)
{
    if (server->online_list_ttl < 1)
    {
        String list = EO_GetBreakByte(server, 0xff);
        int count = 0;
        for (Player **iter = Players_Iter_Begin(server->players);
             iter != Players_Iter_End(server->players);
             iter++)
        {
            if ((*iter)->logged_in && !(*iter)->hide_online)
            {
                list.Insert((*iter)->name, list.Length() + 1);
                list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
                list.Insert((*iter)->title, list.Length() + 1);
                list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
                list.Insert(EO_EncodeNumber(server, (*iter)->level, 1),
                            list.Length() + 1);
                if ((*iter)->in_party)
                {
                    if ((*iter)->admin_level > 1)
                    {
                        if ((*iter)->admin_level < 4)
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
                    if ((*iter)->admin_level > 1)
                    {
                        if ((*iter)->admin_level < 4)
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
                list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
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

String Refresh_BuildReply(Server *server, Player *player)
{
    String data = EO_GetBreakByte(server, 0xff);
    int count = 0;
    Player **iter;
    try
    {
        for (iter = Players_Iter_Begin(server->players);
             iter != Players_Iter_End(server->players);
             iter++)
        {
            if ((*iter)->map_id == player->map_id &&
                Server_InViewRange(server, player->x, player->y, (*iter)->x, (*iter)->y))
            {
                count++;
                data.Insert((*iter)->name, data.Length() + 1);
                data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
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
                data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
            }
        }
        data.Insert(EO_EncodeNumber(server, count, 1), 1);
        Npc **niter;
        if (player->map_id > 0 &&
            player->map_id <= Mapcontrol_GetCount(server->map_control))
        {
            for (niter = (Npc **)Map_NpcIter_Begin(
                     &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                          ->npc_list);
                 niter !=
                 (Npc **)Map_NpcIter_End(
                     &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                          ->npc_list);
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
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
        ChestItem **iiter;
        if (player->map_id > 0 &&
            player->map_id <= Mapcontrol_GetCount(server->map_control))
        {
            for (iiter = (ChestItem **)GroundItemPtrVector_Begin(
                     &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                          ->ground_items);
                 iiter !=
                 (ChestItem **)PtrVector_GetEnd(
                     &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                          ->ground_items);
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
        data = "";
    }
    return data;
}

String Paperdoll_BuildReply(Server *server, Player *player)
{
    String data = "";
    try
    {
        data.Insert(player->name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
        data.Insert(player->home_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
        data.Insert(player->partner_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
        data.Insert(player->title, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
        data.Insert(player->guild_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
        data.Insert(player->guild_rank_name, data.Length() + 1);
        data.Insert(EO_GetBreakByte(server, 0xff), data.Length() + 1);
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
            if (player->admin_level > 1)
            {
                if (player->admin_level < 4)
                    data.Insert(EO_EncodeNumber(server, 9, 1), data.Length() + 1);
                else
                    data.Insert(EO_EncodeNumber(server, 10, 1), data.Length() + 1);
            }
            else
                data.Insert(EO_EncodeNumber(server, 6, 1), data.Length() + 1);
        }
        else
        {
            if (player->admin_level > 1)
            {
                if (player->admin_level < 4)
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

String Player_SerializePaperdoll(Server *server, Player *player)
{
    String out = "";
    try
    {
        std::vector<PlayerQuest>::iterator it;
        out.Insert(player->name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        out.Insert(player->home_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        out.Insert(player->partner_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        out.Insert(player->title, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        out.Insert(player->guild_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        out.Insert(player->guild_rank_name, out.Length() + 1);
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->player_id, 2), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->class_id, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->gender, 1), out.Length() + 1);
        out.Insert(EO_EncodeNumber(server, player->admin_level, 1), out.Length() + 1);
        if (player->in_party)
        {
            if (player->admin_level > 1)
            {
                if (player->admin_level < 4)
                    out.Insert(EO_EncodeNumber(server, 9, 1), out.Length() + 1);
                else
                    out.Insert(EO_EncodeNumber(server, 10, 1), out.Length() + 1);
            }
            else
                out.Insert(EO_EncodeNumber(server, 6, 1), out.Length() + 1);
        }
        else
        {
            if (player->admin_level > 1)
            {
                if (player->admin_level < 4)
                    out.Insert(EO_EncodeNumber(server, 4, 1), out.Length() + 1);
                else
                    out.Insert(EO_EncodeNumber(server, 5, 1), out.Length() + 1);
            }
            else
                out.Insert(EO_EncodeNumber(server, 1, 1), out.Length() + 1);
        }
        out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        for (it = player->quest_history.begin(); it != player->quest_history.end(); it++)
        {
            out.Insert(Questengine::GetQuestName(server->quest_engine, it->quest_id),
                       out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return out;
}

String Player_SerializeAvatar(Server *server, Player *player, int arg)
{
    String out = player->name;
    out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
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
            out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        else
        {
            out.Insert(EO_EncodeNumber(server, arg, 1), out.Length() + 1);
            out.Insert(EO_GetBreakByte(server, 0xff), out.Length() + 1);
        }
    }
    catch (...)
    {
    }
    return out;
}

String Walk_BuildReply(Server *server, Player *player)
{
    String buf = "";
    try
    {
        Player **iter;
        Npc **niter;
        ChestItem **iiter;
        for (iter = Players_Iter_Begin(server->players);
             iter != Players_Iter_End(server->players);
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
        buf.Insert(EO_GetBreakByte(server, 0xff), buf.Length() + 1);
        if (player->map_id > 0)
        {
            if (player->map_id <= Mapcontrol_GetCount(server->map_control))
            {
                for (niter = (Npc **)Map_NpcIter_Begin(
                         &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                              ->npc_list);
                     niter !=
                     (Npc **)Map_NpcIter_End(
                         &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                              ->npc_list);
                     niter++)
                {
                    if (Server_InViewRing(
                            server, player->x, player->y, (*niter)->x, (*niter)->y))
                        buf.Insert(EO_EncodeNumber(server, (*niter)->index, 1),
                                   buf.Length() + 1);
                }
            }
        }
        buf.Insert(EO_GetBreakByte(server, 0xff), buf.Length() + 1);
        if (player->map_id > 0)
        {
            if (player->map_id <= Mapcontrol_GetCount(server->map_control))
            {
                for (iiter = (ChestItem **)GroundItemPtrVector_Begin(
                         &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                              ->ground_items);
                     iiter !=
                     (ChestItem **)PtrVector_GetEnd(
                         &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                              ->ground_items);
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
        buf += "";
    }
    return buf;
}

String Message_BuildServerStatus(Server *server)
{
    String names = EO_GetBreakByte(server, 0xff);
    int count = 0;
    for (Player **iter = Players_Iter_Begin(server->players);
         iter != Players_Iter_End(server->players);
         iter++)
    {
        if ((*iter)->logged_in && !(*iter)->hide_online)
        {
            names.Insert((*iter)->name, names.Length() + 1);
            names.Insert(EO_GetBreakByte(server, 0xff), names.Length() + 1);
            names.Insert((*iter)->title, names.Length() + 1);
            names.Insert(EO_GetBreakByte(server, 0xff), names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->level, 1), names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->experience, 4),
                         names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->gender, 1), names.Length() + 1);
            names.Insert(EO_EncodeNumber(server, (*iter)->admin_level, 1),
                         names.Length() + 1);
            names.Insert(EO_GetBreakByte(server, 0xff), names.Length() + 1);
            count++;
        }
    }
    names.Insert(EO_EncodeNumber(server, count, 2), 1);
    return names;
}

String Server_BuildOnlineNames(Server *server)
{
    if (server->online_names_ttl < 1)
    {
        String names = EO_GetBreakByte(server, 0xff);
        int count = 0;
        for (Player **iter = Players_Iter_Begin(server->players);
             iter != Players_Iter_End(server->players);
             iter++)
        {
            if ((*iter)->logged_in && !(*iter)->hide_online)
            {
                names.Insert((*iter)->name, names.Length() + 1);
                names.Insert(EO_GetBreakByte(server, 0xff), names.Length() + 1);
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
    return server->online_names_cache;
}

String NpcRange_Lookup(Server *server, Player *player, unsigned int npc_index)
{
    String fragment = "";
    Npc **iter;
    try
    {
        if (player->map_id > 0)
        {
            if (player->map_id <= Mapcontrol_GetCount(server->map_control))
            {
                for (iter = (Npc **)Map_NpcIter_Begin(
                         &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                              ->npc_list);
                     iter !=
                     (Npc **)Map_NpcIter_End(
                         &Mapcontrol_GetByIndex(server->map_control, player->map_id - 1)
                              ->npc_list);
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

String Party_EncodeMemberList(Server *server, Player *player)
{
    if (!player->in_party)
        return "";
    String s = "";
    for (int i = 0; i < 10; i++)
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
            s.Insert(EO_GetBreakByte(server, 0xff), s.Length() + 1);
        }
    }
    return s;
}

void FUN_00466840(Server *server, int map_id)
{
    for (Player **iter = Players_Iter_Begin(server->players);
         iter != Players_Iter_End(server->players);
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

int Party_ShareExp(Server *server, Player *player, int exp)
{
    if (!player->in_party)
        return exp;
    if (player->CountPartyMembers() < 2)
        return exp;
    int members = 0;
    for (int i = 0; i < 10; i++)
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
    for (int i = 0; i < 10; i++)
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
        Server_BroadcastToMapExceptSelf(
            server, player, PacketAction_TargetGroup, PacketFamily_Party, pkt);
    return exp;
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
                              MapCoord coord,
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
            Server_InViewRange(
                server, coord.x, coord.y, (*player_iter)->x, (*player_iter)->y))
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
String FUN_0047060c(Server *server, String value);
String FUN_004708d4(Server *server, String value);
void Login_SendCharacterList(Server *server,
                             Player *player,
                             PacketAction action,
                             PacketFamily family,
                             String data);

void MysqlCallback_Dispatch(Server *server, mySQLtask *query_result)
{
    Player *player = Players::Players_GetById(server->players, query_result->player_id);
    if (player == NULL)
        return;
    if (player->query_id != query_result->expected_query_id)
        return;
    if (query_result->query_id == 0x40)
    {
        PacketReader_Init(server, query_result->data, EO_GetBreakByte(server, 0xff));
        String account = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String password = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        if ((*MAINFORM)->myquery->RecordCount < 1)
        {
            Client_SendEncoded(
                server, player, 3, 4, EO_EncodeNumber(server, 1, 2) + "NO");
            return;
        }
        if (password !=
            FUN_0047060c(server,
                         Mysqlcontrols::Db_GetString(server->mysql_controls, "password")))
        {
            Client_SendEncoded(
                server, player, 3, 4, EO_EncodeNumber(server, 2, 2) + "NO");
            return;
        }
        if ((unsigned int)Mysqlcontrols::Db_GetInt(server->mysql_controls, "banned") > 0)
        {
            Client_SendEncoded(
                server, player, 3, 4, EO_EncodeNumber(server, 4, 2) + "NO");
            player->removing = true;
            return;
        }
        int ident = Mysqlcontrols::Db_GetInt(server->mysql_controls, "ident");
        String account_name =
            Mysqlcontrols::Db_GetString(server->mysql_controls, "account");
        String account_type = Mysqlcontrols::Db_GetString(server->mysql_controls, "type");
        if (Mysqlcontrols::IsTaskPending(server->mysql_controls, ident))
        {
            Client_SendEncoded(
                server, player, 3, 4, EO_EncodeNumber(server, 5, 2) + "NO");
            return;
        }
        if (Players::Players_HasField0C(server->players, ident))
        {
            Client_SendEncoded(
                server, player, 3, 4, EO_EncodeNumber(server, 5, 2) + "NO");
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
        player->field_0xc = ident;
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
        Mysqlcontrols::Mysql_ExecDirect_FromCallback(
            server->mysql_controls,
            player->field_0xc,
            "UPDATE endl_accounts SET lastvisit = '" + String(now) +
                "' WHERE ident = " + IntToStr(ident));
        Mysqlcontrols::Mysql_SubmitQuery_FromCallback(
            server->mysql_controls,
            0x41,
            player->player_id,
            player->query_id,
            "",
            "SELECT * FROM endl_characters WHERE ident_account = " +
                IntToStr(player->field_0xc) + " ORDER BY level DESC LIMIT 3");
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
        if ((*MAINFORM)->myquery->RecordCount > 0)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Character,
                               EO_EncodeNumber(server, 1, 2) + "NO");
            return;
        }
        int gender = EO_DecodeNumber(server, query_result->data.SubString(3, 2));
        int hair_modal = EO_DecodeNumber(server, query_result->data.SubString(5, 2));
        int hair_color = EO_DecodeNumber(server, query_result->data.SubString(7, 2));
        int skin_color = EO_DecodeNumber(server, query_result->data.SubString(9, 2));
        String name = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls,
            PacketReader_GetBreakStringAt(
                server, 2, query_result->data, EO_GetBreakByte(server, 0xff)));
        TDateTime now = Now();
        String sql =
            "INSERT INTO endl_characters (ident_account, ident_guild, ident_class, "
            "name, signup, gender, hairmodal, haircolor, skincolor, nav_map, nav_x,"
            " nav_y, hp_max, hp_now, mp_max, mp_now, sp_max, clientusge, money_bank ) "
            "VALUES (";
        sql = sql + IntToStr(player->field_0xc) + ",";
        sql = sql + "'0',1,'";
        sql = sql + name + "',";
        sql = sql + "'" + String(now) + "',";
        sql = sql + IntToStr(gender) + ",";
        sql = sql + IntToStr(hair_modal) + ",";
        sql = sql + IntToStr(hair_color) + ",";
        sql = sql + IntToStr(skin_color) + ",";
        sql = sql + IntToStr(Settings::GetStartMap(server->settings)) + ",";
        sql = sql + IntToStr(Settings::GetStartX(server->settings)) + ",";
        sql = sql + IntToStr(Settings::GetStartY(server->settings)) + ",";
        sql = sql + "10,10,10,10,20,0,0)";
        Mysqlcontrols::Mysql_ExecDirect_FromCallback(
            server->mysql_controls, player->field_0xc, sql);
        Mysqlcontrols::Mysql_SubmitQuery_FromCallback(
            server->mysql_controls,
            0x46,
            player->player_id,
            player->query_id,
            "",
            "SELECT * FROM endl_characters WHERE ident_account = " +
                IntToStr(player->field_0xc) + " ORDER BY level DESC LIMIT 3");
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
        if ((*MAINFORM)->myquery->RecordCount > 0)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Account,
                               EO_EncodeNumber(server, 1, 2) + "NO");
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
        if ((*MAINFORM)->myquery->RecordCount > 0)
        {
            player->removing = true;
            return;
        }
        PacketReader_Init(server, query_result->data, EO_GetBreakByte(server, 0xff));
        PacketReader_GetBreakString(server);
        int code = EO_DecodeNumber(server, query_result->data.SubString(1, 2));
        String account = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        if (player->session_id != code)
        {
            player->removing = true;
            return;
        }
        if (player->account_create_cooldown > 4)
            return;
        player->account_create_cooldown = 6;
        String password = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String realname = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String location = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String email = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String serial_c = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String serial_h = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        TDateTime now = Now();
        String sql = "INSERT INTO endl_accounts (account, password, realname, location, "
                     "email,  signup, lastvisit, serial_c, serial_h , ipaddress, banned) "
                     "VALUES (";
        sql = sql + "'" + account + "',";
        sql = sql + "ENCODE('" + FUN_004708d4(server, password) + "','eoeokeyendl'),";
        sql = sql + "'" + realname + "',";
        sql = sql + "'" + location + "',";
        sql = sql + "'" + email + "',";
        sql = sql + "'" + String(now) + "',";
        sql = sql + "'" + String(now) + "',";
        sql = sql + "'" + serial_c + "',";
        sql = sql + "'" + serial_h + "',";
        sql = sql + "'" + player->socket->RemoteAddress + "',";
        sql = sql + "0)";
        Mysqlcontrols::Mysql_ExecDirect(server->mysql_controls, 0, sql);
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Account,
                           EO_EncodeNumber(server, 3, 2) + "GO");
        server->mysql_controls->file_cache->accounts_count++;
        player->session_id = RandRange(50000) + 10000;
        return;
    }
    if (query_result->query_id == 0x42)
    {
        PacketReader_Init(server, query_result->data, EO_GetBreakByte(server, 0xff));
        String account = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String old_password = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String new_password = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        if ((*MAINFORM)->myquery->RecordCount < 1)
        {
            player->removing = true;
            return;
        }
        if (FUN_0047060c(server,
                         Mysqlcontrols::Db_GetString(server->mysql_controls,
                                                     "password")) != old_password ||
            Mysqlcontrols::Db_GetString(server->mysql_controls, "account") != account)
        {
            Client_SendEncoded(server,
                               player,
                               PacketAction_Reply,
                               PacketFamily_Account,
                               EO_EncodeNumber(server, 5, 2) + "NO");
            return;
        }
        Client_SendEncoded(server,
                           player,
                           PacketAction_Reply,
                           PacketFamily_Account,
                           EO_EncodeNumber(server, 6, 2) + "OK");
        Mysqlcontrols::Mysql_ExecDirect(
            server->mysql_controls,
            player->field_0xc,
            "UPDATE endl_accounts SET password = ENCODE('" +
                FUN_004708d4(server, new_password) +
                "','eoeokeyendl') WHERE ident = " + IntToStr(player->field_0xc));
        return;
    }
    if (query_result->query_id == 0x47)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        int token = EO_DecodeNumber(server, query_result->data.SubString(5, 1));
        String char_name =
            query_result->data.SubString(6, query_result->data.Length() - 5);
        String rank_label = "rank" + IntToStr(token);
        String rank_value =
            Mysqlcontrols::Db_GetString(server->mysql_controls, rank_label);
        Player *target = Players::Players_FindByName(server->players, char_name);
        rank_value = Mysqlcontrols::Mysql_SanitizeString(
            server->mysql_controls, rank_value, false);
        if (target == NULL)
        {
            player->field_0x14 = rank_value;
            Mysqlcontrols::Mysql_SubmitQuery_FromCallback(
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
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x18, 2));
            return;
        }
        if (target->guild_rank_id == 1)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x17, 2));
            return;
        }
        target->guild_rank_name = rank_value;
        target->guild_rank_id = token;
        Client_SendEncoded(
            server, target, 2, 0x27, EO_EncodeNumber(server, token, 1) + rank_value);
        Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x12, 2));
        return;
    }
    if (query_result->query_id == 0x48)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        int token = EO_DecodeNumber(server, query_result->data.SubString(5, 1));
        String char_name =
            query_result->data.SubString(6, query_result->data.Length() - 5);
        if (AnsiLowerCase(
                Mysqlcontrols::Db_GetString(server->mysql_controls, "ident_guild")) !=
            AnsiLowerCase(player->guild_tag))
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x18, 2));
            return;
        }
        if ((unsigned int)Mysqlcontrols::Db_GetInt(server->mysql_controls,
                                                   "ident_rank") == 1)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x17, 2));
            return;
        }
        Mysqlcontrols::Mysql_ExecDirect_FromCallback(
            server->mysql_controls,
            player->field_0xc,
            "UPDATE endl_characters SET ident_rank = " + IntToStr(token) + ", rank = '" +
                player->field_0x14 + "' WHERE name = '" + char_name +
                "' AND ident_guild = '" + player->guild_tag + "'");
        Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x12, 2));
        return;
    }
    if (query_result->query_id == 0x49)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        if (AnsiLowerCase(
                Mysqlcontrols::Db_GetString(server->mysql_controls, "ident_guild")) !=
            AnsiLowerCase(player->guild_tag))
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x14, 2));
            return;
        }
        if ((unsigned int)Mysqlcontrols::Db_GetInt(server->mysql_controls,
                                                   "ident_rank") == 1)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x15, 2));
            return;
        }
        String char_name =
            query_result->data.SubString(5, query_result->data.Length() - 4);
        Mysqlcontrols::Mysql_ExecDirect_FromCallback(
            server->mysql_controls,
            player->field_0xc,
            "UPDATE endl_characters SET ident_guild = '0', ident_rank = 9, "
            "guild = '', rank = '' WHERE name = '" +
                char_name + "' AND ident_guild = '" + player->guild_tag + "'");
        Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x16, 2));
        return;
    }
    if (query_result->query_id == 0x4a)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        String description =
            Mysqlcontrols::Db_GetString(server->mysql_controls, "description");
        if (description.Length() == 0)
            description = " ";
        Client_SendEncoded(server, player, 9, 0x27, description);
        return;
    }
    if (query_result->query_id == 0x4b)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        String ranks = Mysqlcontrols::Db_GetString(server->mysql_controls, "rank1");
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank2"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank3"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank4"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank5"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank6"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank7"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank8"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        ranks.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank9"),
                     ranks.Length() + 1);
        ranks.Insert(EO_GetBreakByte(server, 0xff), ranks.Length() + 1);
        Client_SendEncoded(server, player, 0x1d, 0x27, ranks);
        return;
    }
    if (query_result->query_id == 0x4c)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        Client_SendEncoded(
            server,
            player,
            12,
            0x27,
            EO_EncodeNumber(
                server, Mysqlcontrols::Db_GetInt(server->mysql_controls, "money"), 4));
        return;
    }
    if (query_result->query_id == 0x4d)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x11, 2));
            return;
        }
        String list = EO_EncodeNumber(server, (*MAINFORM)->myquery->RecordCount, 2);
        list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
        while (!(*MAINFORM)->myquery->Eof)
        {
            list.Insert(EO_EncodeNumber(server,
                                        Mysqlcontrols::Db_GetInt(server->mysql_controls,
                                                                 "ident_rank"),
                                        1),
                        list.Length() + 1);
            list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
            list.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "name"),
                        list.Length() + 1);
            list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
            list.Insert(Mysqlcontrols::Db_GetString(server->mysql_controls, "rank"),
                        list.Length() + 1);
            list.Insert(EO_GetBreakByte(server, 0xff), list.Length() + 1);
            (*MAINFORM)->myquery->Next();
        }
        Client_SendEncoded(server, player, 0x14, 0x27, list);
        return;
    }
    if (query_result->query_id == 0x4e)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x11, 2));
            return;
        }
        String type = "bankrupt";
        int money = Mysqlcontrols::Db_GetInt(server->mysql_controls, "money");
        String tag = Mysqlcontrols::Db_GetString(server->mysql_controls, "tag");
        if (money >= 0x7d0)
            type = "poor";
        if (money >= 0x2710)
            type = "normal";
        if (money >= 0xc350)
            type = "wealthy";
        if (money >= 0x186a0)
            type = "very wealthy";
        player->field_0x14 = Mysqlcontrols::Db_GetString(server->mysql_controls, "name");
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "tag"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "signup"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "description"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(type, player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank1"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank2"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank3"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank4"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank5"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank6"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank7"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank8"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        player->field_0x14.Insert(
            Mysqlcontrols::Db_GetString(server->mysql_controls, "rank9"),
            player->field_0x14.Length() + 1);
        player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                  player->field_0x14.Length() + 1);
        Mysqlcontrols::Mysql_SubmitQuery_FromCallback(
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
        if ((*MAINFORM)->myquery->RecordCount < 1)
            has_result = false;
        if (has_result)
        {
            player->field_0x14.Insert(
                EO_EncodeNumber(
                    server, Mysqlcontrols::GetResultCount(server->mysql_controls), 2),
                player->field_0x14.Length() + 1);
            player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                      player->field_0x14.Length() + 1);
            while (!Mysqlcontrols::ResultAtEnd(server->mysql_controls))
            {
                player->field_0x14.Insert(
                    EO_EncodeNumber(
                        server,
                        Mysqlcontrols::Db_GetInt(server->mysql_controls, "ident_rank"),
                        1),
                    player->field_0x14.Length() + 1);
                player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                          player->field_0x14.Length() + 1);
                player->field_0x14.Insert(
                    Mysqlcontrols::Db_GetString(server->mysql_controls, "name"),
                    player->field_0x14.Length() + 1);
                player->field_0x14.Insert(EO_GetBreakByte(server, 0xff),
                                          player->field_0x14.Length() + 1);
                Mysqlcontrols::NextResultRecord(server->mysql_controls);
            }
        }
        Client_SendEncoded(server, player, 0x15, 0x27, player->field_0x14);
        return;
    }
    if (query_result->query_id == 0x50)
    {
        if (Mysqlcontrols::GetResultCount(server->mysql_controls) > 0)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 5, 2));
            return;
        }
        PacketReader_Init(server, query_result->data, EO_GetBreakByte(server, 0xff));
        PacketReader_GetBreakString(server);
        String guild = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String name = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        player->guild_inviter_id = player->player_id;
        Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 6, 2));
        String msg = EO_EncodeNumber(server, player->player_id, 2);
        msg.Insert(name + " (" + guild + ")", msg.Length() + 1);
        Server_BroadcastToMap(server, player->map_id, 1, 0x27, msg);
        return;
    }
    if (query_result->query_id == 0x51)
    {
        if ((*MAINFORM)->myquery->RecordCount > 0)
            return;
        if (Players::Players_CountGuildInvites(server->players, player) < 10)
        {
            Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 4, 2));
            return;
        }
        if (!Players::Player_RemoveItem(server->players, player, 1, 0xc350))
            return;
        PacketReader_Init(server, query_result->data, EO_GetBreakByte(server, 0xff));
        PacketReader_GetBreakString(server);
        String tag = AnsiUpperCase(Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server)));
        String name = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        String description = Mysqlcontrols::Db_SanitizeString(
            server->mysql_controls, PacketReader_GetBreakString(server));
        player->guild_tag = tag;
        player->guild_name = name;
        player->guild_rank_name = "Leader";
        player->guild_inviter_id = -1;
        player->guild_rank_id = 1;
        Players::Players_GuildSetMemberInfo(server->players, player, name, tag);
        TDateTime now = Now();
        tag = Mysqlcontrols::Db_SanitizeString(server->mysql_controls, tag);
        name = Mysqlcontrols::Db_SanitizeString(server->mysql_controls, name);
        description =
            Mysqlcontrols::Db_SanitizeString(server->mysql_controls, description);
        String sql = "INSERT INTO endl_guilds (tag, name, description, money, signup, "
                     "rank1, rank2 ) VALUES (";
        sql = sql + "'" + AnsiUpperCase(tag) + "'";
        sql = sql + "'" + name + "',";
        sql = sql + "'" + description + "',";
        sql = sql + "10000,";
        sql = sql + "'" + String(now) + "',";
        sql = sql + "'Leader',";
        sql = sql + "'Recruiter')";
        Mysqlcontrols::Mysql_ExecDirect(server->mysql_controls, 0, sql);
        if (tag.Length() == 2)
            tag = tag + " ";
        String msg = EO_EncodeNumber(server, player->player_id, 2);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        msg.Insert(tag, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        msg.Insert(name, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        msg.Insert("Leader", msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        Guild_BroadcastToAll(server, player, 6, 0x27, msg);
        msg.Insert(EO_EncodeNumber(server, player->item_change_remaining, 4),
                   msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        Client_SendEncoded(server, player, 6, 0x27, msg);
        server->mysql_controls->file_cache->guilds_count++;
        return;
    }
    if (query_result->query_id == 0x52)
    {
        if ((*MAINFORM)->myquery->RecordCount < 1)
            return;
        Player *other = Players::Players_GetById(
            server->players, EO_DecodeNumber(server, query_result->data.SubString(1, 2)));
        if (other == NULL)
            return;
        if (other->guild_tag.Length() > 1)
            return;
        if (other->map_id != player->map_id)
            return;
        if (AnsiLowerCase(Mysqlcontrols::Db_GetString(server->mysql_controls, "tag")) ==
            AnsiLowerCase(other->guild_tag))
            return;
        if ((unsigned int)Mysqlcontrols::Db_GetInt(server->mysql_controls, "money") <
            0x3e8)
            return;
        String tag = Mysqlcontrols::Db_GetString(server->mysql_controls, "tag");
        String name = Mysqlcontrols::Db_GetString(server->mysql_controls, "name");
        String rank9 = Mysqlcontrols::Db_GetString(server->mysql_controls, "rank9");
        int money =
            (unsigned int)Mysqlcontrols::Db_GetInt(server->mysql_controls, "money") -
            0x3e8;
        Mysqlcontrols::Mysql_ExecDirect(
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
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        msg.Insert(tag, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        msg.Insert(name, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        msg.Insert(rank9, msg.Length() + 1);
        msg.Insert(EO_GetBreakByte(server, 0xff), msg.Length() + 1);
        Client_SendEncoded(server, other, 5, 0x27, msg);
        Client_SendEncoded(server, player, 3, 0x27, EO_EncodeNumber(server, 0x10, 2));
        return;
    }
    if (query_result->query_id == 0x53)
    {
        std::vector<FilecacheEntryB *>::iterator it =
            server->mysql_controls->file_cache->pending_guild_writes.begin();
        while (it != server->mysql_controls->file_cache->pending_guild_writes.end())
        {
            FilecacheEntryB *entry = *it;
            it = server->mysql_controls->file_cache->pending_guild_writes.erase(it);
            delete entry;
        }
        Mysqlcontrols::LoadCachedGuilds(server->mysql_controls);
    }
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
// STUB(0x0045d874, 1342 bytes) Walk_BuildReply - ref: AnsiString *
// Walk_BuildReply(AnsiString * out, Server * server, Player * player)
void *Walk_BuildReply_Stub(void *a0, void *a1, void *a2)
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
void Client_SendEncoded(Server *server,
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
        FUN_004728f8(server, out.Length());
        Sock_Send(player->socket, *(char **)&out);
    }
}
// STUB(0x00467980, 12223 bytes) Attack_Execute - ref: int Attack_Execute(Server * server,
// Player * attacker, PacketAction action, AnsiString * packet_data)
bool Attack_Execute(Server *server, Player *caster, int action, String *reader)
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
        if (reader->Length() < 4)
            return 0;
        String tmp = reader->SubString(3, 2);
        int attack_tick = EO_DecodeNumber(server, tmp);
        int elapsed = attack_tick - caster->last_client_walk_tick;
        if (elapsed < 0 && caster->last_client_walk_tick > 0x7270e0)
            elapsed = 0x2c;
        caster->last_client_walk_tick = attack_tick;
        if (elapsed < 0x2c)
            return 0;
        int window = DateTimeToTimeStamp(Now()).Time / 10 + 0x64;
        if (attack_tick > window)
        {
            int ahead = attack_tick - window;
            if (caster->sync_base_ahead < 0)
                caster->sync_base_ahead = ahead;
            if (Math_Abs(ahead - caster->sync_base_ahead) > 0x320)
                return 1;
        }
        int offset_y = 0;
        int offset_x = 0;
        if (caster->direction == 0)
            offset_y++;
        if (caster->direction == 1)
            offset_x--;
        if (caster->direction == 2)
            offset_y--;
        if (caster->direction == 3)
            offset_x++;
        for (Player **iter = Players_Iter_Begin(server->players);
             iter != Players_Iter_End(server->players);
             iter++)
        {
            if ((*iter)->map_id != caster->map_id)
                continue;
            if ((*iter)->x != offset_x)
                continue;
            if ((*iter)->y != offset_y)
                continue;
            if (Player_IsPartyMember(caster, (*iter)->player_id))
                continue;
            int damage = 0;
            int hit_rate = Combat_CalcHitRate(
                (*MAINFORM)->game_control, caster->accuracy, (*iter)->evasion, 0.9, 1.6);
            if (RandRange(100) >= hit_rate)
                continue;
            int pen = Combat_CalcArmorPen((*MAINFORM)->game_control,
                                          (caster->min_damage + caster->max_damage) / 2,
                                          (*iter)->armor,
                                          0.8,
                                          0.9);
            double scaled = (double)caster->min_damage;
            if (scaled < 0.0)
                scaled = 0.0;
            scaled *= 1.2;
            scaled *= (double)pen;
            damage = (int)(scaled + (double)RandRange(caster->max_damage -
                                                      caster->min_damage + 2));
            if (damage < 1)
                damage = 1;
            if (caster->weapon_item_id > 0)
            {
                int element = 0;
                int element2 = 0;
                Eif_GetElement(
                    (*MAINFORM)->item_values, caster->weapon_item_id, &element);
                if (element == 1)
                {
                    double mult = Combat_CalcElementMult((*MAINFORM)->game_control,
                                                         caster->element_resistances[1],
                                                         (*iter)->element_resistances[2],
                                                         element,
                                                         element2);
                    damage = (int)((double)damage * mult);
                }
                if (element == 2)
                {
                    double mult = Combat_CalcElementMult((*MAINFORM)->game_control,
                                                         caster->element_resistances[2],
                                                         (*iter)->element_resistances[1],
                                                         element,
                                                         element2);
                    damage = (int)((double)damage * mult);
                }
            }
            if ((*iter)->direction == caster->direction)
                damage -= damage / 2;
            (*iter)->hp -= damage;
            if ((*iter)->hp > (*iter)->max_hp)
                (*iter)->hp = (*iter)->max_hp;
            if ((*iter)->hp < 1)
                (*iter)->hp = 0;
            if (damage > 0)
            {
                if ((*iter)->in_party)
                {
                    String party_pkt = EO_EncodeNumber(server, (*iter)->player_id, 2);
                    String hp = Player_HpPercent((*iter), 1);
                    party_pkt.Insert(hp, party_pkt.Length() + 1);
                    Server_BroadcastToParty(server, (*iter), 5, 0x18, party_pkt);
                }
                String atk_pkt = EO_EncodeNumber(server, caster->player_id, 2);
                String target_id = EO_EncodeNumber(server, (*iter)->player_id, 2);
                atk_pkt.Insert(target_id, atk_pkt.Length() + 1);
                String dmg = EO_EncodeNumber(server, damage, 3);
                atk_pkt.Insert(dmg, atk_pkt.Length() + 1);
                String dir = EO_EncodeNumber(server, caster->direction, 1);
                atk_pkt.Insert(dir, atk_pkt.Length() + 1);
                String hp = Player_HpPercent((*iter), 1);
                atk_pkt.Insert(hp, atk_pkt.Length() + 1);
            }
            (void)damage;
        }
        if (offset_y < 0 || offset_x < 0)
        {
            String pkt = EO_EncodeNumber(server, caster->player_id, 2);
            String chr = String(reader[1]);
            pkt = pkt + chr;
            Server_BroadcastNearby(server, caster, 8, 0xb, pkt);
            return 1;
        }
        return 0;
    }
    return 0;
}
// STUB(0x0046a9b0, 17449 bytes) Spell_Execute - ref: int Spell_Execute(Server * server,
// Player * caster, int action, AnsiString * packet_data)
bool Spell_Execute(Server *server, Player *caster, int action, String *packet_data)
{
    *(TTimeStamp *)&caster->walk_tick = DateTimeToTimeStamp(Now());
    if (caster->map_id < 1)
        return 1;
    if (caster->weight_max + 2 < caster->weight_current)
        return 1;
    if (action == 1)
    {
        if (packet_data->Length() < 5)
            return 0;
        String spell = packet_data->SubString(1, 2);
        caster->queued_spell_id = EO_DecodeNumber(server, spell);
        if (!Players::Player_HasSpellId(server->players, caster, caster->queued_spell_id))
            return 0;
        int cast_time =
            SkillValues::GetCastTime((*MAINFORM)->skill_values, caster->queued_spell_id) *
            30;
        String target = packet_data->SubString(3, 3);
        caster->expected_cast_timestamp = EO_DecodeNumber(server, target) + cast_time - 1;
        String pkt = EO_EncodeNumber(server, caster->player_id, 2);
        pkt.Insert(EO_EncodeNumber(server, caster->queued_spell_id, 2), pkt.Length() + 1);
        Server_BroadcastNearby(server, caster, 1, 0xc, pkt);
        return 1;
    }
    if (action == 0x1f)
    {
        if (!caster->logged_in)
            return 0;
        if (caster->sitting || caster->on_chair)
            return 1;
        if (packet_data->Length() < 11)
            return 0;
        String spell = packet_data->SubString(2, 3);
        int spell_id = EO_DecodeNumber(server, spell);
        int elapsed = spell_id - caster->last_client_walk_tick;
        if (elapsed < 0 && caster->last_client_walk_tick > 0x7270e0)
            elapsed = 0x2c;
        caster->last_client_walk_tick = spell_id;
        if (elapsed < 0x2c)
            return 0;
        return 0;
    }
    return 0;
}
bool Walk_Execute(Server *server, Player *player, int action, String *data)
{
    *(TTimeStamp *)&player->walk_tick = DateTimeToTimeStamp(Now());
    if (player->map_id < 1)
        return true;
    if (action != PacketAction_Player && action != PacketAction_Spec &&
        action != PacketAction_Admin)
        return false;
    if (action == PacketAction_Admin && player->admin_level < 3)
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
        direction = 3;
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
    if (direction == 0)
    {
        target_y = player->y + 1;
        target_x = player->x;
    }
    if (direction == 1)
    {
        target_x = player->x - 1;
        target_y = player->y;
    }
    if (direction == 2)
    {
        target_y = player->y - 1;
        target_x = player->x;
    }
    if (direction == 3)
    {
        target_x = player->x + 1;
        target_y = player->y;
    }
    if (Mapcontrol::Map_IsOccupied(
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
        if (player->admin_level < 2)
        {
            TTimeStamp now = DateTimeToTimeStamp(Now());
            int elapsed = now.Date - player->last_pass_ms.Date;
            int ms = now.Time - player->last_pass_ms.Time;
            elapsed = ms / 1000 + elapsed * 86400;
            player->last_pass_ms = now;
            if (elapsed < 7)
                return true;
            if (Mapcontrol::Map_GetTileSpec(
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
    int walkable = Mapcontrol::Map_IsWalkableNPC(
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
            unsigned int spec = Mapcontrol::Map_GetTileSpec(
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
                dmg += EO_EncodeNumber(server, player->player_id, 2);
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
        int target_map = Mapcontrol::Map_GetWarpMap(
            server->map_control, player->map_id, target_x, target_y);
        int level_req = Mapcontrol::Map_GetWarpLevelReq(
            server->map_control, player->map_id, target_x, target_y);
        int warp_x = Mapcontrol::Map_GetWarpX(
            server->map_control, player->map_id, target_x, target_y);
        int warp_y = Mapcontrol::Map_GetWarpY(
            server->map_control, player->map_id, target_x, target_y);
        if (player->level < level_req)
            return true;
        if (target_map > 0 && target_map <= Mapcontrol_GetCount(server->map_control))
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
            player->warp_x = (unsigned short)warp_x;
            player->warp_y = (unsigned short)warp_y;
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
String EO_Encode_Interleave(Server *server, int multiple, char *begin, char *end)
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
String EO_Decode_Deinterleave(Server *server, int multiple, char *begin, char *end)
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
                unsigned char v = woven.front();
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
                woven.pop();
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
void FUN_00472944(Server *server, int value)
{
    server->received_bytes += value;
    while (server->received_bytes > 0x3ff)
    {
        server->received_kilobytes++;
        server->received_bytes -= 0x400;
    }
    while (server->received_kilobytes > 0x3ff)
    {
        server->received_megabytes++;
        server->received_kilobytes -= 0x400;
    }
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
