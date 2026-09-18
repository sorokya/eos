#include <vcl.h>
#pragma hdrstop

#include "Eventcontrol.h"
#include "Map.h"
#include "Mapwarp.h"
#include "Player.h"
#include "Protocol.h"

#pragma package(smart_init)

// Cross-unit operations this unit invokes. Their mangled names are unobservable
// in the stripped image, so they are declared here; the argument shapes are
// pinned by the reference call sites. Replaced by the owning units' headers
// once those are reconstructed.
MapContainer *MapVector_Begin(Mapcontrol *map_control);
MapContainer *MapVector_End(Mapcontrol *map_control);
MapWarp *MapwarpVector_Begin(void *arena_spawn_list);
MapWarp *MapwarpVector_End(void *arena_spawn_list);

Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);
int Players_CountArenaPlayers(Players *players, int map_id);
Player *Players_GetByMapTile(Players *players, int map_id, int x, int y);

void Server_BroadcastToMap(Server *server,
                           int map_id,
                           unsigned char action,
                           unsigned char family,
                           String payload);
void Player_Warp(Server *server,
                 Player *player,
                 int target_map,
                 TPoint pos,
                 int warp_anim,
                 bool do_leave);

int RandRange(int max);

EventController::EventController(Mapcontrol *map_control,
                                 Players *players,
                                 Server *server,
                                 Settings *settings)
{
    pEncode_scratch = (char *)operator new(8);
    this->map_control = map_control;
    this->settings = settings;
    this->players = players;
    this->server = server;
}

EventController::~EventController()
{
}

String
EventController::AppendEncoded(EventController *self, unsigned int value, int width)
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
                self->pEncode_scratch[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = 0xfe;
                self->pEncode_scratch[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded_str(self->pEncode_scratch, width);
    return encoded_str;
}

void EventController::Tick(EventController *self)
{
    for (MapContainer *map_iter = MapVector_Begin(self->map_control);
         map_iter != MapVector_End(self->map_control);
         map_iter++)
    {
        if (map_iter->quest_cooldown > 0)
            map_iter->quest_cooldown--;
        if (map_iter->evac_countdown > 0)
        {
            map_iter->evac_countdown--;
            if (map_iter->evac_countdown == 0x1e || map_iter->evac_countdown == 0x14 ||
                map_iter->evac_countdown == 0xa)
            {
                if (map_iter->evac_countdown == 0xa)
                {
                    Server_BroadcastToMap(self->server,
                                          map_iter->rid,
                                          PacketAction_Server,
                                          PacketFamily_Talk,
                                          "Last warning! - leave this map in (" +
                                              IntToStr(map_iter->evac_countdown) +
                                              ") seconds or be send to jail!");
                }
                else
                {
                    Server_BroadcastToMap(self->server,
                                          map_iter->rid,
                                          PacketAction_Server,
                                          PacketFamily_Talk,
                                          "Warning! - please leave this map in (" +
                                              IntToStr(map_iter->evac_countdown) +
                                              ") seconds or be send to jail!");
                }
            }
            else if (map_iter->evac_countdown > 3)
            {
                Server_BroadcastToMap(self->server,
                                      map_iter->rid,
                                      PacketAction_Player,
                                      PacketFamily_Music,
                                      AppendEncoded(self, 0x33, 1));
            }
            if (map_iter->evac_countdown < 1)
            {
                for (Player **player_iter = Players_Iter_Begin(self->players);
                     player_iter != Players_Iter_End(self->players);
                     player_iter++)
                {
                    if ((*player_iter)->connected &&
                        (*player_iter)->admin_level < AdminLevel_Spy &&
                        (*player_iter)->map_id == map_iter->rid)
                    {
                        TPoint pos;
                        pos.x = RandRange(3) + 0xe;
                        pos.y = RandRange(3) + 0x25;
                        Player_Warp(self->server, *player_iter, 0x2f, pos, 0, false);
                    }
                }
            }
        }
        if (map_iter->arena_enabled)
        {
            map_iter->arena_ticks++;
            if (map_iter->arena_ticks % 0x3c == 0)
            {
                if (map_iter->arena_ticks > 0x77)
                    map_iter->arena_ticks = 0;
                int queued_count =
                    Players_CountArenaPlayers(self->players, map_iter->rid);
                int warped_count = 0;
                if (queued_count >= map_iter->arena_block)
                {
                    if (map_iter->arena_ticks == 0)
                    {
                        Server_BroadcastToMap(self->server,
                                              map_iter->rid,
                                              PacketAction_Drop,
                                              PacketFamily_Arena,
                                              "N");
                    }
                }
                else
                {
                    for (MapWarp *spawn_iter =
                             MapwarpVector_Begin(&map_iter->arena_spawn_list);
                         spawn_iter != MapwarpVector_End(&map_iter->arena_spawn_list);
                         spawn_iter++)
                    {
                        Player *target_player = Players_GetByMapTile(self->players,
                                                                     map_iter->rid,
                                                                     spawn_iter->from_x,
                                                                     spawn_iter->from_y);
                        if (target_player != NULL)
                        {
                            warped_count++;
                            TPoint pos;
                            pos.x = spawn_iter->to_x;
                            pos.y = spawn_iter->to_y;
                            target_player->arena_playing = true;
                            target_player->arena_queued = false;
                            target_player->arena_kills = 0;
                            Player_Warp(self->server,
                                        target_player,
                                        map_iter->rid,
                                        pos,
                                        0,
                                        false);
                            if (map_iter->arena_block == 2 && queued_count == 1)
                                break;
                        }
                    }
                    if (warped_count > 0)
                    {
                        Server_BroadcastToMap(self->server,
                                              map_iter->rid,
                                              PacketAction_Use,
                                              PacketFamily_Arena,
                                              AppendEncoded(self, warped_count, 1));
                    }
                }
            }
        }
    }
}
