#include <vcl.h>
#pragma hdrstop

#include "Effectcontrol.h"

#pragma package(smart_init)

struct MapContainer;
struct Player;

Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);
MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index);
unsigned int Map_GetTileSpec(Mapcontrol *map_control, int map_id, int x, int y);
int Player_HpPercent(Player *player);
void Client_SendEncoded(
    Server *server, Player *player, int action, int family, String data);
void Server_BroadcastNearby(
    Server *server, Player *player, int action, int family, String data);
void Player_Respawn(Server *server, Player *player);
int RandRange(int max);

struct Player
{
    char pad_00[0x8];
    int player_id; // +0x08
    char pad_0c[0xdc - 0x0c];
    int map_id; // +0xdc
    int x;      // +0xe0
    int y;      // +0xe4
    char pad_e8[0x108 - 0xe8];
    int max_hp; // +0x108
    int hp;     // +0x10c
    char pad_110[0x114 - 0x110];
    int max_tp; // +0x114
    int tp;     // +0x118
    char pad_11c[0x3c8 - 0x11c];
    int field_0x3c8; // +0x3c8
    char pad_3cc[0x3dc - 0x3cc];
    char map_has_quakes;   // +0x3dc
    char map_has_hp_drain; // +0x3dd
    char map_has_tp_drain; // +0x3de
    char map_has_spikes;   // +0x3df
};

struct MapContainer
{
    char pad_00[0xb];
    unsigned char field_0xb; // +0x0b
    char pad_0c[0x54 - 0x0c];
    String field_0x54; // +0x54
    char field_0x58;   // +0x58
};

EffectController::EffectController(Mapcontrol *map_control,
                                   Players *players,
                                   Server *server,
                                   Settings *settings)
{
    pEncode_scratch = (char *)operator new(8);
    this->map_control = map_control;
    this->settings = settings;
    this->players = players;
    this->server = server;
    for (int i = 0; i < 4; i++)
    {
        aState_countdown[i] = 10;
        aState_value[i] = 0;
        aState_extra[i] = 1;
    }
    nBroadcast_gate = 10;
}

EffectController::~EffectController()
{
}

String
EffectController::AppendEncoded(EffectController *self, unsigned int value, int width)
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

void EffectController::Tick(EffectController *self)
{
    for (int countdown_slot = 0; countdown_slot < 4; countdown_slot++)
        self->aState_countdown[countdown_slot]--;
    self->nBroadcast_gate--;

    Player **player_iter;
    for (player_iter = Players_Iter_Begin(self->players);
         player_iter != Players_Iter_End(self->players);
         player_iter++)
    {
        if ((*player_iter)->map_has_quakes != 0)
        {
            int idx = Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                          ->field_0xb -
                      3;
            if (idx >= 0 && idx <= 3)
            {
                if (self->aState_countdown[idx] < 1)
                {
                    String pkt = AppendEncoded(self, 1, 1);
                    pkt.Insert(AppendEncoded(self, self->aState_extra[idx], 1),
                               pkt.Length() + 1);
                    Client_SendEncoded(self->server, *player_iter, 0xa, 0x1f, pkt);
                }
            }
            else
                continue;
        }

        if (self->nBroadcast_gate < 1)
        {
            if ((*player_iter)->map_has_hp_drain != 0 && (*player_iter)->hp > 0)
            {
                int hp_regen = (*player_iter)->max_hp / 10;
                if (hp_regen < 1)
                    hp_regen = 1;
                if ((*player_iter)->hp <= hp_regen)
                    hp_regen = (*player_iter)->hp - 1;
                (*player_iter)->hp -= hp_regen;
                (*player_iter)->field_0x3c8 = hp_regen;
                if (Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                        ->field_0x58 != 0)
                {
                    Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                        ->field_0x54 += "";
                    Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                        ->field_0x58 = 0;
                }
                Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->field_0x54.Insert(AppendEncoded(self, (*player_iter)->player_id, 2),
                                        Mapcontrol_GetByIndex(self->map_control,
                                                              (*player_iter)->map_id - 1)
                                                ->field_0x54.Length() +
                                            1);
                Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->field_0x54.Insert(
                        AppendEncoded(self, Player_HpPercent(*player_iter), 1),
                        Mapcontrol_GetByIndex(self->map_control,
                                              (*player_iter)->map_id - 1)
                                ->field_0x54.Length() +
                            1);
                Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->field_0x54.Insert(AppendEncoded(self, hp_regen, 2),
                                        Mapcontrol_GetByIndex(self->map_control,
                                                              (*player_iter)->map_id - 1)
                                                ->field_0x54.Length() +
                                            1);
            }
            if ((*player_iter)->map_has_tp_drain != 0 && (*player_iter)->tp > 0)
            {
                int tp_regen = (*player_iter)->max_tp / 10;
                if (tp_regen < 1)
                    tp_regen = 1;
                if ((*player_iter)->tp <= tp_regen)
                    tp_regen = (*player_iter)->tp - 1;
                (*player_iter)->tp -= tp_regen;
                String pkt = AppendEncoded(self, 1, 1);
                pkt.Insert(AppendEncoded(self, tp_regen, 2), pkt.Length() + 1);
                pkt.Insert(AppendEncoded(self, (*player_iter)->tp, 2), pkt.Length() + 1);
                pkt.Insert(AppendEncoded(self, (*player_iter)->max_tp, 2),
                           pkt.Length() + 1);
                Client_SendEncoded(self->server, *player_iter, 0x10, 0x1f, pkt);
            }
        }

        if ((*player_iter)->map_has_spikes != 0)
        {
            unsigned int spec = Map_GetTileSpec(self->map_control,
                                                (*player_iter)->map_id,
                                                (*player_iter)->x,
                                                (*player_iter)->y);
            if (spec == 0x21 || spec == 0x22)
            {
                if ((*player_iter)->hp > 0)
                {
                    int dmg = (*player_iter)->max_hp / 5;
                    int died = 0;
                    if (dmg < 1)
                        dmg = 1;
                    if ((*player_iter)->hp < dmg)
                        dmg = (*player_iter)->hp;
                    (*player_iter)->hp -= dmg;
                    if ((*player_iter)->hp <= 0)
                    {
                        (*player_iter)->hp = 0;
                        died = 1;
                    }
                    String pkt = AppendEncoded(self, 2, 1);
                    pkt.Insert(AppendEncoded(self, dmg, 2), pkt.Length() + 1);
                    pkt.Insert(AppendEncoded(self, (*player_iter)->hp, 2),
                               pkt.Length() + 1);
                    pkt.Insert(AppendEncoded(self, (*player_iter)->max_hp, 2),
                               pkt.Length() + 1);
                    Client_SendEncoded(self->server, *player_iter, 0x10, 0x1f, pkt);
                    pkt += AppendEncoded(self, (*player_iter)->player_id, 2);
                    pkt.Insert(AppendEncoded(self, Player_HpPercent(*player_iter), 1),
                               pkt.Length() + 1);
                    pkt.Insert(AppendEncoded(self, died, 1), pkt.Length() + 1);
                    pkt.Insert(AppendEncoded(self, dmg, 2), pkt.Length() + 1);
                    Server_BroadcastNearby(self->server, *player_iter, 0x11, 0x1f, pkt);
                    if (died != 0)
                        Player_Respawn(self->server, *player_iter);
                }
            }
            else
            {
                Client_SendEncoded(self->server, *player_iter, 0x15, 0x1f, "S");
            }
        }
    }

    if (self->nBroadcast_gate < 1)
    {
        Player **broadcast_iter;
        for (broadcast_iter = Players_Iter_Begin(self->players);
             broadcast_iter != Players_Iter_End(self->players);
             broadcast_iter++)
        {
            if ((*broadcast_iter)->map_has_hp_drain != 0)
            {
                Mapcontrol_GetByIndex(self->map_control, (*broadcast_iter)->map_id - 1)
                    ->field_0x58 = 1;
                String pkt = AppendEncoded(self, (*broadcast_iter)->field_0x3c8, 2);
                pkt.Insert(AppendEncoded(self, (*broadcast_iter)->hp, 2),
                           pkt.Length() + 1);
                pkt.Insert(AppendEncoded(self, (*broadcast_iter)->max_hp, 2),
                           pkt.Length() + 1);
                pkt.Insert(Mapcontrol_GetByIndex(self->map_control,
                                                 (*broadcast_iter)->map_id - 1)
                               ->field_0x54,
                           pkt.Length() + 1);
                Client_SendEncoded(self->server, *broadcast_iter, 0x1f, 0x1f, pkt);
            }
        }
    }

    for (int reroll_slot = 0; reroll_slot < 4; reroll_slot++)
    {
        if (self->aState_countdown[reroll_slot] < 1)
        {
            int base = 4 - reroll_slot;
            self->aState_countdown[reroll_slot] =
                RandRange(0x32 - 10 * (reroll_slot + 1)) + base * 4 + 2;
            self->aState_value[reroll_slot] += self->aState_countdown[reroll_slot];
            if (0x118 - 40 * reroll_slot < self->aState_value[reroll_slot])
            {
                int r7 = RandRange(7);
                if (r7 > 4 || self->aState_value[reroll_slot] > 600)
                {
                    self->aState_countdown[reroll_slot] = base + 3;
                    self->aState_value[reroll_slot] = 0;
                }
            }
            self->aState_extra[reroll_slot] = RandRange(3) + reroll_slot * 2;
            if (reroll_slot == 1)
                self->aState_extra[reroll_slot] -= 2;
        }
    }
    if (self->nBroadcast_gate < 1)
        self->nBroadcast_gate = 10;
}
