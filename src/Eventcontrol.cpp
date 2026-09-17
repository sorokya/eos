#include <vcl.h>
#pragma hdrstop

#include "Eventcontrol.h"

#pragma package(smart_init)

// Minimal view of the (unreconstructed) Mapwarp/Map units. Only the fields
// this unit reads are declared, at the offsets pinned by the reference
// disassembly; the remaining bytes are padding. sizeof(Map) must be 0x160 (the
// reference map-vector stride is `add [map_iter],0x160`). The reference reads the
// Mapwarp coordinates with movzx, so the fields are unsigned short (Mapwarp.h
// declares short — reconcile).
struct Mapwarp
{
    unsigned short from_x; // +0x00
    unsigned short from_y; // +0x02
    int dest_map;          // +0x04
    int level;             // +0x08
    unsigned short to_x;   // +0x0c
    unsigned short to_y;   // +0x0e
};

struct MapwarpVector
{
    char pad_00[0x20];
};

struct Map
{
    unsigned short rid; // +0x00
    char pad_02[0x10 - 0x02];
    bool arena_enabled;             // +0x10
    char pad_11[3];                 // +0x11
    int arena_block;                // +0x14
    int arena_ticks;                // +0x18
    int field_0x1c;                 // +0x1c
    int evac_countdown;             // +0x20
    MapwarpVector arena_spawn_list; // +0x24
    char pad_44[0x160 - 0x44];
};

// Minimal view of the (unreconstructed) Player unit. Only the fields this unit
// reads are declared, at the offsets pinned by the reference disassembly.
struct Player
{
    bool connected; // +0x00
    char pad_01[0x54 - 0x01];
    bool arena_queued;  // +0x54
    bool arena_playing; // +0x55
    char pad_56[0x58 - 0x56];
    int field_0x58; // +0x58
    char pad_5c[0x98 - 0x5c];
    int admin_level; // +0x98
    char pad_9c[0xdc - 0x9c];
    int map_id; // +0xdc
    int x;      // +0xe0
    int y;      // +0xe4
};

// Cross-unit operations this unit invokes. Their mangled names are unobservable
// in the stripped image, so they are declared here; the argument shapes are
// pinned by the reference call sites. Replaced by the owning units' headers
// once those are reconstructed.
Map *MapVector_Begin(Mapcontrol *map_control);
Map *MapVector_End(Mapcontrol *map_control);
Mapwarp *MapwarpVector_Begin(MapwarpVector *arena_spawn_list);
Mapwarp *MapwarpVector_End(MapwarpVector *arena_spawn_list);

Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);
int Players_CountArenaPlayers(Players *players, int map_id);
Player *Players_GetByMapTile(Players *players, int map_id, int x, int y);

void Server_BroadcastToMap(
    Server *server, int map_id, int action, int family, String payload);
void Player_Warp(Server *server,
                 Player *player,
                 int target_map,
                 TPoint pos,
                 int warp_anim,
                 bool do_leave);

int RandRange(int max);

Eventcontrol::Eventcontrol(Mapcontrol *map_control,
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

Eventcontrol::~Eventcontrol()
{
}

String Eventcontrol::AppendEncoded(Eventcontrol *self, unsigned int value, int width)
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

void Eventcontrol::Tick(Eventcontrol *self)
{
    for (Map *map_iter = MapVector_Begin(self->map_control);
         map_iter != MapVector_End(self->map_control);
         map_iter++)
    {
        if (map_iter->field_0x1c > 0)
            map_iter->field_0x1c--;
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
                                          0x17,
                                          0x12,
                                          "Last warning! - leave this map in (" +
                                              IntToStr(map_iter->evac_countdown) +
                                              ") seconds or be send to jail!");
                }
                else
                {
                    Server_BroadcastToMap(self->server,
                                          map_iter->rid,
                                          0x17,
                                          0x12,
                                          "Warning! - please leave this map in (" +
                                              IntToStr(map_iter->evac_countdown) +
                                              ") seconds or be send to jail!");
                }
            }
            else if (map_iter->evac_countdown > 3)
            {
                Server_BroadcastToMap(
                    self->server, map_iter->rid, 8, 0x28, AppendEncoded(self, 0x33, 1));
            }
            if (map_iter->evac_countdown < 1)
            {
                for (Player **player_iter = Players_Iter_Begin(self->players);
                     player_iter != Players_Iter_End(self->players);
                     player_iter++)
                {
                    if ((*player_iter)->connected && (*player_iter)->admin_level < 1 &&
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
                        Server_BroadcastToMap(
                            self->server, map_iter->rid, 0x18, 0x2d, "N");
                    }
                }
                else
                {
                    for (Mapwarp *spawn_iter =
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
                            target_player->field_0x58 = 0;
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
                                              0xa,
                                              0x2d,
                                              AppendEncoded(self, warped_count, 1));
                    }
                }
            }
        }
    }
}
